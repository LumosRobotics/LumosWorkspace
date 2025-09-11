#include "settings_manager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>

SettingsManager::SettingsManager(QObject* parent) 
    : QObject(parent), colorsModifiedInSession(false) {
    setDefaults();
}

QString SettingsManager::getSettingsFilePath() const {
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataPath);
    return appDataPath + "/settings.json";
}

void SettingsManager::setDefaults() {
    settings = QJsonObject{
        {"window.width", 800},
        {"window.height", 600},
        {"window.x", 100},
        {"window.y", 100},
        {"window.maximized", false},
        {"ui.layout_mode", "bottom_input"},
        {"ui.font_family", "Monaco"},
        {"ui.splitter_sizes", QJsonArray{600, 200}},
        {"ui.background_color", "#2b2b2b"},
        {"ui.text_color", "#ffffff"},
        {"ui.border_color", "#555555"},
        {"tcp.port", 8080}
    };
}

bool SettingsManager::loadSettings() {
    QString filePath = getSettingsFilePath();
    QJsonObject loadedSettings = loadJsonFromFile(filePath);
    
    if (!loadedSettings.isEmpty()) {
        // Merge loaded settings with defaults
        for (auto it = loadedSettings.begin(); it != loadedSettings.end(); ++it) {
            settings[it.key()] = it.value();
        }
    }
    
    emit settingsLoaded();
    return true;
}

bool SettingsManager::saveSettings() {
    // Only save color settings if they were modified programmatically
    if (colorsModifiedInSession) {
        QString filePath = getSettingsFilePath();
        bool success = saveJsonToFile(filePath, settings);
        if (success) {
            emit settingsSaved();
        }
        return success;
    } else {
        // Save non-color settings
        QJsonObject toSave = settings;
        toSave.remove("ui.background_color");
        toSave.remove("ui.text_color");
        toSave.remove("ui.border_color");
        
        QString filePath = getSettingsFilePath();
        
        // Load existing file and preserve color settings
        QJsonObject existing = loadJsonFromFile(filePath);
        if (existing.contains("ui.background_color")) {
            toSave["ui.background_color"] = existing["ui.background_color"];
        }
        if (existing.contains("ui.text_color")) {
            toSave["ui.text_color"] = existing["ui.text_color"];
        }
        if (existing.contains("ui.border_color")) {
            toSave["ui.border_color"] = existing["ui.border_color"];
        }
        
        bool success = saveJsonToFile(filePath, toSave);
        if (success) {
            emit settingsSaved();
        }
        return success;
    }
}

QJsonObject SettingsManager::loadJsonFromFile(const QString& filePath) const {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }
    
    QByteArray data = file.readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << error.errorString();
        return QJsonObject();
    }
    
    return doc.object();
}

bool SettingsManager::saveJsonToFile(const QString& filePath, const QJsonObject& json) const {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open settings file for writing:" << filePath;
        return false;
    }
    
    QJsonDocument doc(json);
    file.write(doc.toJson());
    return true;
}

QVariant SettingsManager::getValue(const QString& key, const QVariant& defaultValue) const {
    if (settings.contains(key)) {
        return settings[key].toVariant();
    }
    return defaultValue;
}

void SettingsManager::setValue(const QString& key, const QVariant& value) {
    QJsonValue jsonValue = QJsonValue::fromVariant(value);
    if (settings[key] != jsonValue) {
        settings[key] = jsonValue;
        
        // Mark colors as modified if setting a color
        if (key.contains("color")) {
            markColorsModified();
        }
        
        emit settingChanged(key, value);
    }
}

QString SettingsManager::getString(const QString& key, const QString& defaultValue) const {
    return getValue(key, defaultValue).toString();
}

int SettingsManager::getInt(const QString& key, int defaultValue) const {
    return getValue(key, defaultValue).toInt();
}

bool SettingsManager::getBool(const QString& key, bool defaultValue) const {
    return getValue(key, defaultValue).toBool();
}

QColor SettingsManager::getColor(const QString& key, const QColor& defaultValue) const {
    QString colorString = getString(key, defaultValue.name());
    return QColor(colorString);
}

QList<int> SettingsManager::getIntList(const QString& key, const QList<int>& defaultValue) const {
    QJsonArray array = settings[key].toArray();
    QList<int> result;
    
    if (array.isEmpty()) {
        return defaultValue;
    }
    
    for (const QJsonValue& value : array) {
        result.append(value.toInt());
    }
    return result;
}

bool SettingsManager::contains(const QString& key) const {
    return settings.contains(key);
}

void SettingsManager::markColorsModified() {
    colorsModifiedInSession = true;
}
#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QJsonObject>
#include <QColor>

class SettingsManager : public QObject {
    Q_OBJECT

public:
    explicit SettingsManager(QObject* parent = nullptr);
    
    // Load and save settings
    bool loadSettings();
    bool saveSettings();
    
    // Generic getters/setters
    QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setValue(const QString& key, const QVariant& value);
    
    // Type-specific getters for convenience
    QString getString(const QString& key, const QString& defaultValue = "") const;
    int getInt(const QString& key, int defaultValue = 0) const;
    bool getBool(const QString& key, bool defaultValue = false) const;
    QColor getColor(const QString& key, const QColor& defaultValue = QColor()) const;
    QList<int> getIntList(const QString& key, const QList<int>& defaultValue = {}) const;
    
    // Check if key exists
    bool contains(const QString& key) const;
    
    // Mark colors as modified (for conditional saving)
    void markColorsModified();
    bool areColorsModified() const { return colorsModifiedInSession; }

signals:
    void settingChanged(const QString& key, const QVariant& value);
    void settingsLoaded();
    void settingsSaved();

private:
    QString getSettingsFilePath() const;
    QJsonObject loadJsonFromFile(const QString& filePath) const;
    bool saveJsonToFile(const QString& filePath, const QJsonObject& json) const;
    void setDefaults();
    
    QJsonObject settings;
    bool colorsModifiedInSession;
};
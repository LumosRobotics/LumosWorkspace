#pragma once

#include <QObject>
#include <QColor>
#include <QWidget>

class SettingsManager;

struct ThemeColors {
    QColor backgroundColor;
    QColor textColor;
    QColor borderColor;
};

class UIThemeManager : public QObject {
    Q_OBJECT

public:
    explicit UIThemeManager(SettingsManager* settingsManager, QObject* parent = nullptr);
    
    void applyTheme(QWidget* rootWidget);
    
    // Get current theme colors
    ThemeColors getCurrentTheme() const;
    
    // Update theme colors
    void setThemeColors(const ThemeColors& colors);

public slots:
    void onSettingChanged(const QString& key, const QVariant& value);

signals:
    void themeChanged(const ThemeColors& colors);

private:
    void applyColorsToWidget(QWidget* widget, const ThemeColors& colors);
    void applyColorsRecursively(QWidget* widget, const ThemeColors& colors);
    QString generateStyleSheet(const ThemeColors& colors) const;
    
    SettingsManager* settings;
    ThemeColors currentTheme;
};
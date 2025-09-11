#include "ui_theme_manager.h"
#include "settings_manager.h"
#include <QApplication>
#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QListWidget>
#include <QSplitter>
#include <QPushButton>
#include <QLabel>

UIThemeManager::UIThemeManager(SettingsManager* settingsManager, QObject* parent)
    : QObject(parent), settings(settingsManager) {
    
    // Load initial theme
    currentTheme.backgroundColor = settings->getColor("ui.background_color", QColor("#2b2b2b"));
    currentTheme.textColor = settings->getColor("ui.text_color", QColor("#ffffff"));
    currentTheme.borderColor = settings->getColor("ui.border_color", QColor("#555555"));
    
    // Connect to settings changes
    connect(settings, &SettingsManager::settingChanged,
            this, &UIThemeManager::onSettingChanged);
}

void UIThemeManager::applyTheme(QWidget* rootWidget) {
    if (!rootWidget) return;
    
    // Apply theme to the entire widget hierarchy
    applyColorsRecursively(rootWidget, currentTheme);
    
    // Apply global stylesheet
    QString globalStyle = generateStyleSheet(currentTheme);
    qApp->setStyleSheet(globalStyle);
}

ThemeColors UIThemeManager::getCurrentTheme() const {
    return currentTheme;
}

void UIThemeManager::setThemeColors(const ThemeColors& colors) {
    currentTheme = colors;
    
    // Update settings
    settings->setValue("ui.background_color", colors.backgroundColor.name());
    settings->setValue("ui.text_color", colors.textColor.name());
    settings->setValue("ui.border_color", colors.borderColor.name());
    
    emit themeChanged(colors);
}

void UIThemeManager::onSettingChanged(const QString& key, const QVariant& value) {
    bool themeUpdated = false;
    
    if (key == "ui.background_color") {
        currentTheme.backgroundColor = QColor(value.toString());
        themeUpdated = true;
    } else if (key == "ui.text_color") {
        currentTheme.textColor = QColor(value.toString());
        themeUpdated = true;
    } else if (key == "ui.border_color") {
        currentTheme.borderColor = QColor(value.toString());
        themeUpdated = true;
    }
    
    if (themeUpdated) {
        emit themeChanged(currentTheme);
    }
}

void UIThemeManager::applyColorsRecursively(QWidget* widget, const ThemeColors& colors) {
    if (!widget) return;
    
    applyColorsToWidget(widget, colors);
    
    // Apply to all child widgets
    const QList<QWidget*> children = widget->findChildren<QWidget*>();
    for (QWidget* child : children) {
        applyColorsToWidget(child, colors);
    }
}

void UIThemeManager::applyColorsToWidget(QWidget* widget, const ThemeColors& colors) {
    if (!widget) return;
    
    QString objectName = widget->objectName();
    
    // Apply specific styling based on widget type and object name
    if (qobject_cast<QTextEdit*>(widget)) {
        if (objectName == "REPLOutput") {
            // REPL output area - no border, rounded corners
            widget->setStyleSheet(QString(
                "QTextEdit { "
                "background-color: %1; "
                "color: %2; "
                "border: none; "
                "border-radius: 8px; "
                "padding: 8px; "
                "}"
            ).arg(colors.backgroundColor.name())
             .arg(colors.textColor.name()));
        } else if (objectName == "REPLInput") {
            // REPL input area - subtle border, rounded corners
            widget->setStyleSheet(QString(
                "QTextEdit { "
                "background-color: %1; "
                "color: %2; "
                "border: 1px solid %3; "
                "border-radius: 8px; "
                "padding: 4px; "
                "}"
            ).arg(colors.backgroundColor.lighter(110).name())
             .arg(colors.textColor.name())
             .arg(colors.borderColor.name()));
        } else {
            // Other text edits - standard styling
            widget->setStyleSheet(QString(
                "QTextEdit { "
                "background-color: %1; "
                "color: %2; "
                "border: 1px solid %3; "
                "border-radius: 6px; "
                "}"
            ).arg(colors.backgroundColor.name())
             .arg(colors.textColor.name())
             .arg(colors.borderColor.name()));
        }
    }
    else if (qobject_cast<QLineEdit*>(widget)) {
        widget->setStyleSheet(QString(
            "QLineEdit { "
            "background-color: %1; "
            "color: %2; "
            "border: 1px solid %3; "
            "}"
        ).arg(colors.backgroundColor.name())
         .arg(colors.textColor.name())
         .arg(colors.borderColor.name()));
    }
    else if (qobject_cast<QListWidget*>(widget)) {
        if (objectName == "VariablesList") {
            // Variables panel - no border, rounded corners, subtle styling
            widget->setStyleSheet(QString(
                "QListWidget { "
                "background-color: %1; "
                "color: %2; "
                "border: none; "
                "border-radius: 8px; "
                "padding: 4px; "
                "} "
                "QListWidget::item { "
                "padding: 4px; "
                "border-radius: 4px; "
                "} "
                "QListWidget::item:selected { "
                "background-color: %3; "
                "}"
            ).arg(colors.backgroundColor.name())
             .arg(colors.textColor.name())
             .arg(colors.borderColor.name()));
        } else {
            widget->setStyleSheet(QString(
                "QListWidget { "
                "background-color: %1; "
                "color: %2; "
                "border: 1px solid %3; "
                "border-radius: 6px; "
                "}"
            ).arg(colors.backgroundColor.name())
             .arg(colors.textColor.name())
             .arg(colors.borderColor.name()));
        }
    }
    else if (objectName == "CustomTitleBar") {
        widget->setStyleSheet(QString(
            "#CustomTitleBar { "
            "background-color: %1; "
            "border-bottom: 1px solid %2; "
            "}"
        ).arg(colors.backgroundColor.darker(110).name())
         .arg(colors.borderColor.name()));
    }
    else if (objectName == "VariablesPanel") {
        // Variables panel container - rounded corners
        widget->setStyleSheet(QString(
            "#VariablesPanel { "
            "background-color: %1; "
            "border-radius: 12px; "
            "margin: 4px; "
            "}"
        ).arg(colors.backgroundColor.name()));
    }
    else if (objectName == "REPLInterface") {
        // REPL interface container - rounded corners  
        widget->setStyleSheet(QString(
            "#REPLInterface { "
            "background-color: %1; "
            "border-radius: 12px; "
            "margin: 4px; "
            "}"
        ).arg(colors.backgroundColor.name()));
    }
    else if (objectName == "TitleLabel") {
        widget->setStyleSheet(QString(
            "#TitleLabel { "
            "color: %1; "
            "font-weight: bold; "
            "}"
        ).arg(colors.textColor.name()));
    }
    else if (objectName == "VariablesSeparator") {
        // Variables panel separator line - very thin
        widget->setStyleSheet(QString(
            "#VariablesSeparator { "
            "background-color: %1; "
            "border: none; "
            "margin: 5px 0px; "
            "height: 1px; "
            "}"
        ).arg(colors.borderColor.name()));
    }
    else {
        // General widget styling
        QPalette palette = widget->palette();
        palette.setColor(QPalette::Window, colors.backgroundColor);
        palette.setColor(QPalette::WindowText, colors.textColor);
        palette.setColor(QPalette::Base, colors.backgroundColor);
        palette.setColor(QPalette::Text, colors.textColor);
        widget->setPalette(palette);
        widget->setAutoFillBackground(true);
    }
}

QString UIThemeManager::generateStyleSheet(const ThemeColors& colors) const {
    return QString(R"(
        QMainWindow {
            background-color: %1;
            color: %2;
        }
        
        QWidget {
            background-color: %1;
            color: %2;
        }
        
        QSplitter::handle {
            background-color: %3;
            border-radius: 1px;
        }
        
        QSplitter::handle:horizontal {
            width: 1px;
        }
        
        QSplitter::handle:vertical {
            height: 1px;
        }
        
        QScrollBar:vertical {
            background-color: %1;
            border: 1px solid %3;
            width: 12px;
        }
        
        QScrollBar::handle:vertical {
            background-color: %3;
            border-radius: 5px;
        }
        
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical {
            border: none;
            background: none;
        }
    )").arg(colors.backgroundColor.name())
       .arg(colors.textColor.name())
       .arg(colors.borderColor.name());
}
#pragma once

#include <QMainWindow>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QKeySequence>
#include <QShortcut>
#include <memory>

// Forward declarations
class PythonEngine;
class SettingsManager;
class UIThemeManager;
class CustomTitleBar;
class REPLInterface;
class VariablesPanel;
class LayoutManager;
class TCPServer;
class DebugAPI;

// Include needed enums and structs
#include "layout_manager.h"
#include "ui_theme_manager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void moveEvent(QMoveEvent* event) override;

private slots:
    void onCommandExecuted(const QString& command, const QString& result);
    void onVariableSelected(const QString& name, const QString& value);
    void onLayoutModeChanged(LayoutMode mode);
    void onThemeChanged(const ThemeColors& colors);
    
    // TCP and Debug API slots
    void onTcpClientConnected(const QString& address);
    void onTcpDataReceived(const QJsonObject& data);
    void onDebugCommandReceived(const QString& command);

private:
    void initializeApplication();
    void createComponents();
    void setupLayout();
    void setupConnections();
    void setupShortcuts();
    void loadSettings();
    void saveSettings();
    void applyInitialTheme();
    void startServers();
    void stopServers();
    
    // Core components
    std::unique_ptr<PythonEngine> pythonEngine;
    std::unique_ptr<SettingsManager> settingsManager;
    std::unique_ptr<UIThemeManager> themeManager;
    
    // UI components
    std::unique_ptr<CustomTitleBar> titleBar;
    std::unique_ptr<REPLInterface> replInterface;
    std::unique_ptr<VariablesPanel> variablesPanel;
    std::unique_ptr<LayoutManager> layoutManager;
    
    // Network components
    std::unique_ptr<TCPServer> tcpServer;
    std::unique_ptr<DebugAPI> debugAPI;
    
    // UI structure
    QWidget* centralWidget;
    QVBoxLayout* mainLayout;
    
    // Shortcuts
    QShortcut* quitShortcut;
};
#include "main_window.h"
#include "python_engine.h"
#include "settings_manager.h"
#include "ui_theme_manager.h"
#include "custom_title_bar.h"
#include "repl_interface.h"
#include "variables_panel.h"
#include "layout_manager.h"
#include "tcp_server.h"
#include "debug_api.h"
#include <QApplication>
#include <QDebug>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), centralWidget(nullptr), mainLayout(nullptr) {
    
    setWindowTitle("LumosWorkspace REPL");
    setMinimumSize(800, 600);
    
    // Make window frameless for custom title bar
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    
    // Initialize the application
    initializeApplication();
}

MainWindow::~MainWindow() {
    stopServers();
    saveSettings();
}

void MainWindow::initializeApplication() {
    // Create core components first
    createComponents();
    
    // Setup UI layout
    setupLayout();
    
    // Setup connections between components
    setupConnections();
    
    // Load settings and apply theme
    loadSettings();
    applyInitialTheme();
    
    // Start network services
    startServers();
    
    qDebug() << "MainWindow initialized successfully";
}

void MainWindow::createComponents() {
    // Core engine and settings
    pythonEngine = std::make_unique<PythonEngine>();
    settingsManager = std::make_unique<SettingsManager>(this);
    
    // UI management
    themeManager = std::make_unique<UIThemeManager>(settingsManager.get(), this);
    titleBar = std::make_unique<CustomTitleBar>(this);
    
    // Main interface components
    replInterface = std::make_unique<REPLInterface>(pythonEngine.get(), this);
    variablesPanel = std::make_unique<VariablesPanel>(pythonEngine.get(), this);
    layoutManager = std::make_unique<LayoutManager>(settingsManager.get(), this);
    
    // Network services
    tcpServer = std::make_unique<TCPServer>(pythonEngine.get(), this);
    debugAPI = std::make_unique<DebugAPI>(pythonEngine.get(), this);
    
    // Initialize Python engine
    if (!pythonEngine->initialize()) {
        qCritical() << "Failed to initialize Python engine";
    }
}

void MainWindow::setupLayout() {
    // Remove default menu bar and status bar
    setMenuBar(nullptr);
    setStatusBar(nullptr);
    
    // Create central widget
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Main layout
    mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Add title bar
    mainLayout->addWidget(titleBar.get());
    
    // Create content widget for the layout manager
    QWidget* contentWidget = new QWidget();
    mainLayout->addWidget(contentWidget, 1);
    
    // Setup the layout manager with the content widget
    layoutManager->setupLayout(contentWidget, replInterface.get(), variablesPanel.get());
    
    // Setup global shortcuts
    setupShortcuts();
}

void MainWindow::setupConnections() {
    // Title bar connections
    connect(titleBar.get(), &CustomTitleBar::minimizeClicked,
            this, &QWidget::showMinimized);
    connect(titleBar.get(), &CustomTitleBar::maximizeClicked, [this]() {
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
    });
    connect(titleBar.get(), &CustomTitleBar::closeClicked,
            this, &QWidget::close);
    
    // REPL interface connections
    connect(replInterface.get(), &REPLInterface::commandExecuted,
            this, &MainWindow::onCommandExecuted);
    
    // Variables panel connections
    connect(variablesPanel.get(), &VariablesPanel::variableSelected,
            this, &MainWindow::onVariableSelected);
    
    // Layout manager connections
    connect(layoutManager.get(), &LayoutManager::layoutModeChanged,
            this, &MainWindow::onLayoutModeChanged);
    
    // Theme manager connections
    connect(themeManager.get(), &UIThemeManager::themeChanged,
            this, &MainWindow::onThemeChanged);
    
    // TCP server connections
    connect(tcpServer.get(), &TCPServer::clientConnected,
            this, &MainWindow::onTcpClientConnected);
    connect(tcpServer.get(), &TCPServer::dataReceived,
            this, &MainWindow::onTcpDataReceived);
    
    // Debug API connections
    connect(debugAPI.get(), &DebugAPI::debugCommandReceived,
            this, &MainWindow::onDebugCommandReceived);
    
    // Settings connections
    connect(settingsManager.get(), &SettingsManager::settingsLoaded, []() {
        qDebug() << "Settings loaded successfully";
    });
}

void MainWindow::setupShortcuts() {
    // Ctrl+Q to quit application (standard Qt shortcut)
    quitShortcut = new QShortcut(QKeySequence::Quit, this);
    connect(quitShortcut, &QShortcut::activated, this, &QWidget::close);
    
    // Note: Ctrl+C (Cmd+C) shortcut removed to prevent accidental application exit
    // Terminal Ctrl+C will still work when terminal has focus
}

void MainWindow::loadSettings() {
    settingsManager->loadSettings();
    
    // Restore window geometry
    int width = settingsManager->getInt("window.width", 800);
    int height = settingsManager->getInt("window.height", 600);
    int x = settingsManager->getInt("window.x", 100);
    int y = settingsManager->getInt("window.y", 100);
    bool maximized = settingsManager->getBool("window.maximized", false);
    
    resize(width, height);
    move(x, y);
    
    if (maximized) {
        showMaximized();
    }
}

void MainWindow::saveSettings() {
    if (!settingsManager) return;
    
    // Save window geometry
    settingsManager->setValue("window.width", size().width());
    settingsManager->setValue("window.height", size().height());
    settingsManager->setValue("window.x", pos().x());
    settingsManager->setValue("window.y", pos().y());
    settingsManager->setValue("window.maximized", isMaximized());
    
    // Save splitter sizes
    layoutManager->saveSplitterSizes();
    
    // Save settings to file
    settingsManager->saveSettings();
}

void MainWindow::applyInitialTheme() {
    themeManager->applyTheme(this);
}

void MainWindow::startServers() {
    // Start TCP server for data injection
    int tcpPort = settingsManager->getInt("tcp.port", 8080);
    if (!tcpServer->startServer(tcpPort)) {
        qWarning() << "Failed to start TCP server on port" << tcpPort;
    }
    
    // Start debug API server
    int debugPort = 8081; // Fixed port for debug API
    if (!debugAPI->startDebugServer(debugPort)) {
        qWarning() << "Failed to start Debug API on port" << debugPort;
    }
}

void MainWindow::stopServers() {
    if (tcpServer) {
        tcpServer->stopServer();
    }
    if (debugAPI) {
        debugAPI->stopDebugServer();
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveSettings();
    stopServers();
    event->accept();
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    
    // Auto-save window size if not maximized
    if (!isMaximized() && settingsManager) {
        settingsManager->setValue("window.width", size().width());
        settingsManager->setValue("window.height", size().height());
    }
}

void MainWindow::moveEvent(QMoveEvent* event) {
    QMainWindow::moveEvent(event);
    
    // Auto-save window position if not maximized
    if (!isMaximized() && settingsManager) {
        settingsManager->setValue("window.x", pos().x());
        settingsManager->setValue("window.y", pos().y());
    }
}

// Slot implementations
void MainWindow::onCommandExecuted(const QString& command, const QString& result) {
    Q_UNUSED(command)
    Q_UNUSED(result)
    
    // Command executed, variables might have changed
    variablesPanel->onVariablesChanged();
    
    qDebug() << "Command executed in REPL";
}

void MainWindow::onVariableSelected(const QString& name, const QString& value) {
    Q_UNUSED(value)
    
    // User selected a variable, could auto-insert into REPL input
    qDebug() << "Variable selected:" << name;
    replInterface->focusInput();
}

void MainWindow::onLayoutModeChanged(LayoutMode mode) {
    Q_UNUSED(mode)
    qDebug() << "Layout mode changed to:" << layoutManager->getCurrentModeString();
}

void MainWindow::onThemeChanged(const ThemeColors& colors) {
    Q_UNUSED(colors)
    
    // Re-apply theme to entire window
    themeManager->applyTheme(this);
    qDebug() << "Theme updated";
}

void MainWindow::onTcpClientConnected(const QString& address) {
    qDebug() << "TCP client connected from:" << address;
    
    // Update variables panel when data is injected
    variablesPanel->onVariablesChanged();
}

void MainWindow::onTcpDataReceived(const QJsonObject& data) {
    Q_UNUSED(data)
    
    // Data was injected via TCP, update variables
    variablesPanel->onVariablesChanged();
    qDebug() << "Data received via TCP";
}

void MainWindow::onDebugCommandReceived(const QString& command) {
    qDebug() << "Debug command received:" << command;
    
    // Update variables panel after debug commands that might change state
    if (command == "execute" || command == "get_variables") {
        variablesPanel->onVariablesChanged();
    }
}
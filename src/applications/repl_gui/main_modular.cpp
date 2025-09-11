#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QStyleFactory>
#include <csignal>
#include <iostream>
#include "../../modules/main_window.h"

// Global pointer to application for signal handler
QApplication* g_app = nullptr;

// Signal handler for Ctrl+C (SIGINT) and SIGTERM
void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << " - shutting down gracefully..." << std::endl;
    if (g_app) {
        g_app->quit();
    }
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    g_app = &app;
    
    // Install signal handlers for terminal Ctrl+C and SIGTERM
    // This will handle signals from terminal, not GUI shortcuts
    std::signal(SIGINT, signalHandler);   // Ctrl+C from terminal
    std::signal(SIGTERM, signalHandler);  // Termination signal
    
    // Set application properties
    app.setApplicationName("LumosWorkspace");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("LumosWorkspace");
    app.setOrganizationDomain("lumosworkspace.dev");
    
    // Ensure application data directory exists
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataPath);
    
    // Create and show main window
    MainWindow window;
    window.show();
    
    int result = app.exec();
    
    // Clean up
    g_app = nullptr;
    
    return result;
}
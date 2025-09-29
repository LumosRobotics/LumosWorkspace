#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QStandardPaths>
#include <memory>

class PythonEngine;
class SettingsManager;

class DebugAPI : public QObject {
    Q_OBJECT

public:
    explicit DebugAPI(PythonEngine* pythonEngine, SettingsManager* settingsManager, QObject* parent = nullptr);
    ~DebugAPI();
    
    bool startDebugServer(int port = 8081);
    void stopDebugServer();
    
    bool isListening() const;
    int serverPort() const;

signals:
    void debugCommandReceived(const QString& command);
    void debugResponse(const QJsonObject& response);

private slots:
    void onNewConnection();
    void onClientDataReady();
    void onClientDisconnected();

private:
    QJsonObject processDebugCommand(const QJsonObject& command);
    QJsonObject executeCommand(const QString& code);
    QJsonObject getVariables();
    QJsonObject getSystemInfo();
    QString formatPythonOutput(const QString& output);
    bool handleSpecialCommand(const QString& command);
    QString saveVariablesToPickle(const QString& customName = "", const QString& varName = "");
    QString loadVariablesFromPickle(const QString& filename);
    QString listPickleFiles() const;
    QString getDefaultPickleDirectory() const;
    QString getHelpText() const;
    
    PythonEngine* pythonEngine;
    SettingsManager* settingsManager;
    std::unique_ptr<QTcpServer> debugServer;
    QList<QTcpSocket*> debugClients;
    QString accumulatedOutput;
    QString lastSpecialCommandResult;
};
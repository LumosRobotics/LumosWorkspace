#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <memory>

class PythonEngine;

class DebugAPI : public QObject {
    Q_OBJECT

public:
    explicit DebugAPI(PythonEngine* pythonEngine, QObject* parent = nullptr);
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
    QString saveVariablesToPickle(const QString& customName = "");
    QString loadVariablesFromPickle(const QString& filename);
    QString getHelpText() const;
    
    PythonEngine* pythonEngine;
    std::unique_ptr<QTcpServer> debugServer;
    QList<QTcpSocket*> debugClients;
    QString accumulatedOutput;
    QString lastSpecialCommandResult;
};
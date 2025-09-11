#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QTimer>
#include <memory>

class PythonEngine;

class TCPServer : public QObject {
    Q_OBJECT

public:
    explicit TCPServer(PythonEngine* pythonEngine, QObject* parent = nullptr);
    ~TCPServer();
    
    bool startServer(int port = 8081);
    void stopServer();
    
    bool isListening() const;
    int serverPort() const;

signals:
    void clientConnected(const QString& address);
    void clientDisconnected(const QString& address);
    void dataReceived(const QJsonObject& data);
    void errorOccurred(const QString& error);

private slots:
    void onNewConnection();
    void onClientDataReady();
    void onClientDisconnected();
    void onServerError();

private:
    void processClientMessage(QTcpSocket* client, const QByteArray& data);
    QJsonObject handleDataInjection(const QJsonObject& message);
    QJsonObject createResponse(bool success, const QString& message = "", const QJsonObject& data = QJsonObject());
    void injectPythonVariable(const QString& name, const QJsonObject& data);
    
    PythonEngine* pythonEngine;
    std::unique_ptr<QTcpServer> server;
    QList<QTcpSocket*> clients;
    QTimer* heartbeatTimer;
};
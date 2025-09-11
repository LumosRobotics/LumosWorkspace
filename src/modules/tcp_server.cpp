#include "tcp_server.h"
#include "python_engine.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QHostAddress>
#include <QDebug>

TCPServer::TCPServer(PythonEngine* pythonEngine, QObject* parent)
    : QObject(parent), pythonEngine(pythonEngine) {
    
    server = std::make_unique<QTcpServer>(this);
    
    connect(server.get(), &QTcpServer::newConnection,
            this, &TCPServer::onNewConnection);
    connect(server.get(), &QTcpServer::acceptError,
            this, &TCPServer::onServerError);
    
    // Heartbeat timer for cleanup
    heartbeatTimer = new QTimer(this);
    heartbeatTimer->setInterval(30000); // 30 seconds
    connect(heartbeatTimer, &QTimer::timeout, [this]() {
        // Clean up disconnected clients
        clients.removeAll(nullptr);
        for (auto it = clients.begin(); it != clients.end();) {
            if ((*it)->state() == QAbstractSocket::UnconnectedState) {
                (*it)->deleteLater();
                it = clients.erase(it);
            } else {
                ++it;
            }
        }
    });
}

TCPServer::~TCPServer() {
    stopServer();
}

bool TCPServer::startServer(int port) {
    if (server->isListening()) {
        stopServer();
    }
    
    if (server->listen(QHostAddress::LocalHost, port)) {
        heartbeatTimer->start();
        qDebug() << "TCP Server started on port" << port;
        return true;
    } else {
        emit errorOccurred(QString("Failed to start TCP server: %1").arg(server->errorString()));
        return false;
    }
}

void TCPServer::stopServer() {
    if (server->isListening()) {
        server->close();
        heartbeatTimer->stop();
        
        // Close all client connections
        for (QTcpSocket* client : clients) {
            if (client) {
                client->disconnectFromHost();
                client->deleteLater();
            }
        }
        clients.clear();
        
        qDebug() << "TCP Server stopped";
    }
}

bool TCPServer::isListening() const {
    return server->isListening();
}

int TCPServer::serverPort() const {
    return server->serverPort();
}

void TCPServer::onNewConnection() {
    while (server->hasPendingConnections()) {
        QTcpSocket* client = server->nextPendingConnection();
        clients.append(client);
        
        connect(client, &QTcpSocket::readyRead,
                this, &TCPServer::onClientDataReady);
        connect(client, &QTcpSocket::disconnected,
                this, &TCPServer::onClientDisconnected);
        
        QString clientAddress = client->peerAddress().toString();
        emit clientConnected(clientAddress);
        qDebug() << "Client connected:" << clientAddress;
    }
}

void TCPServer::onClientDataReady() {
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;
    
    QByteArray data = client->readAll();
    processClientMessage(client, data);
}

void TCPServer::onClientDisconnected() {
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;
    
    QString clientAddress = client->peerAddress().toString();
    clients.removeAll(client);
    client->deleteLater();
    
    emit clientDisconnected(clientAddress);
    qDebug() << "Client disconnected:" << clientAddress;
}

void TCPServer::onServerError() {
    emit errorOccurred(server->errorString());
}

void TCPServer::processClientMessage(QTcpSocket* client, const QByteArray& data) {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        QJsonObject response = createResponse(false, "Invalid JSON: " + error.errorString());
        client->write(QJsonDocument(response).toJson(QJsonDocument::Compact));
        return;
    }
    
    QJsonObject message = doc.object();
    emit dataReceived(message);
    
    // Handle the message
    QJsonObject response;
    QString command = message["command"].toString();
    
    if (command == "inject_data") {
        response = handleDataInjection(message);
    } else {
        response = createResponse(false, "Unknown command: " + command);
    }
    
    // Send response
    client->write(QJsonDocument(response).toJson(QJsonDocument::Compact));
}

QJsonObject TCPServer::handleDataInjection(const QJsonObject& message) {
    QString variableName = message["name"].toString();
    QJsonObject data = message["data"].toObject();
    QString dataType = message["type"].toString();
    
    if (variableName.isEmpty()) {
        return createResponse(false, "Variable name is required");
    }
    
    if (!pythonEngine || !pythonEngine->isInitialized()) {
        return createResponse(false, "Python engine not initialized");
    }
    
    try {
        injectPythonVariable(variableName, data);
        return createResponse(true, "Data injected successfully");
    } catch (const std::exception& e) {
        return createResponse(false, QString("Injection failed: %1").arg(e.what()));
    } catch (...) {
        return createResponse(false, "Injection failed: Unknown error");
    }
}

void TCPServer::injectPythonVariable(const QString& name, const QJsonObject& data) {
    // Thread-safe Python operations
    pythonEngine->acquireGIL();
    
    try {
        // Convert JSON to Python object based on type
        QString pythonCode;
        
        if (data.contains("list")) {
            // Handle list data
            QJsonArray array = data["list"].toArray();
            QStringList items;
            for (const QJsonValue& value : array) {
                if (value.isString()) {
                    items.append("'" + value.toString() + "'");
                } else {
                    items.append(QString::number(value.toDouble()));
                }
            }
            pythonCode = name + " = [" + items.join(", ") + "]";
        } else if (data.contains("dict")) {
            // Handle dictionary data
            QJsonObject dict = data["dict"].toObject();
            QStringList items;
            for (auto it = dict.begin(); it != dict.end(); ++it) {
                QString key = "'" + it.key() + "'";
                QString value = it.value().isString() ? 
                    "'" + it.value().toString() + "'" : 
                    QString::number(it.value().toDouble());
                items.append(key + ": " + value);
            }
            pythonCode = name + " = {" + items.join(", ") + "}";
        } else if (data.contains("string")) {
            // Handle string data
            pythonCode = name + " = '" + data["string"].toString() + "'";
        } else if (data.contains("number")) {
            // Handle numeric data
            pythonCode = name + " = " + QString::number(data["number"].toDouble());
        } else {
            throw std::runtime_error("Unsupported data type");
        }
        
        // Execute the Python code to create the variable
        std::string result = pythonEngine->evaluateExpression(pythonCode.toStdString());
        if (result.find("Error:") == 0) {
            throw std::runtime_error(result);
        }
        
    } catch (...) {
        pythonEngine->releaseGIL();
        throw;
    }
    
    pythonEngine->releaseGIL();
}

QJsonObject TCPServer::createResponse(bool success, const QString& message, const QJsonObject& data) {
    QJsonObject response;
    response["success"] = success;
    if (!message.isEmpty()) {
        response["message"] = message;
    }
    if (!data.isEmpty()) {
        response["data"] = data;
    }
    return response;
}
#include "debug_api.h"
#include "python_engine.h"
#include "settings_manager.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QHostAddress>
#include <QDebug>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>

DebugAPI::DebugAPI(PythonEngine* pythonEngine, SettingsManager* settingsManager, QObject* parent)
    : QObject(parent), pythonEngine(pythonEngine), settingsManager(settingsManager) {
    
    debugServer = std::make_unique<QTcpServer>(this);
    
    connect(debugServer.get(), &QTcpServer::newConnection,
            this, &DebugAPI::onNewConnection);
}

DebugAPI::~DebugAPI() {
    stopDebugServer();
}

bool DebugAPI::startDebugServer(int port) {
    if (debugServer->isListening()) {
        stopDebugServer();
    }
    
    if (debugServer->listen(QHostAddress::LocalHost, port)) {
        qDebug() << "Debug API started on port" << port;
        return true;
    } else {
        qWarning() << "Failed to start debug API:" << debugServer->errorString();
        return false;
    }
}

void DebugAPI::stopDebugServer() {
    if (debugServer->isListening()) {
        debugServer->close();
        
        // Close all debug client connections
        for (QTcpSocket* client : debugClients) {
            if (client) {
                client->disconnectFromHost();
                client->deleteLater();
            }
        }
        debugClients.clear();
        
        qDebug() << "Debug API stopped";
    }
}

bool DebugAPI::isListening() const {
    return debugServer->isListening();
}

int DebugAPI::serverPort() const {
    return debugServer->serverPort();
}

void DebugAPI::onNewConnection() {
    while (debugServer->hasPendingConnections()) {
        QTcpSocket* client = debugServer->nextPendingConnection();
        debugClients.append(client);
        
        connect(client, &QTcpSocket::readyRead,
                this, &DebugAPI::onClientDataReady);
        connect(client, &QTcpSocket::disconnected,
                this, &DebugAPI::onClientDisconnected);
        
        qDebug() << "Debug client connected:" << client->peerAddress().toString();
    }
}

void DebugAPI::onClientDataReady() {
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;
    
    QByteArray data = client->readAll();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        QJsonObject response;
        response["status"] = "error";
        response["message"] = "Invalid JSON: " + error.errorString();
        client->write(QJsonDocument(response).toJson(QJsonDocument::Compact));
        return;
    }
    
    QJsonObject command = doc.object();
    emit debugCommandReceived(command["command"].toString());
    
    QJsonObject response = processDebugCommand(command);
    emit debugResponse(response);
    
    client->write(QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void DebugAPI::onClientDisconnected() {
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;
    
    debugClients.removeAll(client);
    client->deleteLater();
    
    qDebug() << "Debug client disconnected";
}

QJsonObject DebugAPI::processDebugCommand(const QJsonObject& command) {
    QString cmd = command["command"].toString();
    
    if (cmd == "execute") {
        QString code = command["code"].toString();
        return executeCommand(code);
    } else if (cmd == "get_variables") {
        return getVariables();
    } else if (cmd == "get_system_info") {
        return getSystemInfo();
    } else if (cmd == "ping") {
        QJsonObject response;
        response["status"] = "success";
        response["message"] = "pong";
        return response;
    } else {
        QJsonObject response;
        response["status"] = "error";
        response["message"] = "Unknown command: " + cmd;
        return response;
    }
}

QJsonObject DebugAPI::executeCommand(const QString& code) {
    QJsonObject response;
    
    if (!pythonEngine || !pythonEngine->isInitialized()) {
        response["status"] = "error";
        response["message"] = "Python engine not initialized";
        return response;
    }
    
    // Accumulate output for multi-line commands
    accumulatedOutput.clear();
    
    // Split code into lines and execute each
    QStringList lines = code.split('\n', Qt::SkipEmptyParts);
    QString output;
    
    for (const QString& line : lines) {
        QString trimmedLine = line.trimmed();
        if (trimmedLine.isEmpty()) continue;
        
        QString result;
        
        // Check for special commands first
        if (handleSpecialCommand(trimmedLine)) {
            // Special command was handled, get the result from lastSpecialCommandResult
            result = lastSpecialCommandResult;
        } else {
            // Regular Python command
            result = QString::fromStdString(pythonEngine->evaluateExpression(trimmedLine.toStdString()));
        }
        
        // Format output similar to REPL
        if (lines.size() == 1) {
            // Single command
            output += ">>> " + trimmedLine + "\n";
        } else {
            // Multi-line - show the command structure
            if (!output.isEmpty()) output += "\n";
            output += ">>> " + trimmedLine + "\n";
        }
        
        if (!result.isEmpty()) {
            output += result + "\n";
        }
    }
    
    response["status"] = "success";
    response["result"] = formatPythonOutput(output);
    return response;
}

QJsonObject DebugAPI::getVariables() {
    QJsonObject response;
    
    if (!pythonEngine || !pythonEngine->isInitialized()) {
        response["status"] = "error";
        response["message"] = "Python engine not initialized";
        return response;
    }
    
    std::vector<PythonVariable> variables = pythonEngine->getUserVariables();
    
    QJsonArray varsArray;
    for (const PythonVariable& var : variables) {
        QJsonObject varObj;
        varObj["name"] = QString::fromStdString(var.name);
        varObj["type"] = QString::fromStdString(var.type);
        varObj["value"] = QString::fromStdString(var.value);
        varObj["display"] = QString::fromStdString(var.displayString);
        varsArray.append(varObj);
    }
    
    response["status"] = "success";
    response["variables"] = varsArray;
    return response;
}

QJsonObject DebugAPI::getSystemInfo() {
    QJsonObject response;
    QJsonObject sysInfo;
    
    sysInfo["application"] = QCoreApplication::applicationName();
    sysInfo["version"] = QCoreApplication::applicationVersion();
    sysInfo["qt_version"] = qVersion();
    sysInfo["python_initialized"] = pythonEngine && pythonEngine->isInitialized();
    
    response["status"] = "success";
    response["system_info"] = sysInfo;
    return response;
}

QString DebugAPI::formatPythonOutput(const QString& output) {
    return output.trimmed();
}

bool DebugAPI::handleSpecialCommand(const QString& command) {
    QString cmd = command.toLower().trimmed();
    
    if (cmd == "clear") {
        lastSpecialCommandResult = "Output cleared";
        return true;
    }
    else if (cmd == "clear vars") {
        // Clear Python variables
        if (pythonEngine && pythonEngine->isInitialized()) {
            auto variables = pythonEngine->getUserVariables();
            for (const auto& var : variables) {
                QString deleteCmd = QString("del %1").arg(QString::fromStdString(var.name));
                pythonEngine->evaluateExpression(deleteCmd.toStdString());
            }
        }
        lastSpecialCommandResult = "Variables cleared";
        return true;
    }
    else if (cmd == "save" || cmd.startsWith("save ")) {
        // Parse save command arguments
        QString args = cmd.startsWith("save ") ? cmd.mid(5).trimmed() : "";
        QStringList parts = args.split(' ', Qt::SkipEmptyParts);
        
        QString varName = "";
        QString fileName = "";
        
        if (parts.size() == 0) {
            // "save" - save all variables with timestamp
        } else if (parts.size() == 1) {
            // Could be "save filename" or "save varname"
            // Check if it's a valid variable name by checking if it exists
            QString checkCode = QString("'%1' in globals()").arg(parts[0]);
            std::string exists = pythonEngine->evaluateExpression(checkCode.toStdString());
            if (exists == "True") {
                // It's a variable name
                varName = parts[0];
            } else {
                // It's a filename
                fileName = parts[0];
            }
        } else if (parts.size() == 2) {
            // "save varname filename"
            varName = parts[0];
            fileName = parts[1];
        }
        
        lastSpecialCommandResult = saveVariablesToPickle(fileName, varName);
        return true;
    }
    else if (cmd == "load" || cmd.startsWith("load ")) {
        if (cmd == "load") {
            lastSpecialCommandResult = "Error: Please specify a filename";
        } else {
            QString filename = cmd.mid(5).trimmed(); // Extract filename after "load "
            if (!filename.isEmpty()) {
                lastSpecialCommandResult = loadVariablesFromPickle(filename);
            } else {
                lastSpecialCommandResult = "Error: Please specify a filename";
            }
        }
        return true;
    }
    else if (cmd == "ls") {
        lastSpecialCommandResult = listPickleFiles();
        return true;
    }
    else if (cmd == "help") {
        lastSpecialCommandResult = getHelpText();
        return true;
    }
    
    return false;
}

QString DebugAPI::getDefaultPickleDirectory() const
{
    // First priority: Check for custom data_dir in configuration
    if (settingsManager && settingsManager->contains("data_dir")) {
        QString customDir = settingsManager->getString("data_dir");
        if (!customDir.isEmpty()) {
            // Test if we can create/access this directory
            QDir dir(customDir);
            if (dir.exists() || dir.mkpath(customDir)) {
                // Test write permissions by creating a temporary test file
                QString testFile = customDir + "/.write_test";
                QFile file(testFile);
                if (file.open(QIODevice::WriteOnly)) {
                    file.close();
                    file.remove(); // Clean up test file
                    return customDir;
                }
            }
            // Custom directory failed, continue to fallbacks
        }
    }
    
    // Second priority: Try Documents directory (preferred location)
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (!documentsPath.isEmpty()) {
        QString preferredDir = documentsPath + "/LumosWorkspace";
        
        // Test if we can create/access this directory
        QDir dir(preferredDir);
        if (dir.exists() || dir.mkpath(preferredDir)) {
            // Test write permissions by creating a temporary test file
            QString testFile = preferredDir + "/.write_test";
            QFile file(testFile);
            if (file.open(QIODevice::WriteOnly)) {
                file.close();
                file.remove(); // Clean up test file
                return preferredDir;
            }
        }
    }
    
    // Final fallback: /tmp/LumosWorkspace
    return "/tmp/LumosWorkspace";
}

QString DebugAPI::saveVariablesToPickle(const QString& customName, const QString& varName) {
    if (!pythonEngine || !pythonEngine->isInitialized()) {
        return "Error: Python engine not initialized";
    }

    // Get default pickle directory
    QString pickleDir = getDefaultPickleDirectory();
    
    // Ensure directory exists
    QDir dir(pickleDir);
    if (!dir.exists()) {
        if (!dir.mkpath(pickleDir)) {
            return QString("Error: Could not create directory %1").arg(pickleDir);
        }
    }
    
    // Determine filename
    QString filename;
    if (customName.isEmpty()) {
        // Use timestamp with variable-specific prefix if saving single variable
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        if (varName.isEmpty()) {
            filename = QString("saved_variables_%1.pickle").arg(timestamp);
        } else {
            filename = QString("saved_%1_%2.pickle").arg(varName).arg(timestamp);
        }
    } else {
        // Use custom name, ensure .pickle extension
        filename = customName;
        if (!filename.endsWith(".pickle")) {
            filename += ".pickle";
        }
    }
    
    QString fullPath = pickleDir + "/" + filename;
    
    // Execute save operation step by step to avoid compilation issues
    pythonEngine->evaluateExpression("import pickle, os");
    pythonEngine->evaluateExpression("globals_snapshot = dict(globals())");
    
    QString filterCode;
    if (varName.isEmpty()) {
        // Save all user variables
        filterCode = QString(R"(
user_vars = {}
for name, value in globals_snapshot.items():
    if not name.startswith('__') and name not in ['pickle', 'os', 'user_vars', 'globals_snapshot', 'name', 'value', 'f', 'saved_count', 'result_message']:
        try:
            user_vars[name] = value
        except:
            pass
)");
    } else {
        // Save only the specified variable
        filterCode = QString(R"(
user_vars = {}
if '%1' in globals_snapshot:
    try:
        user_vars['%1'] = globals_snapshot['%1']
    except:
        pass
)").arg(varName);
    }
    pythonEngine->evaluateExpression(filterCode.toStdString());
    
    QString saveCode;
    if (varName.isEmpty()) {
        saveCode = QString(R"(
try:
    with open(r'%1', 'wb') as f:
        pickle.dump(user_vars, f)
    saved_count = len(user_vars)
    result_message = f'Saved {saved_count} variables to %2'
except Exception as e:
    result_message = f'Error saving variables: {str(e)}'
)").arg(fullPath).arg(filename);
    } else {
        saveCode = QString(R"(
try:
    with open(r'%1', 'wb') as f:
        pickle.dump(user_vars, f)
    if len(user_vars) > 0:
        result_message = f'Saved variable "%3" to %2'
    else:
        result_message = f'Error: Variable "%3" not found'
except Exception as e:
    result_message = f'Error saving variable: {str(e)}'
)").arg(fullPath).arg(filename).arg(varName);
    }
    
    pythonEngine->evaluateExpression(saveCode.toStdString());
    std::string result = pythonEngine->evaluateExpression("result_message");
    
    // Clean up variables that were created during save operation
    pythonEngine->evaluateExpression(R"(
try:
    del pickle, os, user_vars, globals_snapshot, name, value, f, saved_count, result_message
except:
    pass
)");
    
    if (result.empty()) {
        return QString("Variables saved to %1").arg(filename);
    }
    
    return QString::fromStdString(result);
}

QString DebugAPI::loadVariablesFromPickle(const QString& filename) {
    if (!pythonEngine || !pythonEngine->isInitialized()) {
        return "Error: Python engine not initialized";
    }

    // Get default pickle directory
    QString pickleDir = getDefaultPickleDirectory();
    
    // Determine full file path
    QString actualFilename = filename;
    if (!actualFilename.endsWith(".pickle")) {
        actualFilename += ".pickle";
    }
    
    QString fullPath = pickleDir + "/" + actualFilename;
    
    // Create Python code to load variables from pickle
    QString pythonCode = QString(R"(
import pickle
import os

# Check if file exists
if not os.path.exists(r'%1'):
    result_message = 'Error: File not found: %2'
else:
    try:
        # Load variables from pickle file
        with open(r'%1', 'rb') as f:
            loaded_vars = pickle.load(f)
        
        # Add variables to global namespace
        loaded_count = 0
        if isinstance(loaded_vars, dict):
            for var_name, var_value in loaded_vars.items():
                if not var_name.startswith('__'):
                    globals()[var_name] = var_value
                    loaded_count += 1
            
            result_message = f'Loaded {loaded_count} variables from %2'
        else:
            result_message = 'Error: Pickle file does not contain a dictionary'
            
    except Exception as e:
        result_message = f'Error loading variables: {str(e)}'

print(result_message)
)").arg(fullPath).arg(actualFilename);

    // Execute the Python code
    std::string result = pythonEngine->evaluateExpression(pythonCode.toStdString());
    
    // Clean up variables that were created during load operation (but keep loaded user variables)
    pythonEngine->evaluateExpression(R"(
try:
    del pickle, os, loaded_vars, loaded_count, var_name, var_value, result_message, f
except:
    pass
)");
    
    if (result.empty()) {
        return QString("Variables loaded from %1").arg(actualFilename);
    }
    
    return QString::fromStdString(result);
}

QString DebugAPI::listPickleFiles() const
{
    QString pickleDir = getDefaultPickleDirectory();
    
    // Check if directory exists
    QDir dir(pickleDir);
    if (!dir.exists()) {
        return QString("No data directory found at: %1").arg(pickleDir);
    }
    
    // Get all .pickle files in the directory
    QStringList nameFilters;
    nameFilters << "*.pickle";
    QFileInfoList fileList = dir.entryInfoList(nameFilters, QDir::Files, QDir::Time | QDir::Reversed);
    
    if (fileList.isEmpty()) {
        return QString("No pickle files found in: %1").arg(pickleDir);
    }
    
    // Format the file list
    QString result = QString("Pickle files in %1:\n").arg(pickleDir);
    for (const QFileInfo& fileInfo : fileList) {
        QString fileName = fileInfo.fileName();
        QString fileSize;
        
        qint64 size = fileInfo.size();
        if (size < 1024) {
            fileSize = QString("%1 B").arg(size);
        } else if (size < 1024 * 1024) {
            fileSize = QString("%.1f KB").arg(size / 1024.0);
        } else {
            fileSize = QString("%.1f MB").arg(size / (1024.0 * 1024.0));
        }
        
        QString lastModified = fileInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss");
        result += QString("  %1 (%2, %3)\n").arg(fileName, fileSize, lastModified);
    }
    
    return result.trimmed(); // Remove trailing newline
}

QString DebugAPI::getHelpText() const {
    return QString(R"(
LumosWorkspace REPL - Help & Commands
=====================================

🐍 PYTHON COMMANDS:
  help()              - Python help (limited in embedded environment)
                       Try: print(obj.__doc__) or dir(obj) instead
  Any Python code     - Execute Python expressions and statements
  
⌨️  KEYBOARD SHORTCUTS:
  Enter               - Execute command
  Shift+Enter         - Multi-line input (new line)
  Up/Down arrows      - Navigate command history
  
🔧 SPECIAL COMMANDS:
  help                - Show this help message
  clear               - Clear REPL output (keep variables)
  clear vars          - Clear all Python variables from memory
  
💾 VARIABLE PERSISTENCE:
  save [name]         - Save all variables to pickle file
                       'save' → saved_variables_TIMESTAMP.pickle
                       'save my_data' → my_data.pickle
                       
  load filename       - Load variables from pickle file
                       'load my_data' → loads my_data.pickle
                       'load data.pickle' → loads data.pickle
                       
  ls                  - List all pickle files in data directory
                       Shows filename, size, and modification date
  
📝 EXAMPLES:
  >>> x = 42                    # Create variable
  >>> save session1            # Save to session1.pickle
  >>> clear vars               # Clear all variables
  >>> load session1            # Restore variables
  >>> print(x)                 # Variable restored: 42
  
  >>> print(len.__doc__)        # Get function documentation  
  >>> dir(math)                # List module contents
  >>> help                     # This help message (no parentheses)
  
🎯 TIP: Use 'clear' to clean output, 'clear vars' to reset variables!
)");
}
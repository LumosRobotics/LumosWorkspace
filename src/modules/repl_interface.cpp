#include "repl_interface.h"
#include "settings_manager.h"
#include <QApplication>
#include <QScrollBar>
#include <QFont>
#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QDebug>

REPLInterface::REPLInterface(PythonEngine *pythonEngine, SettingsManager *settingsManager, QWidget *parent)
    : QWidget(parent), pythonEngine(pythonEngine), settingsManager(settingsManager), currentLayoutMode("bottom_input"),
      historyIndex(-1), executingCommand(false), filePickerMode(false), selectedFileIndex(0)
{
    setupUI();
    setupConnections();
    setupEventFilters();
}

void REPLInterface::setupUI()
{
    setObjectName("REPLInterface");

    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    // Output area
    outputArea = new QTextEdit(this);
    outputArea->setObjectName("REPLOutput");
    outputArea->setReadOnly(true);
    outputArea->setFont(QFont("Monaco", 12));

    // Input container
    inputContainer = new QWidget(this);
    inputLayout = new QHBoxLayout(inputContainer);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(5);

    // Input area (using QTextEdit for multi-line support)
    inputArea = new QTextEdit(inputContainer);
    inputArea->setObjectName("REPLInput");
    inputArea->setFont(QFont("Monaco", 12));
    inputArea->setMaximumHeight(100);
    inputArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    inputArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Input layout - clean input area without prompt label
    inputLayout->addWidget(inputArea, 1);

    // Main layout
    mainLayout->addWidget(outputArea, 1);
    mainLayout->addWidget(inputContainer);
}

void REPLInterface::setupConnections()
{
    // No button connections needed - using keyboard shortcuts only
}

void REPLInterface::setupEventFilters()
{
    inputArea->installEventFilter(this);
}

bool REPLInterface::eventFilter(QObject *obj, QEvent *event)
{
    // Handle file picker mode with application-level event filter
    if (filePickerMode && event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        return handleKeyPress(keyEvent);
    }
    
    // Handle normal input area events
    if (obj == inputArea && event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        return handleKeyPress(keyEvent);
    }
    
    return QWidget::eventFilter(obj, event);
}

bool REPLInterface::handleKeyPress(QKeyEvent *keyEvent)
{
    // Handle file picker mode
    if (filePickerMode) {
        qDebug() << "File picker mode - Key pressed:" << keyEvent->key();
        
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            qDebug() << "Enter pressed - confirming selection";
            confirmFileSelection();
            return true;
        }
        else if (keyEvent->key() == Qt::Key_Up && keyEvent->modifiers() == Qt::NoModifier) {
            qDebug() << "Up arrow pressed - moving up";
            selectFile(-1); // Move up in file list
            return true;
        }
        else if (keyEvent->key() == Qt::Key_Down && keyEvent->modifiers() == Qt::NoModifier) {
            qDebug() << "Down arrow pressed - moving down";
            selectFile(1); // Move down in file list
            return true;
        }
        else if (keyEvent->key() == Qt::Key_Escape) {
            qDebug() << "Escape pressed - canceling";
            cancelFilePicker();
            return true;
        }
        // In file picker mode, ignore all other keys
        qDebug() << "Other key ignored in file picker mode";
        return true;
    }
    
    // Normal mode handling
    if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
    {
        if (keyEvent->modifiers() & Qt::ShiftModifier)
        {
            // Shift+Enter: Allow multi-line input
            return false; // Let QTextEdit handle the newline
        }
        else
        {
            // Enter alone: Execute command
            executeCommand();
            keyEvent->accept();
            return true; // Event handled, don't let QTextEdit process it
        }
    }
    else if (keyEvent->key() == Qt::Key_Up && keyEvent->modifiers() == Qt::NoModifier)
    {
        navigateHistory(-1);
        return true; // Event handled
    }
    else if (keyEvent->key() == Qt::Key_Down && keyEvent->modifiers() == Qt::NoModifier)
    {
        navigateHistory(1);
        return true; // Event handled
    }
    
    return false; // Let other events be processed normally
}

void REPLInterface::executeCommand()
{
    if (executingCommand)
        return;

    QString command = inputArea->toPlainText().trimmed();
    if (command.isEmpty())
        return;

    executingCommand = true;

    // Add to history
    addToHistory(command);

    // Clear input
    inputArea->clear();

    // Check for special commands first
    if (handleSpecialCommand(command))
    {
        executingCommand = false;
        return;
    }

    // Show command in output
    appendOutput(formatPrompt(command));

    // Execute command
    if (pythonEngine && pythonEngine->isInitialized())
    {
        QString result = QString::fromStdString(pythonEngine->evaluateExpression(command.toStdString()));

        // Show result if not empty
        if (!result.isEmpty())
        {
            appendOutput(formatResult(result));
        }

        emit commandExecuted(command, result);
    }
    else
    {
        QString error = "Error: Python engine not initialized";
        appendOutput(formatResult(error));
        emit commandExecuted(command, error);
    }

    appendOutput(""); // Empty line for spacing
    executingCommand = false;
}

void REPLInterface::addToHistory(const QString &command)
{
    if (!command.isEmpty() && (commandHistory.isEmpty() || commandHistory.last() != command))
    {
        commandHistory.append(command);

        // Limit history size
        if (commandHistory.size() > 100)
        {
            commandHistory.removeFirst();
        }
    }
    historyIndex = commandHistory.size();
}

void REPLInterface::navigateHistory(int direction)
{
    if (commandHistory.isEmpty())
        return;

    historyIndex += direction;

    if (historyIndex < 0)
    {
        historyIndex = 0;
    }
    else if (historyIndex >= commandHistory.size())
    {
        historyIndex = commandHistory.size();
        inputArea->clear();
        return;
    }

    if (historyIndex < commandHistory.size())
    {
        inputArea->setPlainText(commandHistory[historyIndex]);

        // Move cursor to end
        QTextCursor cursor = inputArea->textCursor();
        cursor.movePosition(QTextCursor::End);
        inputArea->setTextCursor(cursor);
    }
}

bool REPLInterface::handleSpecialCommand(const QString &command)
{
    QString cmd = command.toLower().trimmed();

    if (cmd == "clear")
    {
        // Clear REPL output completely and show just empty prompt
        clearOutput();
        appendOutput(">>> ");
        emit commandExecuted(command, "Output cleared");
        return true;
    }
    else if (cmd == "clear vars")
    {
        // Clear Python variables silently
        clearVariables();
        emit commandExecuted(command, "Variables cleared");
        return true;
    }
    else if (cmd == "save" || cmd.startsWith("save "))
    {
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
        
        QString result = saveVariablesToPickle(fileName, varName);
        appendOutput(formatResult(result));
        emit commandExecuted(command, result);
        return true;
    }
    else if (cmd == "load" || cmd.startsWith("load "))
    {
        // Load variables from pickle file
        if (cmd == "load") {
            // Start interactive file picker
            startFilePicker();
            return true;
        } else {
            QString filename = cmd.mid(5).trimmed(); // Extract filename after "load "
            if (!filename.isEmpty()) {
                QString result = loadVariablesFromPickle(filename);
                appendOutput(formatResult(result));
                emit commandExecuted(command, result);
                return true;
            } else {
                QString error = "Error: Please specify a filename";
                appendOutput(formatResult(error));
                emit commandExecuted(command, error);
                return true;
            }
        }
    }
    else if (cmd == "ls")
    {
        // List pickle files in current data directory
        QString result = listPickleFiles();
        appendOutput(formatResult(result));
        emit commandExecuted(command, result);
        return true;
    }
    else if (cmd == "help")
    {
        // Show help instructions
        QString helpText = getHelpText();
        appendOutput(formatResult(helpText));
        emit commandExecuted(command, helpText);
        return true;
    }

    return false; // Not a special command
}

void REPLInterface::clearVariables()
{
    if (!pythonEngine || !pythonEngine->isInitialized())
    {
        return;
    }

    // Get all user variables and delete them
    auto variables = pythonEngine->getUserVariables();

    for (const auto &var : variables)
    {
        QString deleteCmd = QString("del %1").arg(QString::fromStdString(var.name));
        pythonEngine->evaluateExpression(deleteCmd.toStdString());
    }
}

QString REPLInterface::getDefaultPickleDirectory() const
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

QString REPLInterface::saveVariablesToPickle(const QString& customName, const QString& varName)
{
    if (!pythonEngine || !pythonEngine->isInitialized())
    {
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

QString REPLInterface::loadVariablesFromPickle(const QString& filename)
{
    if (!pythonEngine || !pythonEngine->isInitialized())
    {
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

QString REPLInterface::listPickleFiles() const
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

void REPLInterface::startFilePicker()
{
    QString pickleDir = getDefaultPickleDirectory();
    QDir dir(pickleDir);
    
    if (!dir.exists()) {
        QString error = QString("No data directory found at: %1").arg(pickleDir);
        appendOutput(formatResult(error));
        return;
    }
    
    // Get all .pickle files
    QStringList nameFilters;
    nameFilters << "*.pickle";
    QFileInfoList fileInfoList = dir.entryInfoList(nameFilters, QDir::Files, QDir::Time | QDir::Reversed);
    
    availableFiles.clear();
    for (const QFileInfo& info : fileInfoList) {
        availableFiles.append(info.fileName());
    }
    
    if (availableFiles.isEmpty()) {
        QString error = QString("No pickle files found in: %1").arg(pickleDir);
        appendOutput(formatResult(error));
        return;
    }
    
    // Enter file picker mode
    filePickerMode = true;
    selectedFileIndex = 0;
    
    // Clear input area and make it read-only to prevent text entry
    inputArea->clear();
    inputArea->setReadOnly(true);
    inputArea->setPlaceholderText("Use ↑↓ arrows to navigate, Enter to select, Esc to cancel");
    
    // Install event filter on the application to catch all key events
    QApplication::instance()->installEventFilter(this);
    
    // Set focus to ensure we capture key events
    inputArea->setFocus();
    
    // Display file picker
    updateFilePickerDisplay();
}

void REPLInterface::updateFilePickerDisplay()
{
    if (!filePickerMode || availableFiles.isEmpty()) {
        return;
    }
    
    // Build the file picker display
    QString display = "📂 Select a file to load (↑↓ to navigate, Enter to select, Esc to cancel):\n\n";
    
    for (int i = 0; i < availableFiles.size(); ++i) {
        QString prefix = (i == selectedFileIndex) ? "► " : "  ";
        display += QString("%1%2\n").arg(prefix, availableFiles[i]);
    }
    
    // Update the output area
    appendOutput(display);
    
    // Scroll to the bottom to show the picker
    if (outputArea) {
        QScrollBar* scrollBar = outputArea->verticalScrollBar();
        scrollBar->setValue(scrollBar->maximum());
    }
}

void REPLInterface::selectFile(int direction)
{
    if (!filePickerMode || availableFiles.isEmpty()) {
        return;
    }
    
    // Update selected index
    selectedFileIndex += direction;
    
    // Wrap around
    if (selectedFileIndex < 0) {
        selectedFileIndex = availableFiles.size() - 1;
    } else if (selectedFileIndex >= availableFiles.size()) {
        selectedFileIndex = 0;
    }
    
    // Clear the previous display and show updated one
    // Remove the last file picker display from output
    QString currentText = outputArea->toPlainText();
    int lastPickerStart = currentText.lastIndexOf("📂 Select a file to load");
    if (lastPickerStart != -1) {
        outputArea->setPlainText(currentText.left(lastPickerStart));
    }
    
    // Show updated picker
    updateFilePickerDisplay();
}

void REPLInterface::confirmFileSelection()
{
    if (!filePickerMode || availableFiles.isEmpty() || selectedFileIndex < 0 || selectedFileIndex >= availableFiles.size()) {
        return;
    }
    
    QString selectedFile = availableFiles[selectedFileIndex];
    
    // Exit file picker mode
    cancelFilePicker();
    
    // Load the selected file
    QString result = loadVariablesFromPickle(selectedFile);
    appendOutput(formatResult(result));
    emit commandExecuted("load", result);
}

void REPLInterface::cancelFilePicker()
{
    if (!filePickerMode) {
        return;
    }
    
    // Exit file picker mode
    filePickerMode = false;
    availableFiles.clear();
    selectedFileIndex = 0;
    
    // Remove application event filter
    QApplication::instance()->removeEventFilter(this);
    
    // Restore input area to normal state
    inputArea->setReadOnly(false);
    inputArea->setPlaceholderText("");
    
    // Show prompt
    appendOutput(">>> ");
}

QString REPLInterface::getHelpText() const
{
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
                       
  load [filename]     - Load variables from pickle file
                       'load' → interactive file picker with ↑↓ navigation
                       'load my_data' → loads my_data.pickle directly
                       
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

void REPLInterface::setLayoutMode(const QString &mode)
{
    currentLayoutMode = mode;
}

void REPLInterface::focusInput()
{
    inputArea->setFocus();
}

void REPLInterface::clearOutput()
{
    outputArea->clear();
}

void REPLInterface::appendOutput(const QString &text)
{
    outputArea->append(text);

    // Auto-scroll to bottom
    QScrollBar *scrollBar = outputArea->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

QString REPLInterface::formatPrompt(const QString &command) const
{
    // Handle multi-line commands
    QStringList lines = command.split('\n');
    QString formatted;

    for (int i = 0; i < lines.size(); ++i)
    {
        if (i == 0)
        {
            formatted += ">>> " + lines[i];
        }
        else
        {
            formatted += "\n... " + lines[i];
        }
    }

    return formatted;
}

QString REPLInterface::formatResult(const QString &result) const
{
    return result;
}
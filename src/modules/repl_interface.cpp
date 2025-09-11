#include "repl_interface.h"
#include <QApplication>
#include <QScrollBar>
#include <QFont>
#include <QDateTime>
#include <QCoreApplication>

REPLInterface::REPLInterface(PythonEngine *pythonEngine, QWidget *parent)
    : QWidget(parent), pythonEngine(pythonEngine), currentLayoutMode("bottom_input"),
      historyIndex(-1), executingCommand(false)
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
    if (obj == inputArea && event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        return handleKeyPress(keyEvent);
    }
    return QWidget::eventFilter(obj, event);
}

bool REPLInterface::handleKeyPress(QKeyEvent *keyEvent)
{
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
        // Save all Python variables to pickle file
        QString customName = "";
        if (cmd.startsWith("save ")) {
            customName = cmd.mid(5).trimmed(); // Extract name after "save "
        }
        QString result = saveVariablesToPickle(customName);
        appendOutput(formatResult(result));
        emit commandExecuted(command, result);
        return true;
    }
    else if (cmd == "load" || cmd.startsWith("load "))
    {
        // Load variables from pickle file
        if (cmd == "load") {
            QString error = "Error: Please specify a filename";
            appendOutput(formatResult(error));
            emit commandExecuted(command, error);
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

QString REPLInterface::saveVariablesToPickle(const QString& customName)
{
    if (!pythonEngine || !pythonEngine->isInitialized())
    {
        return "Error: Python engine not initialized";
    }

    // Get executable directory path
    QString executablePath = QCoreApplication::applicationDirPath();
    
    // Determine filename
    QString filename;
    if (customName.isEmpty()) {
        // Use timestamp if no custom name provided
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        filename = QString("saved_variables_%1.pickle").arg(timestamp);
    } else {
        // Use custom name, ensure .pickle extension
        filename = customName;
        if (!filename.endsWith(".pickle")) {
            filename += ".pickle";
        }
    }
    
    QString fullPath = executablePath + "/" + filename;
    
    // Execute save operation step by step to avoid compilation issues
    pythonEngine->evaluateExpression("import pickle, os");
    pythonEngine->evaluateExpression("globals_snapshot = dict(globals())");
    
    QString filterCode = QString(R"(
user_vars = {}
for name, value in globals_snapshot.items():
    if not name.startswith('__') and name not in ['pickle', 'os', 'user_vars', 'globals_snapshot', 'name', 'value', 'f', 'saved_count', 'result_message']:
        try:
            user_vars[name] = value
        except:
            pass
)");
    pythonEngine->evaluateExpression(filterCode.toStdString());
    
    QString saveCode = QString(R"(
try:
    with open(r'%1', 'wb') as f:
        pickle.dump(user_vars, f)
    saved_count = len(user_vars)
    result_message = f'Saved {saved_count} variables to %2'
except Exception as e:
    result_message = f'Error saving variables: {str(e)}'
)").arg(fullPath).arg(filename);
    
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

    // Get executable directory path
    QString executablePath = QCoreApplication::applicationDirPath();
    
    // Determine full file path
    QString actualFilename = filename;
    if (!actualFilename.endsWith(".pickle")) {
        actualFilename += ".pickle";
    }
    
    QString fullPath = executablePath + "/" + actualFilename;
    
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
                       
  load filename       - Load variables from pickle file
                       'load my_data' → loads my_data.pickle
                       'load data.pickle' → loads data.pickle
  
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
#include <Python.h>
#include <QApplication>
#include <QMainWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QSplitter>
#include <QTimer>
#include <QFontDatabase>
#include <QDir>
#include <QDebug>
#include <QWindow>
#include <QPushButton>
#include <QMouseEvent>
#include <QFrame>
#include "../../modules/tcp_server/tcp_server.h"
#include "../../modules/settings_handler/settings_handler.h"
#include <vector>
#include <string>
#include <random>
#include <sstream>
#include <memory>
#include <signal.h>

QString loadCustomFont(const QString &fontPath)
{
    // Check if file exists first
    if (!QFile::exists(fontPath))
    {
        return QString();
    }

    // Check if file is readable
    QFileInfo fileInfo(fontPath);
    if (!fileInfo.isReadable())
    {
        return QString();
    }

    // Check file size
    qint64 fileSize = fileInfo.size();
    if (fileSize == 0)
    {
        return QString();
    }

    // Try to add the font
    int fontId = QFontDatabase::addApplicationFont(fontPath);

    if (fontId != -1)
    {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);

        if (!fontFamilies.isEmpty())
        {
            return fontFamilies.first();
        }
    }

    return QString();
}

class CustomTitleBar : public QWidget
{
    Q_OBJECT

public:
    CustomTitleBar(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void onCloseClicked();
    void onMinimizeClicked();
    void onMaximizeClicked();

private:
    QPoint m_startPos;
    bool m_dragging;
};

CustomTitleBar::CustomTitleBar(QWidget *parent)
    : QWidget(parent), m_dragging(false)
{
    setFixedHeight(30);
    setStyleSheet("background-color: #202A34; border: none;");

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 12, 0);
    layout->setSpacing(0);

    // macOS-style window control buttons (left side)
    QPushButton *closeBtn = new QPushButton();
    QPushButton *minimizeBtn = new QPushButton();
    QPushButton *maximizeBtn = new QPushButton();

    // Set fixed size for round buttons
    closeBtn->setFixedSize(12, 12);
    minimizeBtn->setFixedSize(12, 12);
    maximizeBtn->setFixedSize(12, 12);

    // macOS button styles - round colored circles with symbols
    QString closeStyle =
        "QPushButton {"
        "    background-color: #FF5F57;"
        "    border: none;"
        "    border-radius: 6px;"
        "    color: transparent;"
        "    font-size: 8px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #FF4136;"
        "    color: #8B0000;"
        "}";

    QString minimizeStyle =
        "QPushButton {"
        "    background-color: #FFBD2E;"
        "    border: none;"
        "    border-radius: 6px;"
        "    color: transparent;"
        "    font-size: 8px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #FF851B;"
        "    color: #8B4513;"
        "}";

    QString maximizeStyle =
        "QPushButton {"
        "    background-color: #28CA42;"
        "    border: none;"
        "    border-radius: 6px;"
        "    color: transparent;"
        "    font-size: 8px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #2ECC40;"
        "    color: #006400;"
        "}";

    // Set button text (symbols)
    closeBtn->setText("×");
    minimizeBtn->setText("−");
    maximizeBtn->setText("+");

    closeBtn->setStyleSheet(closeStyle);
    minimizeBtn->setStyleSheet(minimizeStyle);
    maximizeBtn->setStyleSheet(maximizeStyle);

    connect(closeBtn, &QPushButton::clicked, this, &CustomTitleBar::onCloseClicked);
    connect(minimizeBtn, &QPushButton::clicked, this, &CustomTitleBar::onMinimizeClicked);
    connect(maximizeBtn, &QPushButton::clicked, this, &CustomTitleBar::onMaximizeClicked);

    // Add buttons to layout with proper spacing
    layout->addWidget(closeBtn);
    layout->addSpacing(8);
    layout->addWidget(minimizeBtn);
    layout->addSpacing(8);
    layout->addWidget(maximizeBtn);

    // Add stretch to fill the remaining space
    layout->addStretch();
}

void CustomTitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_startPos = event->globalPosition().toPoint();
        m_dragging = true;
    }
}

void CustomTitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton))
    {
        QWidget *topLevelWidget = window();
        if (topLevelWidget)
        {
            topLevelWidget->move(topLevelWidget->pos() + event->globalPosition().toPoint() - m_startPos);
            m_startPos = event->globalPosition().toPoint();
        }
    }
}

void CustomTitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_dragging = false;
    }
}

void CustomTitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        onMaximizeClicked();
    }
}

void CustomTitleBar::onCloseClicked()
{
    QWidget *topLevelWidget = window();
    if (topLevelWidget)
    {
        topLevelWidget->close();
    }
}

void CustomTitleBar::onMinimizeClicked()
{
    QWidget *topLevelWidget = window();
    if (topLevelWidget)
    {
        topLevelWidget->showMinimized();
    }
}

void CustomTitleBar::onMaximizeClicked()
{
    QWidget *topLevelWidget = window();
    if (topLevelWidget)
    {
        if (topLevelWidget->isMaximized())
        {
            topLevelWidget->showNormal();
        }
        else
        {
            topLevelWidget->showMaximized();
        }
    }
}

class PythonREPLWidget : public QMainWindow
{
    Q_OBJECT

public:
    PythonREPLWidget(QWidget *parent = nullptr);
    ~PythonREPLWidget();

private slots:
    void executeCommand();
    void updateVariables();

private:
    QTextEdit *outputArea;
    QLineEdit *inputLine;
    QListWidget *variablesList;
    QTimer *updateTimer;
    TCPServer *tcpServer;
    QString customFontFamily;
    CustomTitleBar *titleBar;
    std::unique_ptr<SettingsHandler> settingsHandler;

    void setupUI();
    void initializePython();
    void loadFonts();
    void loadSettings();
    void saveSettings();
    void injectPythonVariable(const std::string &header, const std::string &payload);
    std::vector<std::string> getUserVariables();
    std::string parseJsonValue(const std::string &json, const std::string &key);
    std::string generateRandomVariableName();
    std::string evaluatePythonExpression(const std::string &expression);
};

PythonREPLWidget::PythonREPLWidget(QWidget *parent)
    : QMainWindow(parent), tcpServer(nullptr)
{
    // Initialize settings handler
    settingsHandler = std::make_unique<SettingsHandler>("LumosWorkspace");
    loadSettings();
    
    loadFonts();
    setupUI();
    initializePython();

    // Start TCP server
    tcpServer = new TCPServer(8080);
    tcpServer->onDataReceived = [this](const std::string &header, const std::string &payload)
    {
        this->injectPythonVariable(header, payload);
    };

    if (tcpServer->start())
    {
        outputArea->append("TCP server started on port 8080");
    }
    else
    {
        outputArea->append("Failed to start TCP server");
    }

    // Timer to update variables periodically
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &PythonREPLWidget::updateVariables);
    updateTimer->start(1000); // Update every second

    updateVariables(); // Initial update
}

PythonREPLWidget::~PythonREPLWidget()
{
    // Save settings before destroying
    saveSettings();
    
    if (tcpServer)
    {
        tcpServer->stop();
        delete tcpServer;
    }
    Py_Finalize();
}

void PythonREPLWidget::setupUI()
{
    setWindowTitle("Python REPL with TCP Integration");
    setMinimumSize(800, 600);

    // Remove system window frame to use custom title bar
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    // Set main window background
    setStyleSheet("QMainWindow { background-color: #202A34; border: none; }");

    // Create main container widget
    QWidget *mainWidget = new QWidget(this);
    mainWidget->setStyleSheet("background-color: #202A34; border: none;");
    setCentralWidget(mainWidget);

    // Create main vertical layout for title bar + content
    QVBoxLayout *mainVerticalLayout = new QVBoxLayout(mainWidget);
    mainVerticalLayout->setContentsMargins(0, 0, 0, 0);
    mainVerticalLayout->setSpacing(0);

    // Add custom title bar
    titleBar = new CustomTitleBar(this);
    mainVerticalLayout->addWidget(titleBar);

    // Create content widget for the REPL and variables
    QWidget *contentWidget = new QWidget();
    contentWidget->setStyleSheet("background-color: #202A34; border: none;");
    mainVerticalLayout->addWidget(contentWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(contentWidget);
    mainLayout->setContentsMargins(5, 0, 0, 0); // Small left margin, no other margins

    // Left side - REPL (output + input)
    QWidget *replWidget = new QWidget();
    replWidget->setStyleSheet("background-color: #202A34;");
    QVBoxLayout *replLayout = new QVBoxLayout(replWidget);

    outputArea = new QTextEdit();
    outputArea->setReadOnly(true);
    outputArea->setFont(QFont(customFontFamily.isEmpty() ? "Monaco" : customFontFamily, 12));
    outputArea->setStyleSheet(
        "QTextEdit {"
        "    background-color: #202A34;"
        "    color: #CCCCCC;" // Light gray text
        "    border: none;"
        "    selection-background-color: #444444;"
        "}");
    replLayout->addWidget(outputArea);

    inputLine = new QLineEdit();
    inputLine->setFont(QFont(customFontFamily.isEmpty() ? "Monaco" : customFontFamily, 12));
    inputLine->setStyleSheet(
        "QLineEdit {"
        "    background-color: #202A34;"
        "    color: #CCCCCC;" // Light gray text
        "    border: 1px solid #555555;"
        "    border-radius: 6px;"
        "    padding: 6px 10px;"
        "}");
    connect(inputLine, &QLineEdit::returnPressed, this, &PythonREPLWidget::executeCommand);
    replLayout->addWidget(inputLine);

    // Right side - Variables
    QWidget *variablesWidget = new QWidget();
    variablesWidget->setStyleSheet("background-color: #202A34;");
    QVBoxLayout *variablesLayout = new QVBoxLayout(variablesWidget);

    QLabel *variablesLabel = new QLabel("Variables");
    variablesLabel->setFont(QFont(customFontFamily.isEmpty() ? "Monaco" : customFontFamily, 12, QFont::Bold));
    variablesLabel->setStyleSheet("QLabel { color: #CCCCCC; background-color: #202A34; }"); // Light gray text
    variablesLayout->addWidget(variablesLabel);

    // Add horizontal line below Variables label
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Plain);
    line->setStyleSheet("QFrame { color: #555555; background-color: #555555; }");
    line->setFixedHeight(1);
    variablesLayout->addWidget(line);

    variablesList = new QListWidget();
    variablesList->setFont(QFont(customFontFamily.isEmpty() ? "Monaco" : customFontFamily, 10));
    variablesList->setStyleSheet(
        "QListWidget {"
        "    background-color: #202A34;"
        "    color: #CCCCCC;" // Light gray text
        "    border: none;"
        "    selection-background-color: #444444;"
        "}"
        "QListWidget::item {"
        "    padding: 2px;"
        "    border-bottom: 1px solid #444444;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #444444;"
        "}");
    variablesLayout->addWidget(variablesList);

    // Splitter to adjust sizes
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->setStyleSheet(
        "QSplitter {"
        "    background-color: #202A34;"
        "}"
        "QSplitter::handle {"
        "    background-color: #555555;"
        "    width: 1px;"
        "}");
    splitter->addWidget(replWidget);
    splitter->addWidget(variablesWidget);
    splitter->setSizes({600, 200});

    mainLayout->addWidget(splitter);

    outputArea->append("Python REPL with TCP Integration");
    outputArea->append("Type Python commands below. TCP server listening on port 8080.");
    outputArea->append(">>> ");
}

void PythonREPLWidget::initializePython()
{
    // Set Python path environment variable
    setenv("PYTHONPATH", "../third_party/cpython/Lib", 1);
    setenv("PYTHONHOME", "../third_party/cpython", 1);

    // Initialize Python interpreter
    Py_Initialize();

    if (!Py_IsInitialized())
    {
        outputArea->append("Failed to initialize Python interpreter");
        return;
    }

    // Redirect Python stdout to capture print() statements
    const char *stdout_redirect_code = R"(
import sys
import io

class QtOutput:
    def __init__(self):
        self.buffer = io.StringIO()
    
    def write(self, text):
        self.buffer.write(text)
    
    def flush(self):
        pass
    
    def get_output(self):
        content = self.buffer.getvalue()
        self.buffer = io.StringIO()  # Reset buffer
        return content

# Create custom stdout redirector
_qt_stdout = QtOutput()
sys.stdout = _qt_stdout
sys.stderr = _qt_stdout
)";

    PyRun_SimpleString(stdout_redirect_code);

    outputArea->append("Python interpreter initialized");
}

void PythonREPLWidget::loadFonts()
{
    // Try to load custom fonts from these locations
    QStringList fontPaths = {
        "./fonts/custom_font.ttf",                            // Relative to executable
        "../fonts/custom_font.ttf",                           // Relative to build dir
        QDir::homePath() + "/.fonts/custom_font.ttf",         // User fonts
        "/System/Library/Fonts/Monaco.ttf",                   // System Monaco on macOS
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf" // Linux example
    };

    for (const QString &fontPath : fontPaths)
    {

        if (QFile::exists(fontPath))
        {
            QString family = loadCustomFont(fontPath);
            if (!family.isEmpty())
            {
                customFontFamily = family;
                return;
            }
        }
    }

    customFontFamily = QString(); // Use default
}

void PythonREPLWidget::executeCommand()
{
    QString command = inputLine->text().trimmed();
    inputLine->clear();

    if (command.isEmpty())
    {
        outputArea->append(">>> ");
        return;
    }

    outputArea->append(QString(">>> %1").arg(command));

    if (command == "exit()" || command == "quit()")
    {
        close();
        return;
    }

    std::string result = evaluatePythonExpression(command.toStdString());

    if (!result.empty())
    {
        outputArea->append(QString::fromStdString(result));
    }

    outputArea->append(">>> ");

    // Scroll to bottom
    QTextCursor cursor = outputArea->textCursor();
    cursor.movePosition(QTextCursor::End);
    outputArea->setTextCursor(cursor);

    // Update variables immediately after execution
    updateVariables();
}

void PythonREPLWidget::updateVariables()
{
    auto variables = getUserVariables();
    variablesList->clear();

    for (const auto &var : variables)
    {
        variablesList->addItem(QString::fromStdString(var));
    }
}

std::string PythonREPLWidget::generateRandomVariableName()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);

    return "tcp_var_" + std::to_string(dis(gen));
}

std::string PythonREPLWidget::parseJsonValue(const std::string &json, const std::string &key)
{
    size_t key_pos = json.find("\"" + key + "\"");
    if (key_pos == std::string::npos)
        return "";

    size_t colon_pos = json.find(":", key_pos);
    if (colon_pos == std::string::npos)
        return "";

    size_t start = json.find("\"", colon_pos);
    if (start == std::string::npos)
        return "";
    start++;

    size_t end = json.find("\"", start);
    if (end == std::string::npos)
        return "";

    return json.substr(start, end - start);
}

void PythonREPLWidget::injectPythonVariable(const std::string &header, const std::string &payload)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    std::string type = parseJsonValue(header, "type");
    std::string requested_name = parseJsonValue(header, "name");
    std::string var_name = requested_name.empty() ? generateRandomVariableName() : requested_name;

    PyObject *main_module = PyImport_AddModule("__main__");
    PyObject *main_dict = PyModule_GetDict(main_module);

    if (type == "int_list")
    {
        PyObject *list_obj = PyList_New(0);

        // Simple parser for [1, 2, 3, 4, 5] format
        size_t start = payload.find('[');
        size_t end = payload.find(']');
        if (start != std::string::npos && end != std::string::npos)
        {
            std::string numbers = payload.substr(start + 1, end - start - 1);
            std::istringstream iss(numbers);
            std::string token;

            while (std::getline(iss, token, ','))
            {
                // Trim whitespace
                token.erase(0, token.find_first_not_of(" \t"));
                token.erase(token.find_last_not_of(" \t") + 1);

                if (!token.empty())
                {
                    int value = std::stoi(token);
                    PyObject *py_int = PyLong_FromLong(value);
                    PyList_Append(list_obj, py_int);
                    Py_DECREF(py_int);
                }
            }
        }

        PyDict_SetItemString(main_dict, var_name.c_str(), list_obj);
        Py_DECREF(list_obj);

        outputArea->append(QString("TCP: Injected list as variable '%1'").arg(QString::fromStdString(var_name)));
    }
    else if (type == "string")
    {
        // Remove quotes from payload
        std::string str_value = payload;
        if (str_value.front() == '"' && str_value.back() == '"')
        {
            str_value = str_value.substr(1, str_value.length() - 2);
        }

        PyObject *py_str = PyUnicode_FromString(str_value.c_str());
        PyDict_SetItemString(main_dict, var_name.c_str(), py_str);
        Py_DECREF(py_str);

        outputArea->append(QString("TCP: Injected string as variable '%1'").arg(QString::fromStdString(var_name)));
    }

    PyGILState_Release(gstate);

    // Update variables list immediately
    updateVariables();

    // Scroll to bottom
    QTextCursor cursor = outputArea->textCursor();
    cursor.movePosition(QTextCursor::End);
    outputArea->setTextCursor(cursor);
}

std::vector<std::string> PythonREPLWidget::getUserVariables()
{
    std::vector<std::string> variables;

    // Get main module dictionary
    PyObject *main_module = PyImport_AddModule("__main__");
    PyObject *main_dict = PyModule_GetDict(main_module);

    if (!main_dict)
        return variables;

    PyObject *keys = PyDict_Keys(main_dict);
    if (!keys)
        return variables;

    Py_ssize_t size = PyList_Size(keys);
    for (Py_ssize_t i = 0; i < size; i++)
    {
        PyObject *key = PyList_GetItem(keys, i);
        if (!key)
            continue;

        const char *key_str = PyUnicode_AsUTF8(key);
        if (!key_str)
            continue;

        // Skip built-in variables that start with __ and internal modules
        std::string key_name(key_str);
        if (key_name.substr(0, 2) == "__")
            continue;

        // Skip internal modules and stdout redirector
        if (key_name == "sys" || key_name == "io" || key_name == "_qt_stdout" ||
            key_name == "QtOutput")
            continue;

        // Get the value and its type
        PyObject *value = PyDict_GetItem(main_dict, key);
        if (value)
        {
            PyObject *type_obj = PyObject_Type(value);
            PyObject *type_name = PyObject_GetAttrString(type_obj, "__name__");
            PyObject *repr_obj = PyObject_Repr(value);

            if (type_name && repr_obj)
            {
                const char *type_str = PyUnicode_AsUTF8(type_name);
                const char *repr_str = PyUnicode_AsUTF8(repr_obj);

                if (type_str && repr_str)
                {
                    std::string var_info = key_name + ": " + type_str;

                    // Add value if it's not too long
                    std::string value_str(repr_str);
                    if (value_str.length() < 50)
                    {
                        var_info += " = " + value_str;
                    }

                    variables.push_back(var_info);
                }
            }

            Py_XDECREF(type_obj);
            Py_XDECREF(type_name);
            Py_XDECREF(repr_obj);
        }
    }

    Py_DECREF(keys);
    return variables;
}

std::string PythonREPLWidget::evaluatePythonExpression(const std::string &expression)
{
    // Get main module and its dictionary for persistent state
    PyObject *main_module = PyImport_AddModule("__main__");
    PyObject *main_dict = PyModule_GetDict(main_module);

    std::string output = "";

    // Try to evaluate as expression first
    PyObject *result = PyRun_String(expression.c_str(), Py_eval_input, main_dict, main_dict);

    if (result)
    {
        // Expression evaluation succeeded
        if (result != Py_None)
        {
            PyObject *repr = PyObject_Repr(result);
            if (repr)
            {
                const char *result_str = PyUnicode_AsUTF8(repr);
                output = result_str ? result_str : "";
                Py_DECREF(repr);
            }
        }
        Py_DECREF(result);
    }
    else
    {
        // Clear error and try as statement
        PyErr_Clear();

        // For statements, just execute them
        result = PyRun_String(expression.c_str(), Py_single_input, main_dict, main_dict);

        if (result)
        {
            Py_DECREF(result);
        }
        else
        {
            // Handle error
            if (PyErr_Occurred())
            {
                PyObject *exc_type, *exc_value, *exc_traceback;
                PyErr_Fetch(&exc_type, &exc_value, &exc_traceback);

                std::string error_msg = "Error: ";
                if (exc_value)
                {
                    PyObject *str_exc = PyObject_Str(exc_value);
                    if (str_exc)
                    {
                        const char *msg = PyUnicode_AsUTF8(str_exc);
                        if (msg)
                        {
                            error_msg += msg;
                        }
                        Py_DECREF(str_exc);
                    }
                }

                Py_XDECREF(exc_type);
                Py_XDECREF(exc_value);
                Py_XDECREF(exc_traceback);

                return error_msg;
            }
        }
    }

    // Get any output from print() statements
    PyObject *get_output = PyRun_String("_qt_stdout.get_output()", Py_eval_input, main_dict, main_dict);
    if (get_output)
    {
        const char *print_output = PyUnicode_AsUTF8(get_output);
        if (print_output && strlen(print_output) > 0)
        {
            if (!output.empty())
            {
                output += "\n" + std::string(print_output);
            }
            else
            {
                output = std::string(print_output);
            }
        }
        Py_DECREF(get_output);
    }

    return output;
}

void PythonREPLWidget::loadSettings()
{
    if (!settingsHandler) return;
    
    // Load window geometry
    int width = settingsHandler->getInt("window.width", 800);
    int height = settingsHandler->getInt("window.height", 600);
    int x = settingsHandler->getInt("window.x", -1);
    int y = settingsHandler->getInt("window.y", -1);
    
    resize(width, height);
    if (x >= 0 && y >= 0) {
        move(x, y);
    }
    
    // Load TCP server port
    int tcpPort = settingsHandler->getInt("tcp.port", 8080);
    // Note: TCP port will be used when creating the server
    
    // Load UI preferences
    std::string fontFamily = settingsHandler->getString("ui.font_family", "");
    if (!fontFamily.empty()) {
        customFontFamily = QString::fromStdString(fontFamily);
    }
    
    // Load splitter sizes
    std::vector<int> splitterSizes = settingsHandler->getSetting<std::vector<int>>("ui.splitter_sizes", std::vector<int>{600, 200});
    // Note: Splitter sizes will be applied in setupUI()
}

void PythonREPLWidget::saveSettings()
{
    if (!settingsHandler) return;
    
    // Save window geometry
    settingsHandler->setInt("window.width", width());
    settingsHandler->setInt("window.height", height());
    settingsHandler->setInt("window.x", x());
    settingsHandler->setInt("window.y", y());
    settingsHandler->setBool("window.maximized", isMaximized());
    
    // Save TCP server port (if we ever make it configurable)
    if (tcpServer) {
        // For now, just save the default port
        settingsHandler->setInt("tcp.port", 8080);
    }
    
    // Save UI preferences
    if (!customFontFamily.isEmpty()) {
        settingsHandler->setString("ui.font_family", customFontFamily.toStdString());
    }
    
    // Save splitter sizes (we'd need to access the splitter for this)
    // For now, just save default values
    std::vector<int> defaultSizes = {600, 200};
    settingsHandler->setSetting("ui.splitter_sizes", defaultSizes);
    
    // Force save to file
    settingsHandler->saveSettings();
}

void signalHandler(int signal) {
    if (signal == SIGINT) {
        QApplication::quit();
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Install signal handler for Ctrl+C
    signal(SIGINT, signalHandler);

    PythonREPLWidget window;
    window.show();

    return app.exec();
}

#include "main.moc"
#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QStringList>
#include <QKeyEvent>
#include <QStandardPaths>
#include "python_engine.h"

class SettingsManager;

class REPLInterface : public QWidget {
    Q_OBJECT

public:
    explicit REPLInterface(PythonEngine* pythonEngine, SettingsManager* settingsManager, QWidget* parent = nullptr);
    
    void setLayoutMode(const QString& mode);
    void focusInput();
    void clearOutput();
    void appendOutput(const QString& text);

signals:
    void commandExecuted(const QString& command, const QString& result);
    void layoutModeChangeRequested(const QString& mode);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void executeCommand();

private:
    void setupUI();
    void setupConnections();
    void setupEventFilters();
    bool handleKeyPress(QKeyEvent* keyEvent);
    void addToHistory(const QString& command);
    void navigateHistory(int direction);
    bool handleSpecialCommand(const QString& command);
    void clearVariables();
    QString saveVariablesToPickle(const QString& customName = "", const QString& varName = "");
    QString loadVariablesFromPickle(const QString& filename);
    QString listPickleFiles() const;
    QString getDefaultPickleDirectory() const;
    QString getHelpText() const;
    
    // File picker methods
    void startFilePicker();
    void updateFilePickerDisplay();
    void selectFile(int direction); // -1 for up, +1 for down
    void confirmFileSelection();
    void cancelFilePicker();
    QString formatPrompt(const QString& command) const;
    QString formatResult(const QString& result) const;
    
    PythonEngine* pythonEngine;
    SettingsManager* settingsManager;
    
    // UI Components
    QVBoxLayout* mainLayout;
    QTextEdit* outputArea;
    QWidget* inputContainer;
    QHBoxLayout* inputLayout;
    QTextEdit* inputArea;
    
    // State
    QString currentLayoutMode;
    QStringList commandHistory;
    int historyIndex;
    bool executingCommand;
    
    // File picker state
    bool filePickerMode;
    QStringList availableFiles;
    int selectedFileIndex;
};
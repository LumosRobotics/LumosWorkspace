#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QStringList>
#include <QKeyEvent>
#include "python_engine.h"

class REPLInterface : public QWidget {
    Q_OBJECT

public:
    explicit REPLInterface(PythonEngine* pythonEngine, QWidget* parent = nullptr);
    
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
    QString saveVariablesToPickle(const QString& customName = "");
    QString loadVariablesFromPickle(const QString& filename);
    QString getHelpText() const;
    QString formatPrompt(const QString& command) const;
    QString formatResult(const QString& result) const;
    
    PythonEngine* pythonEngine;
    
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
};
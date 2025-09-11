#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QTimer>
#include "python_engine.h"

class VariablesPanel : public QWidget {
    Q_OBJECT

public:
    explicit VariablesPanel(PythonEngine* pythonEngine, QWidget* parent = nullptr);
    
    void updateVariables();
    void setAutoUpdate(bool enabled, int intervalMs = 1000);

public slots:
    void onVariablesChanged();

signals:
    void variableSelected(const QString& name, const QString& value);

private slots:
    void onItemClicked(QListWidgetItem* item);
    void onAutoUpdateTimer();

private:
    void setupUI();
    void setupConnections();
    void populateVariablesList(const std::vector<PythonVariable>& variables);
    QString formatVariableDisplay(const PythonVariable& var);
    
    PythonEngine* pythonEngine;
    QVBoxLayout* layout;
    QLabel* headerLabel;
    QListWidget* variablesList;
    QTimer* autoUpdateTimer;
    
    bool autoUpdateEnabled;
};
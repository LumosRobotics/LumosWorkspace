#include "variables_panel.h"
#include <QListWidgetItem>
#include <QFont>
#include <QFrame>

VariablesPanel::VariablesPanel(PythonEngine *pythonEngine, QWidget *parent)
    : QWidget(parent), pythonEngine(pythonEngine), autoUpdateEnabled(true)
{
    setupUI();
    setupConnections();

    // Set up auto-update timer
    autoUpdateTimer = new QTimer(this);
    autoUpdateTimer->setInterval(1000); // 1 second
    connect(autoUpdateTimer, &QTimer::timeout, this, &VariablesPanel::onAutoUpdateTimer);
    autoUpdateTimer->start();
}

void VariablesPanel::setupUI()
{
    setObjectName("VariablesPanel");
    setMinimumWidth(200);

    layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);

    // Header
    headerLabel = new QLabel("Variables", this);
    headerLabel->setObjectName("VariablesPanelHeader");
    QFont headerFont("Monaco", 12);
    headerFont.setBold(true);
    headerLabel->setFont(headerFont);
    headerLabel->setAlignment(Qt::AlignCenter);

    // Horizontal separator line
    QFrame* separatorLine = new QFrame(this);
    separatorLine->setObjectName("VariablesSeparator");
    separatorLine->setFrameShape(QFrame::HLine);
    separatorLine->setFrameShadow(QFrame::Plain);
    separatorLine->setLineWidth(0);
    separatorLine->setFixedHeight(1);

    // Variables list
    variablesList = new QListWidget(this);
    variablesList->setObjectName("VariablesList");
    variablesList->setAlternatingRowColors(false);
    variablesList->setSelectionMode(QAbstractItemView::SingleSelection);
    variablesList->setFont(QFont("Monaco", 10));

    // Layout
    layout->addWidget(headerLabel);
    layout->addWidget(separatorLine);
    layout->addWidget(variablesList, 1); // Give it stretch factor
}

void VariablesPanel::setupConnections()
{
    connect(variablesList, &QListWidget::itemClicked,
            this, &VariablesPanel::onItemClicked);
}

void VariablesPanel::updateVariables()
{
    if (!pythonEngine || !pythonEngine->isInitialized())
    {
        variablesList->clear();
        QListWidgetItem *item = new QListWidgetItem("Python not initialized");
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        variablesList->addItem(item);
        return;
    }

    std::vector<PythonVariable> variables = pythonEngine->getUserVariables();
    populateVariablesList(variables);
}

void VariablesPanel::populateVariablesList(const std::vector<PythonVariable> &variables)
{
    variablesList->clear();

    if (variables.empty())
    {
        // QListWidgetItem* item = new QListWidgetItem("No variables defined");
        // item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        // variablesList->addItem(item);
        return;
    }

    for (const PythonVariable &var : variables)
    {
        QListWidgetItem *item = new QListWidgetItem(formatVariableDisplay(var));

        // Store variable data
        item->setData(Qt::UserRole, QString::fromStdString(var.name));
        item->setData(Qt::UserRole + 1, QString::fromStdString(var.value));
        item->setData(Qt::UserRole + 2, QString::fromStdString(var.type));

        // Set tooltip with full information
        QString tooltip = QString("Name: %1\nType: %2\nValue: %3")
                              .arg(QString::fromStdString(var.name))
                              .arg(QString::fromStdString(var.type))
                              .arg(QString::fromStdString(var.value));
        item->setToolTip(tooltip);

        variablesList->addItem(item);
    }
}

QString VariablesPanel::formatVariableDisplay(const PythonVariable &var)
{
    QString display = QString::fromStdString(var.name) + ": " + QString::fromStdString(var.type);

    // Add value if it's short enough
    if (var.value.length() < 80)
    {
        display += " = " + QString::fromStdString(var.value);
    }

    return display;
}

void VariablesPanel::setAutoUpdate(bool enabled, int intervalMs)
{
    autoUpdateEnabled = enabled;

    if (enabled)
    {
        autoUpdateTimer->setInterval(intervalMs);
        autoUpdateTimer->start();
    }
    else
    {
        autoUpdateTimer->stop();
    }
}

void VariablesPanel::onVariablesChanged()
{
    if (autoUpdateEnabled)
    {
        updateVariables();
    }
}

void VariablesPanel::onItemClicked(QListWidgetItem *item)
{
    if (!item)
        return;

    QString name = item->data(Qt::UserRole).toString();
    QString value = item->data(Qt::UserRole + 1).toString();

    if (!name.isEmpty())
    {
        emit variableSelected(name, value);
    }
}

void VariablesPanel::onAutoUpdateTimer()
{
    if (autoUpdateEnabled)
    {
        updateVariables();
    }
}
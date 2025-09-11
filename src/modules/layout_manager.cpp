#include "layout_manager.h"
#include "settings_manager.h"
#include "repl_interface.h"
#include "variables_panel.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>

LayoutManager::LayoutManager(SettingsManager* settingsManager, QObject* parent)
    : QObject(parent), settings(settingsManager), parentWidget(nullptr), 
      replInterface(nullptr), variablesPanel(nullptr), mainSplitter(nullptr),
      leftWidget(nullptr), leftLayout(nullptr), currentMode(LayoutMode::BottomInput) {
}

void LayoutManager::setupLayout(QWidget* parent, REPLInterface* repl, VariablesPanel* vars) {
    parentWidget = parent;
    replInterface = repl;
    variablesPanel = vars;
    
    // Clear any existing layout
    if (parentWidget->layout()) {
        delete parentWidget->layout();
    }
    
    // Create main splitter
    mainSplitter = new QSplitter(Qt::Horizontal, parentWidget);
    mainSplitter->setObjectName("MainSplitter");
    
    // Create left widget container
    leftWidget = new QWidget();
    leftWidget->setObjectName("LeftWidget");
    leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);
    
    // Add components to splitter
    mainSplitter->addWidget(leftWidget);
    mainSplitter->addWidget(variablesPanel);
    
    // Set up parent layout
    QHBoxLayout* parentLayout = new QHBoxLayout(parentWidget);
    parentLayout->setContentsMargins(0, 0, 0, 0);
    parentLayout->addWidget(mainSplitter);
    
    // Load initial layout mode
    QString modeString = settings->getString("ui.layout_mode", "bottom_input");
    setLayoutMode(modeString);
    
    // Connect to layout change requests
    connect(replInterface, &REPLInterface::layoutModeChangeRequested,
            this, &LayoutManager::onLayoutModeChangeRequested);
    
    // Restore splitter sizes
    restoreSplitterSizes();
}

void LayoutManager::setLayoutMode(LayoutMode mode) {
    if (currentMode == mode && leftLayout->count() > 0) {
        return; // Already in this mode
    }
    
    currentMode = mode;
    
    // Remove REPL from current layout
    if (replInterface->parent() != leftWidget) {
        replInterface->setParent(leftWidget);
    }
    
    // Clear left layout
    while (leftLayout->count() > 0) {
        QLayoutItem* item = leftLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->setParent(nullptr);
        }
    }
    
    // Setup new layout based on mode
    switch (currentMode) {
        case LayoutMode::BottomInput:
            setupBottomInputLayout();
            break;
        case LayoutMode::InlineInput:
            setupInlineInputLayout();
            break;
    }
    
    // Update settings
    settings->setValue("ui.layout_mode", getCurrentModeString());
    
    // Update REPL interface
    replInterface->setLayoutMode(getCurrentModeString());
    
    emit layoutModeChanged(currentMode);
    qDebug() << "Layout mode changed to:" << getCurrentModeString();
}

void LayoutManager::setLayoutMode(const QString& mode) {
    LayoutMode layoutMode = LayoutMode::BottomInput;
    
    if (mode == "inline_input") {
        layoutMode = LayoutMode::InlineInput;
    } else if (mode == "bottom_input") {
        layoutMode = LayoutMode::BottomInput;
    }
    
    setLayoutMode(layoutMode);
}

QString LayoutManager::getCurrentModeString() const {
    switch (currentMode) {
        case LayoutMode::InlineInput:
            return "inline_input";
        case LayoutMode::BottomInput:
        default:
            return "bottom_input";
    }
}

void LayoutManager::setupBottomInputLayout() {
    // In bottom input mode, REPL takes the full left panel
    leftLayout->addWidget(replInterface, 1);
    replInterface->show();
}

void LayoutManager::setupInlineInputLayout() {
    // In inline input mode, REPL also takes the full left panel
    // The difference is in how the REPL interface internally handles input
    leftLayout->addWidget(replInterface, 1);
    replInterface->show();
}

void LayoutManager::onLayoutModeChangeRequested(const QString& mode) {
    setLayoutMode(mode);
}

void LayoutManager::saveSplitterSizes() {
    if (!mainSplitter) return;
    
    QList<int> sizes = mainSplitter->sizes();
    if (sizes.size() >= 2) {
        QJsonArray sizeArray;
        for (int size : sizes) {
            sizeArray.append(size);
        }
        settings->setValue("ui.splitter_sizes", sizeArray.toVariantList());
    }
}

void LayoutManager::restoreSplitterSizes() {
    if (!mainSplitter) return;
    
    QList<int> defaultSizes = {600, 200};
    QList<int> sizes = settings->getIntList("ui.splitter_sizes", defaultSizes);
    
    if (sizes.size() >= 2) {
        applySplitterSizes(sizes);
    }
}

void LayoutManager::applySplitterSizes(const QList<int>& sizes) {
    if (!mainSplitter || sizes.size() < 2) return;
    
    mainSplitter->setSizes(sizes);
    
    // Ensure minimum sizes
    mainSplitter->setStretchFactor(0, 1); // Left panel is stretchable
    mainSplitter->setStretchFactor(1, 0); // Variables panel has fixed size preference
    
    // Set minimum sizes
    if (replInterface) {
        replInterface->setMinimumWidth(300);
    }
    if (variablesPanel) {
        variablesPanel->setMinimumWidth(150);
    }
}

QList<int> LayoutManager::getSplitterSizes() const {
    if (mainSplitter) {
        return mainSplitter->sizes();
    }
    return {600, 200};
}
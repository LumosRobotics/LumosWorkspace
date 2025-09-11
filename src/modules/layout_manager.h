#pragma once

#include <QObject>
#include <QWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>

class SettingsManager;
class REPLInterface;
class VariablesPanel;

enum class LayoutMode {
    BottomInput,
    InlineInput
};

class LayoutManager : public QObject {
    Q_OBJECT

public:
    explicit LayoutManager(SettingsManager* settingsManager, QObject* parent = nullptr);
    
    void setupLayout(QWidget* parentWidget, REPLInterface* replInterface, VariablesPanel* variablesPanel);
    void setLayoutMode(LayoutMode mode);
    void setLayoutMode(const QString& mode);
    
    LayoutMode getCurrentMode() const { return currentMode; }
    QString getCurrentModeString() const;
    
    // Splitter management
    void saveSplitterSizes();
    void restoreSplitterSizes();

public slots:
    void onLayoutModeChangeRequested(const QString& mode);

signals:
    void layoutModeChanged(LayoutMode mode);

private:
    void setupBottomInputLayout();
    void setupInlineInputLayout();
    void applySplitterSizes(const QList<int>& sizes);
    QList<int> getSplitterSizes() const;
    
    SettingsManager* settings;
    QWidget* parentWidget;
    REPLInterface* replInterface;
    VariablesPanel* variablesPanel;
    
    QSplitter* mainSplitter;
    QWidget* leftWidget;
    QVBoxLayout* leftLayout;
    
    LayoutMode currentMode;
};
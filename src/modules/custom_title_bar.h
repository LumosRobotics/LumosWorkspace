#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QMouseEvent>

class CustomTitleBar : public QWidget {
    Q_OBJECT

public:
    explicit CustomTitleBar(QWidget* parent = nullptr);
    
    void setTitle(const QString& title);
    void setButtonsEnabled(bool enabled);

signals:
    void minimizeClicked();
    void maximizeClicked(); 
    void closeClicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private slots:
    void onMinimizeClicked();
    void onMaximizeClicked();
    void onCloseClicked();

private:
    void setupUI();
    void setupConnections();
    
    QHBoxLayout* layout;
    QLabel* titleLabel;
    QPushButton* minimizeButton;
    QPushButton* maximizeButton;
    QPushButton* closeButton;
    
    QPoint dragPosition;
    bool dragging;
};
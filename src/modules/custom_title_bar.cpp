#include "custom_title_bar.h"
#include <QApplication>

CustomTitleBar::CustomTitleBar(QWidget *parent)
    : QWidget(parent), dragging(false)
{
    setupUI();
    setupConnections();
}

void CustomTitleBar::setupUI()
{
    setFixedHeight(30);
    setObjectName("CustomTitleBar");

    layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(10);

    // titleLabel = new QLabel("LumosWorkspace REPL", this);
    // titleLabel->setObjectName("TitleLabel");

    // Create macOS-style buttons
    closeButton = new QPushButton("", this);
    minimizeButton = new QPushButton("", this);
    maximizeButton = new QPushButton("", this);

    // macOS-style button styling
    QString buttonBaseStyle = R"(
        QPushButton {
            min-width: 12px;
            max-width: 12px;
            min-height: 12px;
            max-height: 12px;
            border: none;
            border-radius: 6px;
            margin: 2px;
        }
    )";

    closeButton->setStyleSheet(buttonBaseStyle + R"(
        QPushButton {
            background-color: #ff5f56;
        }
        QPushButton:hover {
            background-color: #ff3b30;
        }
    )");

    minimizeButton->setStyleSheet(buttonBaseStyle + R"(
        QPushButton {
            background-color: #ffbd2e;
        }
        QPushButton:hover {
            background-color: #ff9500;
        }
    )");

    maximizeButton->setStyleSheet(buttonBaseStyle + R"(
        QPushButton {
            background-color: #28ca42;
        }
        QPushButton:hover {
            background-color: #30d158;
        }
    )");

    closeButton->setObjectName("CloseButton");
    minimizeButton->setObjectName("MinimizeButton");
    maximizeButton->setObjectName("MaximizeButton");

    // Layout - macOS style with buttons on the left
    layout->addWidget(closeButton);
    layout->addWidget(minimizeButton);
    layout->addWidget(maximizeButton);
    layout->addWidget(titleLabel);
    layout->addStretch();
}

void CustomTitleBar::setupConnections()
{
    connect(minimizeButton, &QPushButton::clicked, this, &CustomTitleBar::onMinimizeClicked);
    connect(maximizeButton, &QPushButton::clicked, this, &CustomTitleBar::onMaximizeClicked);
    connect(closeButton, &QPushButton::clicked, this, &CustomTitleBar::onCloseClicked);
}

void CustomTitleBar::setTitle(const QString &title)
{
    titleLabel->setText(title);
}

void CustomTitleBar::setButtonsEnabled(bool enabled)
{
    minimizeButton->setEnabled(enabled);
    maximizeButton->setEnabled(enabled);
    closeButton->setEnabled(enabled);
}

void CustomTitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        dragPosition = event->globalPosition().toPoint() - window()->frameGeometry().topLeft();
        dragging = true;
        event->accept();
    }
}

void CustomTitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (dragging && (event->buttons() & Qt::LeftButton))
    {
        window()->move(event->globalPosition().toPoint() - dragPosition);
        event->accept();
    }
}

void CustomTitleBar::onMinimizeClicked()
{
    emit minimizeClicked();
    if (QWidget *topLevel = window())
    {
        topLevel->showMinimized();
    }
}

void CustomTitleBar::onMaximizeClicked()
{
    emit maximizeClicked();
    if (QWidget *topLevel = window())
    {
        if (topLevel->isMaximized())
        {
            topLevel->showNormal();
            maximizeButton->setText("□");
        }
        else
        {
            topLevel->showMaximized();
            maximizeButton->setText("⧉");
        }
    }
}

void CustomTitleBar::onCloseClicked()
{
    emit closeClicked();
    if (QWidget *topLevel = window())
    {
        topLevel->close();
    }
}
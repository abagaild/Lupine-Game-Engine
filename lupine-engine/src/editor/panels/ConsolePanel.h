#pragma once

#ifndef CONSOLEPANEL_H
#define CONSOLEPANEL_H

#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>

class ConsolePanel : public QWidget
{
    Q_OBJECT
public:
    explicit ConsolePanel(QWidget *parent = nullptr);
    void addMessage(const QString& message);
private:
    QVBoxLayout* m_layout;
    QTextEdit* m_textEdit;
};

#endif // CONSOLEPANEL_H

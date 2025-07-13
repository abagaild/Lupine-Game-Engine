#include "ConsolePanel.h"

ConsolePanel::ConsolePanel(QWidget *parent) : QWidget(parent)
{
    m_layout = new QVBoxLayout(this);
    m_textEdit = new QTextEdit();
    m_textEdit->setReadOnly(true);
    m_textEdit->append("Lupine Engine Console");
    m_layout->addWidget(m_textEdit);
}

void ConsolePanel::addMessage(const QString& message)
{
    m_textEdit->append(message);
}

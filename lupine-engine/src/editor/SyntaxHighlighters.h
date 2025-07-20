#pragma once

#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <QFont>
#include <QColor>

/**
 * @brief Python syntax highlighter
 */
class PythonSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit PythonSyntaxHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> m_highlightingRules;

    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_functionFormat;
    QTextCharFormat m_numberFormat;
};

/**
 * @brief Lua syntax highlighter
 */
class LuaSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit LuaSyntaxHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> m_highlightingRules;

    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_functionFormat;
    QTextCharFormat m_numberFormat;
};

/**
 * @brief C++ syntax highlighter
 */
class CppSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit CppSyntaxHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> m_highlightingRules;

    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_functionFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_preprocessorFormat;
};

/**
 * @brief Markdown syntax highlighter
 */
class MarkdownSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit MarkdownSyntaxHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> m_highlightingRules;

    QTextCharFormat m_headerFormat;
    QTextCharFormat m_boldFormat;
    QTextCharFormat m_italicFormat;
    QTextCharFormat m_codeFormat;
    QTextCharFormat m_linkFormat;
};

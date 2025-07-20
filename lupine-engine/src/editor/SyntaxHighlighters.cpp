#include "SyntaxHighlighters.h"

// Python Syntax Highlighter Implementation
PythonSyntaxHighlighter::PythonSyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    // Keyword format
    m_keywordFormat.setForeground(QColor(86, 156, 214));
    m_keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns;
    keywordPatterns << "\\bdef\\b" << "\\bclass\\b" << "\\bif\\b" << "\\belse\\b" << "\\belif\\b"
                    << "\\bfor\\b" << "\\bwhile\\b" << "\\btry\\b" << "\\bexcept\\b" << "\\bfinally\\b"
                    << "\\bwith\\b" << "\\bas\\b" << "\\bimport\\b" << "\\bfrom\\b" << "\\breturn\\b"
                    << "\\byield\\b" << "\\blambda\\b" << "\\band\\b" << "\\bor\\b" << "\\bnot\\b"
                    << "\\bin\\b" << "\\bis\\b" << "\\bTrue\\b" << "\\bFalse\\b" << "\\bNone\\b";

    foreach (const QString &pattern, keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_keywordFormat;
        m_highlightingRules.append(rule);
    }

    // String format
    m_stringFormat.setForeground(QColor(206, 145, 120));
    rule.pattern = QRegularExpression("\".*\"|'.*'");
    rule.format = m_stringFormat;
    m_highlightingRules.append(rule);

    // Comment format
    m_commentFormat.setForeground(QColor(106, 153, 85));
    rule.pattern = QRegularExpression("#[^\n]*");
    rule.format = m_commentFormat;
    m_highlightingRules.append(rule);

    // Function format
    m_functionFormat.setForeground(QColor(220, 220, 170));
    rule.pattern = QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\()");
    rule.format = m_functionFormat;
    m_highlightingRules.append(rule);

    // Number format
    m_numberFormat.setForeground(QColor(181, 206, 168));
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b");
    rule.format = m_numberFormat;
    m_highlightingRules.append(rule);
}

void PythonSyntaxHighlighter::highlightBlock(const QString& text)
{
    foreach (const HighlightingRule &rule, m_highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

// Lua Syntax Highlighter Implementation
LuaSyntaxHighlighter::LuaSyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    // Keyword format
    m_keywordFormat.setForeground(QColor(86, 156, 214));
    m_keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns;
    keywordPatterns << "\\bfunction\\b" << "\\bend\\b" << "\\bif\\b" << "\\bthen\\b" << "\\belse\\b"
                    << "\\belseif\\b" << "\\bfor\\b" << "\\bwhile\\b" << "\\bdo\\b" << "\\brepeat\\b"
                    << "\\buntil\\b" << "\\breturn\\b" << "\\bbreak\\b" << "\\blocal\\b" << "\\band\\b"
                    << "\\bor\\b" << "\\bnot\\b" << "\\bin\\b" << "\\btrue\\b" << "\\bfalse\\b" << "\\bnil\\b";

    foreach (const QString &pattern, keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_keywordFormat;
        m_highlightingRules.append(rule);
    }

    // String format
    m_stringFormat.setForeground(QColor(206, 145, 120));
    rule.pattern = QRegularExpression("\".*\"|'.*'|\\[\\[.*\\]\\]");
    rule.format = m_stringFormat;
    m_highlightingRules.append(rule);

    // Comment format
    m_commentFormat.setForeground(QColor(106, 153, 85));
    rule.pattern = QRegularExpression("--[^\n]*|--\\[\\[.*\\]\\]");
    rule.format = m_commentFormat;
    m_highlightingRules.append(rule);

    // Function format
    m_functionFormat.setForeground(QColor(220, 220, 170));
    rule.pattern = QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\()");
    rule.format = m_functionFormat;
    m_highlightingRules.append(rule);

    // Number format
    m_numberFormat.setForeground(QColor(181, 206, 168));
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b");
    rule.format = m_numberFormat;
    m_highlightingRules.append(rule);
}

void LuaSyntaxHighlighter::highlightBlock(const QString& text)
{
    foreach (const HighlightingRule &rule, m_highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

// C++ Syntax Highlighter Implementation
CppSyntaxHighlighter::CppSyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    // Keyword format
    m_keywordFormat.setForeground(QColor(86, 156, 214));
    m_keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns;
    keywordPatterns << "\\bclass\\b" << "\\bstruct\\b" << "\\benum\\b" << "\\bunion\\b" << "\\bnamespace\\b"
                    << "\\bpublic\\b" << "\\bprivate\\b" << "\\bprotected\\b" << "\\bvirtual\\b" << "\\bstatic\\b"
                    << "\\bconst\\b" << "\\bvolatile\\b" << "\\bmutable\\b" << "\\binline\\b" << "\\bexplicit\\b"
                    << "\\bif\\b" << "\\belse\\b" << "\\bfor\\b" << "\\bwhile\\b" << "\\bdo\\b" << "\\bswitch\\b"
                    << "\\bcase\\b" << "\\bdefault\\b" << "\\bbreak\\b" << "\\bcontinue\\b" << "\\breturn\\b"
                    << "\\btry\\b" << "\\bcatch\\b" << "\\bthrow\\b" << "\\bnew\\b" << "\\bdelete\\b"
                    << "\\btrue\\b" << "\\bfalse\\b" << "\\bnullptr\\b" << "\\bauto\\b" << "\\btypename\\b";

    foreach (const QString &pattern, keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_keywordFormat;
        m_highlightingRules.append(rule);
    }

    // Preprocessor format
    m_preprocessorFormat.setForeground(QColor(155, 155, 155));
    rule.pattern = QRegularExpression("^\\s*#.*");
    rule.format = m_preprocessorFormat;
    m_highlightingRules.append(rule);

    // String format
    m_stringFormat.setForeground(QColor(206, 145, 120));
    rule.pattern = QRegularExpression("\".*\"|'.*'");
    rule.format = m_stringFormat;
    m_highlightingRules.append(rule);

    // Comment format
    m_commentFormat.setForeground(QColor(106, 153, 85));
    rule.pattern = QRegularExpression("//[^\n]*|/\\*.*\\*/");
    rule.format = m_commentFormat;
    m_highlightingRules.append(rule);

    // Function format
    m_functionFormat.setForeground(QColor(220, 220, 170));
    rule.pattern = QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\()");
    rule.format = m_functionFormat;
    m_highlightingRules.append(rule);

    // Number format
    m_numberFormat.setForeground(QColor(181, 206, 168));
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*[fFlL]?\\b");
    rule.format = m_numberFormat;
    m_highlightingRules.append(rule);
}

void CppSyntaxHighlighter::highlightBlock(const QString& text)
{
    foreach (const HighlightingRule &rule, m_highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

// Markdown Syntax Highlighter Implementation
MarkdownSyntaxHighlighter::MarkdownSyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    // Header format
    m_headerFormat.setForeground(QColor(86, 156, 214));
    m_headerFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("^#{1,6}.*");
    rule.format = m_headerFormat;
    m_highlightingRules.append(rule);

    // Bold format
    m_boldFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("\\*\\*.*\\*\\*|__.*__");
    rule.format = m_boldFormat;
    m_highlightingRules.append(rule);

    // Italic format
    m_italicFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("\\*.*\\*|_.*_");
    rule.format = m_italicFormat;
    m_highlightingRules.append(rule);

    // Code format
    m_codeFormat.setForeground(QColor(206, 145, 120));
    m_codeFormat.setBackground(QColor(40, 40, 40));
    rule.pattern = QRegularExpression("`.*`|```.*```");
    rule.format = m_codeFormat;
    m_highlightingRules.append(rule);

    // Link format
    m_linkFormat.setForeground(QColor(86, 156, 214));
    m_linkFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);
    rule.pattern = QRegularExpression("\\[.*\\]\\(.*\\)");
    rule.format = m_linkFormat;
    m_highlightingRules.append(rule);
}

void MarkdownSyntaxHighlighter::highlightBlock(const QString& text)
{
    foreach (const HighlightingRule &rule, m_highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

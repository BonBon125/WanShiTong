#pragma once
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class CodeHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit CodeHighlighter(QTextDocument* doc);
    void refreshTheme();

protected:
    void highlightBlock(const QString& text) override;

private:
    void loadColors();

    QTextCharFormat m_keyword, m_string, m_number, m_comment;
};

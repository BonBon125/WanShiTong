#pragma once
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class CodeHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit CodeHighlighter(QTextDocument* doc);

protected:
    void highlightBlock(const QString& text) override;

private:
    QTextCharFormat m_keyword, m_string, m_number, m_comment;
};

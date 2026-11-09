#pragma once
#include <QChar>
#include <QString>

class QTextDocument;

namespace md {

struct WriterOptions {
    QChar bullet = QLatin1Char('-'); // '-', '*', '+'
    QChar emphasis = QLatin1Char('*'); // '*' or '_'
    bool backslashBreak = false; // hard break: "\\\n" instead of two spaces
    int indentWidth = 4; // nested list indentation (2 or 4)
};

class MarkdownWriter {
public:
    explicit MarkdownWriter(WriterOptions o = { })
        : m_opt(o)
    {
    }
    QString write(const QTextDocument* doc) const;

private:
    WriterOptions m_opt;
};

} // namespace md

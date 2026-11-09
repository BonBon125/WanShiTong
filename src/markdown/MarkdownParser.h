#pragma once
#include <QString>

class QTextDocument;

namespace md {

class MarkdownParser {
public:
    // Replaces the contents of `doc` with the parsed markdown.
    static void parse(const QString& markdown, QTextDocument* doc);
};

} // namespace md

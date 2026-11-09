#include "markdown/MarkdownParser.h"
#include "markdown/MarkdownWriter.h"
#include <QTextDocument>
#include <QtTest>

class RoundTripTest : public QObject {
    Q_OBJECT
private slots:
    void roundTrip_data()
    {
        QTest::addColumn<QString>("in");
        QTest::addColumn<QString>("out");
        auto same = [](const char* name, const char* md) {
            QTest::newRow(name) << QString::fromUtf8(md) << QString::fromUtf8(md);
        };
        same("bold", "**bold**\n");
        same("italic", "*it*\n");
        same("bolditalic", "***both***\n");
        same("strike", "~~gone~~\n");
        same("highlight", "==mark==\n");
        same("code", "`code`\n");
        same("subscript", "H~2~O\n");
        same("superscript", "X^2^\n");
        same("atx", "## Two\n");
        same("setext", "Title\n=====\n");
        same("bullets", "- a\n- b\n");
        same("ordered", "1. a\n2. b\n");
        same("nested", "- a\n    - b\n");
        same("tasks", "- [ ] todo\n- [x] done\n");
        same("quote", "> quote\n");
        same("nestedquote", ">> deeper\n");
        same("fence", "```js\nlet a = 1;\n```\n");
        same("link", "[t](http://x.y)\n");
        same("autolink", "<https://example.com>\n");
        same("reflink", "[t][id]\n\n[id]: http://x.y\n");
        same("footnote", "Text[^1]\n\n[^1]: Note\n");
        same("table", "| A | B |\n| :-- | --: |\n| 1 | 2 |\n");
        same("rule", "a\n\n---\n\nb\n");
        same("hardbreak", "a  \nb\n");
        same("escapes", "\\*not italic\\*\n");
        same("entities", "&amp; &lt; &gt;\n");
        QTest::newRow("underscore-bold") << "__b__ and _i_\n"
                                         << "**b** and *i*\n";
    }

    void roundTrip()
    {
        QFETCH(QString, in);
        QFETCH(QString, out);
        QTextDocument doc;
        md::MarkdownParser::parse(in, &doc);
        QCOMPARE(md::MarkdownWriter().write(&doc), out);
    }
};

QTEST_MAIN(RoundTripTest)
#include "RoundTripTest.moc"

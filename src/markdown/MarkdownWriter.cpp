#include "MarkdownWriter.h"
#include "MdFormat.h"

#include <QRegularExpression>
#include <QTextFragment>
#include <QTextFrame>
#include <QTextTable>
#include <QVariantMap>

namespace md {
namespace {

    struct Mark {
        QString key, open, close;
        bool operator==(const Mark& o) const { return key == o.key; }
    };
    struct Run {
        QString text;
        bool raw = false; // already markdown, don't split whitespace / escape
        QVector<Mark> marks;
    };

    bool isWs(QChar c) { return c == QLatin1Char(' ') || c == QLatin1Char('\t'); }

    QString escapeText(const QString& s)
    {
        QString out;
        out.reserve(s.size() + 8);
        for (int i = 0; i < s.size(); ++i) {
            const QChar c = s[i];
            switch (c.unicode()) {
            case '\\':
            case '*':
            case '_':
            case '`':
            case '[':
            case ']':
            case '~':
            case '^':
                out += QLatin1Char('\\');
                out += c;
                break;
            case '&':
                out += QLatin1String("&amp;");
                break;
            case '<':
                out += QLatin1String("&lt;");
                break;
            case '>':
                out += QLatin1String("&gt;");
                break;
            case 0xA0:
                out += QLatin1String("&nbsp;");
                break;
            case '=':
                if ((i + 1 < s.size() && s[i + 1] == QLatin1Char('=')) || (i > 0 && s[i - 1] == QLatin1Char('=')))
                    out += QLatin1Char('\\');
                out += c;
                break;
            default:
                out += c;
            }
        }
        return out;
    }

    QString blockStartEscape(const QString& s)
    {
        static const QRegularExpression ol(QStringLiteral(R"re(^(\d+)([.)])(\s|$))re"));
        static const QRegularExpression underline(QStringLiteral(R"re(^(=+|-+)\s*$)re"));
        if (s.startsWith(QLatin1Char('#')))
            return QLatin1Char('\\') + s;
        if (!s.isEmpty() && (s[0] == QLatin1Char('-') || s[0] == QLatin1Char('+')) && (s.size() == 1 || s[1].isSpace()))
            return QLatin1Char('\\') + s;
        const auto m = ol.match(s);
        if (m.hasMatch()) {
            const int p = m.capturedEnd(1);
            return s.left(p) + QLatin1Char('\\') + s.mid(p);
        }
        if (underline.match(s).hasMatch())
            return QLatin1Char('\\') + s;
        return s;
    }

    QString codeSpan(const QString& t)
    {
        int longest = 0, cur = 0;
        for (QChar c : t) {
            if (c == QLatin1Char('`'))
                longest = qMax(longest, ++cur);
            else
                cur = 0;
        }
        const QString fence(longest == 0 ? 1 : longest + 1, QLatin1Char('`'));
        const bool pad = t.startsWith(QLatin1Char('`')) || t.endsWith(QLatin1Char('`')) || (t.startsWith(QLatin1Char(' ')) && t.endsWith(QLatin1Char(' ')));
        return fence + (pad ? QStringLiteral(" ") : QString()) + t + (pad ? QStringLiteral(" ") : QString()) + fence;
    }

    QVector<Mark> intersect(const QVector<Mark>& a, const QVector<Mark>& b)
    {
        QVector<Mark> r;
        for (const Mark& m : a)
            if (b.contains(m))
                r.push_back(m);
        return r;
    }

    // Moves leading/trailing whitespace outside emphasis markers (`** a**` is not valid emphasis).
    QVector<Run> splitWhitespace(const QVector<Run>& in)
    {
        QVector<Run> out;
        for (int i = 0; i < in.size(); ++i) {
            const Run& r = in[i];
            if (r.raw || r.text.isEmpty()) {
                out.push_back(r);
                continue;
            }
            const QVector<Mark> prev = i > 0 ? in[i - 1].marks : QVector<Mark>();
            const QVector<Mark> next = i + 1 < in.size() ? in[i + 1].marks : QVector<Mark>();
            const QString& t = r.text;
            int a = 0;
            while (a < t.size() && isWs(t[a]))
                ++a;
            int b = t.size();
            while (b > a && isWs(t[b - 1]))
                --b;
            if (a == t.size()) {
                Run w;
                w.text = t;
                w.marks = intersect(intersect(r.marks, prev), next);
                out.push_back(w);
                continue;
            }
            if (a > 0) {
                Run w;
                w.text = t.left(a);
                w.marks = intersect(r.marks, prev);
                out.push_back(w);
            }
            Run core = r;
            core.text = t.mid(a, b - a);
            out.push_back(core);
            if (b < t.size()) {
                Run w;
                w.text = t.mid(b);
                w.marks = intersect(r.marks, next);
                out.push_back(w);
            }
        }
        return out;
    }

    QString inlineMarkdown(const QTextBlock& block, const WriterOptions& o, bool ignoreBold)
    {
        const QString em(1, o.emphasis), strong(2, o.emphasis);
        const QString brk = o.backslashBreak ? QStringLiteral("\\\n") : QStringLiteral("  \n");
        auto mark = [](const QString& key, const QString& open, const QString& close) {
            return Mark { key, open, close };
        };

        QVector<Run> runs;
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fr = it.fragment();
            if (!fr.isValid())
                continue;
            const QTextCharFormat f = fr.charFormat();
            Run r;

            bool autolink = false;
            if (f.isAnchor() && !f.anchorHref().isEmpty()) {
                const QString href = f.anchorHref();
                const QString dest = href.contains(QLatin1Char(' ')) ? "<" + href + ">" : href;
                switch (f.intProperty(PropLinkKind)) {
                case LinkAuto:
                    autolink = true;
                    r.marks.push_back(mark("a:" + href, "<", ">"));
                    break;
                case LinkReference: {
                    const QString id = f.stringProperty(PropLinkRef);
                    r.marks.push_back(mark("r:" + id + href, "[", "][" + id + "]"));
                    break;
                }
                default:
                    r.marks.push_back(mark("l:" + href, "[", "](" + dest + ")"));
                }
                if (autolink) {
                    r.text = href;
                    r.raw = true;
                    runs.push_back(r);
                    continue;
                }
            }

            if (f.isImageFormat()) {
                const QTextImageFormat im = f.toImageFormat();
                r.text = "![" + escapeText(im.stringProperty(PropAlt)) + "](" + im.name() + ")";
                r.raw = true;
                runs.push_back(r);
                continue;
            }
            if (f.hasProperty(PropFootnoteRef)) {
                r.text = "[^" + f.stringProperty(PropFootnoteRef) + "]";
                r.raw = true;
                runs.push_back(r);
                continue;
            }

            if (!ignoreBold && f.fontWeight() >= QFont::Bold)
                r.marks.push_back(mark("b", strong, strong));
            if (f.fontItalic())
                r.marks.push_back(mark("i", em, em));
            if (f.fontStrikeOut())
                r.marks.push_back(mark("s", "~~", "~~"));
            if (f.boolProperty(PropHighlight))
                r.marks.push_back(mark("h", "==", "=="));
            if (f.verticalAlignment() == QTextCharFormat::AlignSubScript)
                r.marks.push_back(mark("sub", "~", "~"));
            if (f.verticalAlignment() == QTextCharFormat::AlignSuperScript)
                r.marks.push_back(mark("sup", "^", "^"));

            QString t = fr.text();
            if (f.boolProperty(PropInlineCode)) {
                t.replace(QChar(QChar::LineSeparator), QLatin1Char(' '));
                r.text = codeSpan(t);
                r.raw = true;
            } else {
                r.text = escapeText(t);
            }
            runs.push_back(r);
        }

        runs = splitWhitespace(runs);

        QString out;
        QVector<Mark> stack;
        auto closeTo = [&](int idx) {
            while (stack.size() > idx)
                out += stack.takeLast().close;
        };
        for (const Run& run : runs) {
            int bad = -1;
            for (int i = 0; i < stack.size(); ++i)
                if (!run.marks.contains(stack[i])) {
                    bad = i;
                    break;
                }
            QVector<Mark> reopen;
            if (bad >= 0) {
                const QVector<Mark> popped = stack.mid(bad);
                closeTo(bad);
                for (const Mark& m : popped)
                    if (run.marks.contains(m))
                        reopen.push_back(m);
            }
            for (const Mark& m : reopen) {
                out += m.open;
                stack.push_back(m);
            }
            for (const Mark& m : run.marks)
                if (!stack.contains(m)) {
                    out += m.open;
                    stack.push_back(m);
                }
            QString t = run.text;
            if (!run.raw)
                t.replace(QChar(QChar::LineSeparator), brk);
            out += t;
        }
        closeTo(0);
        return out;
    }

    bool isOrdered(QTextListFormat::Style s)
    {
        return s == QTextListFormat::ListDecimal || s == QTextListFormat::ListLowerAlpha || s == QTextListFormat::ListUpperAlpha || s == QTextListFormat::ListLowerRoman || s == QTextListFormat::ListUpperRoman;
    }

    struct Writer {
        enum Kind { None,
            Para,
            Heading,
            List,
            Code,
            Rule,
            Table,
            Footnote };

        explicit Writer(const WriterOptions& opt)
            : o(opt)
        {
        }

        const WriterOptions& o;
        QStringList out;
        Kind prev = None;
        int prevQuote = 0;

        bool inCode = false;
        QStringList code;
        QString codeLang, codeFence;
        int codeQuote = 0;

        void put(const QStringList& lines, int quote, Kind kind)
        {
            if (prev != None) {
                const bool tight = (kind == List && prev == List && quote == prevQuote) || (kind == Footnote && prev == Footnote && quote == prevQuote);
                if (!tight)
                    out << ((quote > 0 && prevQuote > 0) ? QString(qMin(quote, prevQuote), QLatin1Char('>')) : QString());
            }
            const QString pre = quote > 0 ? QString(quote, QLatin1Char('>')) + QLatin1Char(' ') : QString();
            for (const QString& l : lines)
                out << (l.isEmpty() ? pre.trimmed() : pre + l);
            prev = kind;
            prevQuote = quote;
        }

        void flushCode()
        {
            if (!inCode)
                return;
            QString fence = codeFence.isEmpty() ? QStringLiteral("```") : codeFence;
            const QChar fc = fence[0];
            for (const QString& l : code) {
                int run = 0;
                while (run < l.size() && l[run] == fc)
                    ++run;
                if (run >= fence.size())
                    fence = QString(run + 1, fc);
            }
            QStringList lines;
            lines << fence + codeLang;
            lines += code;
            lines << fence;
            inCode = false;
            code.clear();
            put(lines, codeQuote, Code);
        }

        void writeFrame(QTextFrame* frame)
        {
            for (QTextFrame::iterator it = frame->begin(); !it.atEnd(); ++it) {
                if (auto* t = qobject_cast<QTextTable*>(it.currentFrame())) {
                    flushCode();
                    writeTable(t);
                } else if (QTextFrame* f = it.currentFrame()) {
                    flushCode();
                    writeFrame(f);
                } else if (it.currentBlock().isValid()) {
                    writeBlock(it.currentBlock());
                }
            }
        }

        void writeBlock(const QTextBlock& b)
        {
            const QTextBlockFormat bf = b.blockFormat();
            const int q = bf.intProperty(PropQuoteLevel);

            if (bf.boolProperty(PropCodeBlock)) {
                if (inCode && codeQuote != q)
                    flushCode();
                if (!inCode) {
                    inCode = true;
                    codeLang = bf.stringProperty(PropCodeLang);
                    codeFence = bf.stringProperty(PropCodeFence);
                    codeQuote = q;
                }
                QString t = b.text();
                t.replace(QChar(QChar::LineSeparator), QLatin1Char('\n'));
                code += t.split(QLatin1Char('\n'));
                return;
            }
            flushCode();

            if (bf.boolProperty(PropIsRule)) {
                const QString s = bf.stringProperty(PropRuleStyle);
                put({ s.isEmpty() ? QStringLiteral("---") : s }, q, Rule);
                return;
            }

            const bool hasObject = b.text().contains(QChar(QChar::ObjectReplacementCharacter));
            if (b.text().trimmed().isEmpty() && !b.textList() && !hasObject && !bf.hasProperty(PropFootnoteDef))
                return;

            if (bf.hasProperty(PropFootnoteDef)) {
                const QStringList ls = inlineMarkdown(b, o, false).split(QLatin1Char('\n'));
                QStringList res;
                for (int i = 0; i < ls.size(); ++i)
                    res << (i == 0 ? "[^" + bf.stringProperty(PropFootnoteDef) + "]: " + ls[i] : "    " + ls[i]);
                put(res, q, Footnote);
                return;
            }

            if (const int lvl = bf.headingLevel(); lvl > 0) {
                QString s = inlineMarkdown(b, o, true);
                s.replace(QLatin1Char('\n'), QLatin1Char(' '));
                s = s.trimmed();
                if (bf.boolProperty(PropSetext) && lvl <= 2 && !s.isEmpty())
                    put({ s, QString(qMax(3, s.size()), lvl == 1 ? QLatin1Char('=') : QLatin1Char('-')) }, q, Heading);
                else
                    put({ QString(lvl, QLatin1Char('#')) + QLatin1Char(' ') + s }, q, Heading);
                return;
            }

            if (QTextList* l = b.textList()) {
                const int level = qMax(0, l->format().indent() - 1);
                const bool ordered = isOrdered(l->format().style());
                const QString marker = ordered ? QString::number(l->itemNumber(b) + 1) + QLatin1Char('.')
                                               : QString(o.bullet);
                QString task;
                if (bf.marker() == QTextBlockFormat::MarkerType::Checked)
                    task = QStringLiteral("[x] ");
                else if (bf.marker() == QTextBlockFormat::MarkerType::Unchecked)
                    task = QStringLiteral("[ ] ");
                const QStringList ls = inlineMarkdown(b, o, false).split(QLatin1Char('\n'));
                const QString ind(level * o.indentWidth, QLatin1Char(' '));
                QStringList res;
                for (int i = 0; i < ls.size(); ++i)
                    res << (i == 0 ? ind + marker + QLatin1Char(' ') + task + ls[i]
                                   : ind + QString(marker.size() + 1, QLatin1Char(' ')) + ls[i]);
                put(res, q, List);
                return;
            }

            put(blockStartEscape(inlineMarkdown(b, o, false)).split(QLatin1Char('\n')), q, Para);
        }

        void writeTable(QTextTable* t)
        {
            static const QRegularExpression breaks(QStringLiteral(R"(\s*\n)"));
            QStringList lines, aligns;
            for (int r = 0; r < t->rows(); ++r) {
                QStringList cells;
                for (int c = 0; c < t->columns(); ++c) {
                    const QTextTableCell cell = t->cellAt(r, c);
                    QString txt;
                    for (QTextFrame::iterator it = cell.begin(); !it.atEnd(); ++it) {
                        if (!it.currentBlock().isValid())
                            continue;
                        if (!txt.isEmpty())
                            txt += QLatin1String("<br>");
                        txt += inlineMarkdown(it.currentBlock(), o, r == 0);
                    }
                    txt.replace(breaks, QStringLiteral("<br>"));
                    txt.replace(QLatin1Char('|'), QLatin1String("\\|"));
                    cells << txt.trimmed();
                    if (r == 0)
                        aligns << cell.firstCursorPosition().blockFormat().stringProperty(PropColAlign);
                }
                lines << "| " + cells.join(QLatin1String(" | ")) + " |";
                if (r == 0) {
                    QStringList d;
                    for (const QString& a : aligns)
                        d << (a == QLatin1String("c")     ? QStringLiteral(":-:")
                                : a == QLatin1String("r") ? QStringLiteral("--:")
                                : a == QLatin1String("l") ? QStringLiteral(":--")
                                                          : QStringLiteral("---"));
                    lines << "| " + d.join(QLatin1String(" | ")) + " |";
                }
            }
            put(lines, 0, Table);
        }
    };

} // namespace

QString MarkdownWriter::write(const QTextDocument* doc) const
{
    Writer w(m_opt);
    w.writeFrame(doc->rootFrame());
    w.flushCode();

    const QVariantMap refs = doc->property("md.refs").toMap();
    if (!refs.isEmpty()) {
        QStringList defs;
        for (auto it = refs.begin(); it != refs.end(); ++it)
            defs << "[" + it.key() + "]: " + it.value().toString();
        w.put(defs, 0, Writer::Para);
    }
    while (!w.out.isEmpty() && w.out.last().trimmed().isEmpty())
        w.out.removeLast();
    return w.out.join(QLatin1Char('\n')) + QLatin1Char('\n');
}

} // namespace md

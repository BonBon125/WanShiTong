#include "MarkdownParser.h"
#include "MdFormat.h"

#include <QMap>
#include <QRegularExpression>
#include <QTextDocumentFragment>
#include <QTextTable>
#include <QVariantMap>

namespace md {
namespace {

    struct Style {
        bool bold = false, italic = false, strike = false, hl = false, sub = false, sup = false;
        bool link = false;
        QString href, ref;
        int kind = LinkInline;
    };

    struct Item {
        bool active = false;
        int level = 0;
        bool ordered = false;
        int task = 0; // 0 none, 1 unchecked, 2 checked
        QStringList lines;
    };

    const QString kPunct = QStringLiteral("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~");

#define RE(name, pat) const QRegularExpression name(QStringLiteral(pat))
    RE(kFencePrefix, R"re(^\s{0,3}(`{3,}|~{3,}))re");
    RE(kFenceOpen, R"re(^\s{0,3}(`{3,}|~{3,})\s*([^\s`]*).*$)re");
    RE(kQuote, R"re(^\s{0,3}>)re");
    RE(kAtx, R"re(^\s{0,3}(#{1,6})(?:[ \t]+(.*?))?(?:[ \t]+#+)?[ \t]*$)re");
    RE(kSetext, R"re(^\s{0,3}(=+|-+)[ \t]*$)re");
    RE(kRule, R"re(^\s{0,3}([-*_])(?:[ \t]*\1){2,}[ \t]*$)re");
    RE(kDelim, R"re(^\s*\|?\s*:?-+:?\s*(?:\|\s*:?-+:?\s*)*\|?\s*$)re");
    RE(kFootDef, R"re(^\[\^([^\]\s]+)\]:[ \t]*(.*)$)re");
    RE(kRefDef, R"re(^\s{0,3}\[([^\]^][^\]]*)\]:\s*<?([^\s>]+)>?(?:\s+["'(].*["')])?\s*$)re");
    RE(kListItem, R"re(^(\s*)([-+*]|\d{1,9}[.)])(?:[ \t]+(.*))?$)re");
    RE(kTask, R"re(^\[([ xX])\](?:[ \t]+(.*))?$)re");
    RE(kEntity, R"re(\G&(?:#[0-9]{1,7}|#[xX][0-9a-fA-F]{1,6}|[A-Za-z][A-Za-z0-9]{1,31});)re");
    RE(kAutoLink, R"re(\G<([A-Za-z][A-Za-z0-9+.\-]{1,31}:[^\s<>]*|[^\s@<>]+@[^\s@<>]+\.[^\s@<>]+)>)re");
    RE(kFootRef, R"re(\G\[\^([^\]\s]+)\])re");
#undef RE
    const QRegularExpression kBr(QStringLiteral(R"re(\G<br\s*/?>)re"), QRegularExpression::CaseInsensitiveOption);

    QString decodeEntity(const QString& e)
    {
        if (e.startsWith(QLatin1String("&#"))) {
            bool ok = false;
            const bool hex = e[2] == QLatin1Char('x') || e[2] == QLatin1Char('X');
            const uint cp = hex ? e.mid(3, e.size() - 4).toUInt(&ok, 16) : e.mid(2, e.size() - 3).toUInt(&ok, 10);
            if (ok && cp > 0 && cp <= 0x10FFFF) {
                const char32_t c = cp;
                return QString::fromUcs4(&c, 1);
            }
            return e;
        }
        static const QHash<QString, QString> common {
            { QStringLiteral("&amp;"), QStringLiteral("&") }, { QStringLiteral("&lt;"), QStringLiteral("<") },
            { QStringLiteral("&gt;"), QStringLiteral(">") }, { QStringLiteral("&quot;"), QStringLiteral("\"") },
            { QStringLiteral("&apos;"), QStringLiteral("'") }, { QStringLiteral("&nbsp;"), QString(QChar(0xA0)) }
        };
        const auto it = common.find(e);
        if (it != common.end())
            return *it;
        const QString r = QTextDocumentFragment::fromHtml(e).toPlainText();
        return r.isEmpty() ? e : r;
    }

    QString joinLines(const QStringList& ls)
    {
        QString out;
        for (int i = 0; i < ls.size(); ++i) {
            QString t = ls[i];
            const bool last = (i == ls.size() - 1);
            bool hard = false;
            if (!last) {
                int trail = 0;
                while (t.endsWith(QLatin1Char(' '))) {
                    t.chop(1);
                    ++trail;
                }
                hard = trail >= 2;
                if (!hard && t.endsWith(QLatin1Char('\\'))) {
                    int bs = 0;
                    for (int k = t.size() - 1; k >= 0 && t[k] == QLatin1Char('\\'); --k)
                        ++bs;
                    if (bs % 2 == 1) {
                        t.chop(1);
                        hard = true;
                    }
                }
            }
            out += t.trimmed();
            if (!last)
                out += hard ? QString(QChar(QChar::LineSeparator)) : QStringLiteral(" ");
        }
        return out;
    }

    QStringList splitRow(QString row)
    {
        row = row.trimmed();
        if (row.startsWith(QLatin1Char('|')))
            row.remove(0, 1);
        if (row.endsWith(QLatin1Char('|')) && !row.endsWith(QLatin1String("\\|")))
            row.chop(1);
        QStringList cells;
        QString cur;
        for (int i = 0; i < row.size(); ++i) {
            if (row[i] == QLatin1Char('\\') && i + 1 < row.size() && row[i + 1] == QLatin1Char('|')) {
                cur += QLatin1Char('|');
                ++i;
            } else if (row[i] == QLatin1Char('|')) {
                cells << cur.trimmed();
                cur.clear();
            } else {
                cur += row[i];
            }
        }
        cells << cur.trimmed();
        return cells;
    }

    int findClose(const QString& s, int open, QChar o = QLatin1Char('['), QChar c = QLatin1Char(']'))
    {
        int depth = 0;
        for (int i = open; i < s.size(); ++i) {
            const QChar ch = s[i];
            if (ch == QLatin1Char('\\')) {
                ++i;
                continue;
            }
            if (ch == o)
                ++depth;
            else if (ch == c && --depth == 0)
                return i;
        }
        return -1;
    }

    // s[pos] == '('
    bool parseDest(const QString& s, int pos, QString& url, int& end)
    {
        const int close = findClose(s, pos, QLatin1Char('('), QLatin1Char(')'));
        if (close < 0)
            return false;
        const QString inner = s.mid(pos + 1, close - pos - 1).trimmed();
        if (inner.startsWith(QLatin1Char('<'))) {
            const int e = inner.indexOf(QLatin1Char('>'));
            url = e > 0 ? inner.mid(1, e - 1) : inner;
        } else {
            int sp = -1;
            for (int k = 0; k < inner.size(); ++k)
                if (inner[k].isSpace()) {
                    sp = k;
                    break;
                }
            url = sp < 0 ? inner : inner.left(sp); // optional "title" is dropped
        }
        end = close + 1;
        return true;
    }

    class Impl {
    public:
        explicit Impl(QTextDocument* d)
            : doc(d)
            , cursor(d)
        {
        }

        void run(const QString& text)
        {
            QString t = text;
            t.replace(QLatin1String("\r\n"), QLatin1String("\n")).replace(QLatin1Char('\r'), QLatin1Char('\n'));
            QStringList lines = t.split(QLatin1Char('\n'));
            for (QString& l : lines) {
                int k = 0;
                while (k < l.size() && l[k] == QLatin1Char('\t'))
                    ++k;
                if (k)
                    l = QString(k * 4, QLatin1Char(' ')) + l.mid(k);
            }
            // pre-pass: reference definitions (outside fences)
            QStringList kept;
            bool fence = false;
            for (const QString& l : lines) {
                if (kFencePrefix.match(l).hasMatch())
                    fence = !fence;
                if (!fence) {
                    const auto r = kRefDef.match(l);
                    if (r.hasMatch()) {
                        refs.insert(r.captured(1).toLower(), r.captured(2));
                        refsOut.insert(r.captured(1), r.captured(2));
                        continue;
                    }
                }
                kept << l;
            }
            parseBlocks(kept, 0);
        }

        QVariantMap refsOut;

    private:
        QTextDocument* doc;
        QTextCursor cursor;
        bool first = true;
        QMap<QString, QString> refs;

        // ---------------- block level ----------------
        QTextBlockFormat blockFmt(int q) const
        {
            QTextBlockFormat bf;
            if (q > 0)
                bf.setProperty(PropQuoteLevel, QVariant(q));
            return bf;
        }

        void newBlock(const QTextBlockFormat& bf, const QTextCharFormat& cf)
        {
            if (first) {
                cursor.setBlockFormat(bf);
                cursor.setBlockCharFormat(cf);
                first = false;
            } else {
                cursor.insertBlock(bf, cf);
            }
        }

        void emitParagraph(const QString& text, int q)
        {
            newBlock(blockFmt(q), baseCharFormat());
            inlineTo(text, baseCharFormat(), Style());
        }

        void emitHeading(const QString& text, int level, bool setext, int q)
        {
            QTextBlockFormat bf = blockFmt(q);
            bf.setHeadingLevel(level);
            if (setext)
                bf.setProperty(PropSetext, QVariant(true));
            const QTextCharFormat cf = headingCharFormat(level);
            newBlock(bf, cf);
            inlineTo(text.trimmed(), cf, Style());
        }

        void emitRule(QChar ch, int q)
        {
            QTextBlockFormat bf = blockFmt(q);
            bf.setProperty(PropIsRule, QVariant(true));
            bf.setProperty(PropRuleStyle, QString(3, ch));
            newBlock(bf, baseCharFormat());
        }

        void emitCode(const QStringList& codeLines, const QString& lang, QChar fenceCh, int q)
        {
            QTextBlockFormat bf = blockFmt(q);
            bf.setProperty(PropCodeBlock, QVariant(true));
            bf.setProperty(PropCodeLang, lang);
            bf.setProperty(PropCodeFence, QString(3, fenceCh));
            const QTextCharFormat cf = codeBlockCharFormat();
            const QStringList ls = codeLines.isEmpty() ? QStringList { QString() } : codeLines;
            for (const QString& l : ls) {
                newBlock(bf, cf);
                if (!l.isEmpty())
                    cursor.insertText(l, cf);
            }
        }

        void emitFootnote(const QString& id, const QString& text, int q)
        {
            QTextBlockFormat bf = blockFmt(q);
            bf.setProperty(PropFootnoteDef, id);
            newBlock(bf, baseCharFormat());
            inlineTo(text, baseCharFormat(), Style());
        }

        void emitItem(const Item& it, int q, QMap<int, QTextList*>& lists)
        {
            QTextBlockFormat bf = blockFmt(q);
            if (it.task)
                bf.setMarker(it.task == 2 ? QTextBlockFormat::MarkerType::Checked
                                          : QTextBlockFormat::MarkerType::Unchecked);
            newBlock(bf, baseCharFormat());

            QTextListFormat lf;
            lf.setStyle(it.ordered ? QTextListFormat::ListDecimal
                                   : bulletStyle(it.level, it.task != 0));
            lf.setIndent(it.level + 1);

            QTextList* l = lists.value(it.level, nullptr);
            if (l && l->format().style() == lf.style()) {
                l->add(cursor.block());
            } else {
                l = cursor.createList(lf);
                lists[it.level] = l;
            }
            for (auto k = lists.begin(); k != lists.end();) {
                if (k.key() > it.level)
                    k = lists.erase(k);
                else
                    ++k;
            }
            inlineTo(joinLines(it.lines), baseCharFormat(), Style());
        }

        void emitTable(const QStringList& header, const QStringList& delim, const QList<QStringList>& body)
        {
            QStringList aligns;
            for (const QString& d : delim) {
                const bool l = d.startsWith(QLatin1Char(':')), r = d.endsWith(QLatin1Char(':'));
                aligns << (l && r ? QStringLiteral("c") : r ? QStringLiteral("r")
                        : l                                 ? QStringLiteral("l")
                                                            : QString());
            }
            const int cols = header.size(), rows = 1 + body.size();
            if (!first)
                cursor.insertBlock(QTextBlockFormat(), baseCharFormat());
            QTextTable* t = createTable(cursor, rows, cols, aligns);
            for (int r = 0; r < rows; ++r) {
                const QStringList& row = r == 0 ? header : body[r - 1];
                for (int c = 0; c < cols; ++c) {
                    QTextCharFormat cf = baseCharFormat();
                    if (r == 0)
                        setBold(cf, true);
                    const QTextCursor saved = cursor;
                    cursor = t->cellAt(r, c).firstCursorPosition();
                    inlineTo(row.value(c), cf, Style());
                    cursor = saved;
                }
            }
            cursor.movePosition(QTextCursor::End);
            first = true; // the empty block after the table is reused
        }

        void parseBlocks(const QStringList& L, int q)
        {
            QStringList para;
            Item item;
            QMap<int, QTextList*> lists;
            QVector<int> indents;

            auto flushPara = [&] {
                if (!para.isEmpty()) {
                    emitParagraph(joinLines(para), q);
                    para.clear();
                }
            };
            auto flushItem = [&] {
                if (item.active) {
                    emitItem(item, q, lists);
                    item = Item();
                }
            };
            auto flushAll = [&] { flushPara(); flushItem(); };
            auto resetLists = [&] { lists.clear(); indents.clear(); };

            for (int i = 0; i < L.size(); ++i) {
                const QString& line = L[i];

                if (line.trimmed().isEmpty()) {
                    flushAll();
                    int j = i + 1;
                    while (j < L.size() && L[j].trimmed().isEmpty())
                        ++j;
                    if (j >= L.size() || !kListItem.match(L[j]).hasMatch())
                        resetLists();
                    continue;
                }

                if (const auto m = kFenceOpen.match(line); m.hasMatch()) {
                    flushAll();
                    resetLists();
                    const QString fence = m.captured(1);
                    QStringList code;
                    int j = i + 1;
                    for (; j < L.size(); ++j) {
                        const QString t = L[j].trimmed();
                        if (t.size() >= fence.size() && t == QString(t.size(), fence[0]))
                            break;
                        code << L[j];
                    }
                    emitCode(code, m.captured(2), fence[0], q);
                    i = j;
                    continue;
                }

                if (kQuote.match(line).hasMatch()) {
                    flushAll();
                    resetLists();
                    QStringList inner;
                    int j = i;
                    for (; j < L.size() && kQuote.match(L[j]).hasMatch(); ++j) {
                        QString t = L[j];
                        t = t.mid(t.indexOf(QLatin1Char('>')) + 1);
                        if (t.startsWith(QLatin1Char(' ')))
                            t.remove(0, 1);
                        inner << t;
                    }
                    parseBlocks(inner, q + 1);
                    i = j - 1;
                    continue;
                }

                if (const auto m = kAtx.match(line); m.hasMatch()) {
                    flushAll();
                    resetLists();
                    emitHeading(m.captured(2), m.captured(1).size(), false, q);
                    continue;
                }

                if (!para.isEmpty()) {
                    if (const auto m = kSetext.match(line); m.hasMatch()) {
                        const int level = m.captured(1)[0] == QLatin1Char('=') ? 1 : 2;
                        const QString text = para.join(QLatin1Char(' '));
                        para.clear();
                        resetLists();
                        emitHeading(text, level, true, q);
                        continue;
                    }
                }

                if (const auto m = kRule.match(line); m.hasMatch()) {
                    flushAll();
                    resetLists();
                    emitRule(m.captured(1)[0], q);
                    continue;
                }

                if (line.contains(QLatin1Char('|')) && i + 1 < L.size() && L[i + 1].contains(QLatin1Char('|')) && kDelim.match(L[i + 1]).hasMatch()) {
                    const QStringList header = splitRow(line);
                    const QStringList delim = splitRow(L[i + 1]);
                    if (header.size() == delim.size()) {
                        flushAll();
                        resetLists();
                        QList<QStringList> body;
                        int j = i + 2;
                        for (; j < L.size() && !L[j].trimmed().isEmpty() && L[j].contains(QLatin1Char('|')); ++j)
                            body << splitRow(L[j]);
                        emitTable(header, delim, body);
                        i = j - 1;
                        continue;
                    }
                }

                if (const auto m = kFootDef.match(line); m.hasMatch()) {
                    flushAll();
                    resetLists();
                    QString text = m.captured(2);
                    int j = i + 1;
                    for (; j < L.size() && L[j].startsWith(QLatin1String("    ")) && !L[j].trimmed().isEmpty(); ++j)
                        text += QLatin1Char(' ') + L[j].trimmed();
                    emitFootnote(m.captured(1), text, q);
                    i = j - 1;
                    continue;
                }

                if (const auto m = kListItem.match(line); m.hasMatch()) {
                    flushAll();
                    const int indent = m.captured(1).size();
                    const QString marker = m.captured(2);
                    if (indents.isEmpty())
                        indents << indent;
                    else if (indent >= indents.last() + 2)
                        indents << indent;
                    else
                        while (indents.size() > 1 && indent < indents.last())
                            indents.removeLast();

                    item = Item();
                    item.active = true;
                    item.level = indents.size() - 1;
                    item.ordered = marker.back() == QLatin1Char('.') || marker.back() == QLatin1Char(')');
                    QString content = m.captured(3);
                    if (const auto tm = kTask.match(content); tm.hasMatch()) {
                        item.task = tm.captured(1) == QLatin1String(" ") ? 1 : 2;
                        content = tm.captured(2);
                    }
                    item.lines << content;
                    continue;
                }

                // plain text: continuation of a list item or a paragraph line
                if (item.active) {
                    item.lines << line.trimmed();
                } else {
                    if (para.isEmpty())
                        resetLists();
                    para << line;
                }
            }
            flushAll();
        }

        // ---------------- inline level ----------------
        QTextCharFormat fmt(const QTextCharFormat& base, const Style& s) const
        {
            QTextCharFormat f = base;
            if (s.bold)
                setBold(f, true);
            if (s.italic)
                f.setFontItalic(true);
            if (s.strike)
                f.setFontStrikeOut(true);
            if (s.hl)
                setHighlight(f, true);
            if (s.sub)
                f.setVerticalAlignment(QTextCharFormat::AlignSubScript);
            if (s.sup)
                f.setVerticalAlignment(QTextCharFormat::AlignSuperScript);
            if (s.link)
                setLink(f, s.href, s.kind, s.ref);
            return f;
        }

        void insertImage(const QString& alt, const QString& url, const QTextCharFormat& base, const Style& st)
        {
            QTextImageFormat im;
            static_cast<QTextCharFormat&>(im) = fmt(base, st);
            im.setName(url);
            im.setProperty(PropAlt, alt);
            const QSize sz = ensureImageResource(doc, url, alt);
            if (sz.width() > 640)
                im.setWidth(640);
            cursor.insertImage(im);
        }

        void inlineTo(const QString& s, const QTextCharFormat& base, const Style& st)
        {
            QString buf;
            auto flush = [&] {
                if (!buf.isEmpty()) {
                    cursor.insertText(buf, fmt(base, st));
                    buf.clear();
                }
            };
            const int n = s.size();
            int i = 0;
            while (i < n) {
                const QChar c = s[i];

                // backslash escape
                if (c == QLatin1Char('\\') && i + 1 < n && kPunct.contains(s[i + 1])) {
                    buf += s[i + 1];
                    i += 2;
                    continue;
                }
                // entities
                if (c == QLatin1Char('&')) {
                    const auto m = kEntity.match(s, i);
                    if (m.hasMatch()) {
                        buf += decodeEntity(m.captured(0));
                        i += m.capturedLength();
                        continue;
                    }
                }
                // code span
                if (c == QLatin1Char('`')) {
                    int k = 1;
                    while (i + k < n && s[i + k] == QLatin1Char('`'))
                        ++k;
                    int j = i + k, end = -1;
                    while (j < n) {
                        const int p = s.indexOf(QLatin1Char('`'), j);
                        if (p < 0)
                            break;
                        int m = 1;
                        while (p + m < n && s[p + m] == QLatin1Char('`'))
                            ++m;
                        if (m == k) {
                            end = p;
                            break;
                        }
                        j = p + m;
                    }
                    if (end >= 0) {
                        flush();
                        QString code = s.mid(i + k, end - i - k);
                        if (code.size() >= 2 && code.startsWith(QLatin1Char(' ')) && code.endsWith(QLatin1Char(' ')) && !code.trimmed().isEmpty())
                            code = code.mid(1, code.size() - 2);
                        QTextCharFormat f = fmt(base, st);
                        setInlineCode(f, true);
                        cursor.insertText(code, f);
                        i = end + k;
                        continue;
                    }
                    buf += QString(k, QLatin1Char('`'));
                    i += k;
                    continue;
                }
                // <br> and autolinks
                if (c == QLatin1Char('<')) {
                    if (const auto br = kBr.match(s, i); br.hasMatch()) {
                        buf += QChar(QChar::LineSeparator);
                        i += br.capturedLength();
                        continue;
                    }
                    if (const auto au = kAutoLink.match(s, i); au.hasMatch()) {
                        flush();
                        const QString url = au.captured(1);
                        Style a = st;
                        a.link = true;
                        a.kind = LinkAuto;
                        a.href = (url.contains(QLatin1Char('@')) && !url.contains(QLatin1Char(':'))) ? "mailto:" + url : url;
                        cursor.insertText(url, fmt(base, a));
                        i += au.capturedLength();
                        continue;
                    }
                }
                // images
                if (c == QLatin1Char('!') && i + 1 < n && s[i + 1] == QLatin1Char('[')) {
                    const int cb = findClose(s, i + 1);
                    QString url;
                    int end = 0;
                    if (cb > 0 && cb + 1 < n && s[cb + 1] == QLatin1Char('(') && parseDest(s, cb + 1, url, end)) {
                        flush();
                        insertImage(s.mid(i + 2, cb - i - 2), url, base, st);
                        i = end;
                        continue;
                    }
                }
                // footnote refs, links
                if (c == QLatin1Char('[')) {
                    if (const auto fn = kFootRef.match(s, i); fn.hasMatch()) {
                        flush();
                        cursor.insertText(fn.captured(1), footnoteRefFormat(base, fn.captured(1)));
                        i += fn.capturedLength();
                        continue;
                    }
                    if (!st.link) {
                        const int cb = findClose(s, i);
                        if (cb > 0) {
                            const QString label = s.mid(i + 1, cb - i - 1);
                            Style ls = st;
                            ls.link = true;
                            bool done = false;
                            QString url;
                            int end = 0;
                            if (cb + 1 < n && s[cb + 1] == QLatin1Char('(') && parseDest(s, cb + 1, url, end)) {
                                ls.href = url;
                                ls.kind = LinkInline;
                                i = end;
                                done = true;
                            } else if (cb + 1 < n && s[cb + 1] == QLatin1Char('[')) {
                                const int cb2 = findClose(s, cb + 1);
                                if (cb2 > 0) {
                                    QString id = s.mid(cb + 2, cb2 - cb - 2);
                                    if (id.isEmpty())
                                        id = label;
                                    if (refs.contains(id.toLower())) {
                                        ls.href = refs.value(id.toLower());
                                        ls.kind = LinkReference;
                                        ls.ref = id;
                                        i = cb2 + 1;
                                        done = true;
                                    }
                                }
                            } else if (refs.contains(label.toLower())) {
                                ls.href = refs.value(label.toLower());
                                ls.kind = LinkReference;
                                ls.ref = label;
                                i = cb + 1;
                                done = true;
                            }
                            if (done) {
                                flush();
                                inlineTo(label, base, ls);
                                continue;
                            }
                        }
                    }
                }
                // emphasis-like delimiters
                if (c == QLatin1Char('*') || c == QLatin1Char('_') || c == QLatin1Char('~') || c == QLatin1Char('=') || c == QLatin1Char('^')) {
                    int k = 1;
                    while (i + k < n && s[i + k] == c)
                        ++k;
                    QVector<int> tries;
                    if (c == QLatin1Char('*') || c == QLatin1Char('_')) {
                        for (int m = qMin(k, 3); m >= 1; --m)
                            tries << m;
                    } else if (c == QLatin1Char('~')) {
                        if (k <= 2)
                            tries << k;
                    } else if (c == QLatin1Char('=')) {
                        if (k == 2)
                            tries << 2;
                    } else if (k == 1)
                        tries << 1;

                    bool done = false;
                    for (const int m : tries) {
                        const int cs = i + k;
                        if (cs >= n || s[cs].isSpace())
                            continue;
                        if (c == QLatin1Char('_') && i > 0 && s[i - 1].isLetterOrNumber())
                            continue;
                        int j = cs, close = -1;
                        while (j < n) {
                            const QChar d = s[j];
                            if (d == QLatin1Char('\\')) {
                                j += 2;
                                continue;
                            }
                            if (d == QLatin1Char('`')) {
                                int kk = 1;
                                while (j + kk < n && s[j + kk] == QLatin1Char('`'))
                                    ++kk;
                                const int e = s.indexOf(QString(kk, QLatin1Char('`')), j + kk);
                                j = e < 0 ? j + kk : e + kk;
                                continue;
                            }
                            if (d == c) {
                                int r = 1;
                                while (j + r < n && s[j + r] == c)
                                    ++r;
                                if (r == m && j > cs && !s[j - 1].isSpace() && !(c == QLatin1Char('_') && j + r < n && s[j + r].isLetterOrNumber())) {
                                    close = j;
                                    break;
                                }
                                j += r;
                                continue;
                            }
                            ++j;
                        }
                        if (close < 0)
                            continue;
                        const QString inner = s.mid(cs, close - cs);
                        if ((c == QLatin1Char('~') && m == 1) || c == QLatin1Char('^')) {
                            bool ws = false;
                            for (int x = 0; x < inner.size(); ++x) {
                                if (inner[x] == QLatin1Char('\\')) {
                                    ++x;
                                    continue;
                                }
                                if (inner[x].isSpace()) {
                                    ws = true;
                                    break;
                                }
                            }
                            if (ws)
                                continue; // sub/sup may not contain spaces
                        }
                        buf += QString(k - m, c);
                        flush();
                        Style s2 = st;
                        if (c == QLatin1Char('*') || c == QLatin1Char('_')) {
                            if (m == 3)
                                s2.bold = s2.italic = true;
                            else if (m == 2)
                                s2.bold = true;
                            else
                                s2.italic = true;
                        } else if (c == QLatin1Char('~')) {
                            if (m == 2)
                                s2.strike = true;
                            else
                                s2.sub = true;
                        } else if (c == QLatin1Char('=')) {
                            s2.hl = true;
                        } else {
                            s2.sup = true;
                        }
                        inlineTo(inner, base, s2);
                        i = close + m;
                        done = true;
                        break;
                    }
                    if (done)
                        continue;
                }

                buf += c;
                ++i;
            }
            flush();
        }
    };

} // namespace

void MarkdownParser::parse(const QString& markdown, QTextDocument* doc)
{
    doc->setUndoRedoEnabled(false);
    doc->clear();
    Impl impl(doc);
    impl.run(markdown);
    doc->setProperty("md.refs", impl.refsOut);
    restyleAll(doc);
    doc->setModified(false);
    doc->setUndoRedoEnabled(true);
}

} // namespace md

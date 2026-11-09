#include "CodeHighlighter.h"
#include "markdown/MdFormat.h"

#include <QHash>
#include <QSet>

namespace {

struct Lang {
    QSet<QString> kw;
    QString line;
    bool block = false;
    bool backtick = false;
};

Lang make(const char* words, const char* line, bool block, bool backtick)
{
    Lang l;
    for (const QString& w : QString::fromLatin1(words).split(QLatin1Char(' '), Qt::SkipEmptyParts))
        l.kw.insert(w);
    l.line = QString::fromLatin1(line);
    l.block = block;
    l.backtick = backtick;
    return l;
}

const Lang* langFor(const QString& name)
{
    static const QHash<QString, Lang> langs = [] {
        QHash<QString, Lang> h;
        const Lang cpp = make("alignas auto bool break case catch char class const constexpr continue default delete do double else enum explicit extern false float for friend goto if inline int long namespace new noexcept nullptr operator private protected public return short signed sizeof static struct switch template this throw true try typedef typename union unsigned using virtual void volatile while", "//", true, false);
        const Lang js = make("async await break case catch class const continue debugger default delete do else export extends false finally for function if import in instanceof let new null return super switch this throw true try typeof undefined var void while with yield interface type enum implements", "//", true, true);
        const Lang py = make("and as assert async await break class continue def del elif else except False finally for from global if import in is lambda None nonlocal not or pass raise return True try while with yield", "#", false, false);
        const Lang sh = make("if then else elif fi for while do done case esac function in return local export echo exit", "#", false, false);
        const Lang rs = make("as async await break const continue crate dyn else enum extern false fn for if impl in let loop match mod move mut pub ref return self Self static struct super trait true type unsafe use where while", "//", true, false);
        const Lang java = make("abstract boolean break byte case catch char class const continue default do double else enum extends final finally float for if implements import instanceof int interface long new null package private protected public return short static super switch this throw throws true false try void volatile while", "//", true, false);
        const Lang go = make("break case chan const continue default defer else fallthrough for func go goto if import interface map package range return select struct switch type var true false nil", "//", true, true);
        const Lang json = make("true false null", "", false, false);
        for (const char* k : { "c", "h", "cpp", "c++", "cc", "hpp", "cxx" })
            h.insert(QString::fromLatin1(k), cpp);
        for (const char* k : { "js", "javascript", "ts", "typescript", "jsx", "tsx" })
            h.insert(QString::fromLatin1(k), js);
        for (const char* k : { "python", "py" })
            h.insert(QString::fromLatin1(k), py);
        for (const char* k : { "bash", "sh", "shell", "zsh" })
            h.insert(QString::fromLatin1(k), sh);
        for (const char* k : { "rust", "rs" })
            h.insert(QString::fromLatin1(k), rs);
        h.insert(QStringLiteral("java"), java);
        for (const char* k : { "go", "golang" })
            h.insert(QString::fromLatin1(k), go);
        h.insert(QStringLiteral("json"), json);
        return h;
    }();
    const auto it = langs.constFind(name.toLower());
    return it == langs.constEnd() ? nullptr : &*it;
}

} // namespace

CodeHighlighter::CodeHighlighter(QTextDocument* doc)
    : QSyntaxHighlighter(doc)
{
    m_keyword.setForeground(QColor(0xff, 0x7b, 0x72));
    m_string.setForeground(QColor(0xa5, 0xd6, 0xff));
    m_number.setForeground(QColor(0x79, 0xc0, 0xff));
    m_comment.setForeground(QColor(0x8b, 0x94, 0x9e));
    m_comment.setFontItalic(true);
}

void CodeHighlighter::highlightBlock(const QString& text)
{
    setCurrentBlockState(0);
    const QTextBlockFormat bf = currentBlock().blockFormat();
    if (!bf.boolProperty(md::PropCodeBlock))
        return;
    const Lang* lang = langFor(bf.stringProperty(md::PropCodeLang));
    if (!lang)
        return;

    const int n = text.size();
    int i = 0;
    bool inBlock = lang->block && previousBlockState() == 1;
    while (i < n) {
        if (inBlock) {
            const int e = text.indexOf(QLatin1String("*/"), i);
            if (e < 0) {
                setFormat(i, n - i, m_comment);
                setCurrentBlockState(1);
                return;
            }
            setFormat(i, e + 2 - i, m_comment);
            i = e + 2;
            inBlock = false;
            continue;
        }
        if (lang->block && text.mid(i, 2) == QLatin1String("/*")) {
            const int e = text.indexOf(QLatin1String("*/"), i + 2);
            if (e < 0) {
                setFormat(i, n - i, m_comment);
                setCurrentBlockState(1);
                return;
            }
            setFormat(i, e + 2 - i, m_comment);
            i = e + 2;
            continue;
        }
        if (!lang->line.isEmpty() && text.mid(i, lang->line.size()) == lang->line) {
            setFormat(i, n - i, m_comment);
            return;
        }
        const QChar c = text[i];
        if (c == QLatin1Char('"') || c == QLatin1Char('\'') || (c == QLatin1Char('`') && lang->backtick)) {
            int j = i + 1;
            while (j < n && text[j] != c) {
                if (text[j] == QLatin1Char('\\'))
                    ++j;
                ++j;
            }
            j = qMin(j + 1, n);
            setFormat(i, j - i, m_string);
            i = j;
        } else if (c.isDigit()) {
            int j = i;
            while (j < n && (text[j].isLetterOrNumber() || text[j] == QLatin1Char('.') || text[j] == QLatin1Char('_')))
                ++j;
            setFormat(i, j - i, m_number);
            i = j;
        } else if (c.isLetter() || c == QLatin1Char('_')) {
            int j = i;
            while (j < n && (text[j].isLetterOrNumber() || text[j] == QLatin1Char('_')))
                ++j;
            if (lang->kw.contains(text.mid(i, j - i)))
                setFormat(i, j - i, m_keyword);
            i = j;
        } else {
            ++i;
        }
    }
}

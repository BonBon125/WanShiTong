#pragma once
// Custom QTextFormat properties + helper functions shared by the
// parser, the writer and the editor.
#include "theme/Theme.h"
#include <QBrush>
#include <QColor>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextList>
#include <QTextTable>
#include <QUrl>
#include <QVector>

namespace md {

enum Prop {
    // char properties
    PropHighlight = QTextFormat::UserProperty + 1,
    PropInlineCode,
    PropLinkKind, // LinkKind
    PropLinkRef, // reference id
    PropFootnoteRef, // footnote id
    PropAlt, // image alt text
    // block properties
    PropQuoteLevel,
    PropCodeBlock,
    PropCodeLang,
    PropCodeFence,
    PropSetext,
    PropFootnoteDef,
    PropIsRule,
    PropRuleStyle, // "---", "***", "___"
    PropColAlign // "", "l", "c", "r" (table cells)
};

enum LinkKind { LinkInline = 0,
    LinkReference = 1,
    LinkAuto = 2 };

constexpr qreal kBasePt = 12.0;
constexpr int kQuoteIndent = 22;

// ---- colours (from the active theme) --------------------------------------

inline QColor linkColor() { return theme().link; }
inline QColor mutedColor() { return theme().muted; }
inline QColor borderColor() { return theme().border; }
inline QColor canvasBg() { return theme().canvas; }
inline QColor pageBg() { return theme().page; }
inline QColor codeBg() { return theme().inlineCode; }
inline QColor blockBg() { return theme().codeBlock; }
inline QColor quoteBg() { return theme().quote; }
inline QColor highlightBg() { return theme().highlight; }
inline QColor headerBg() { return theme().surface; }

// ---- list markers ---------------------------------------------------------

// Qt has no outlined-square style. We use "undefined" (Qt draws nothing)
// and paint the square ourselves in NoteEditor::paintEvent.
constexpr QTextListFormat::Style kHollowSquare = QTextListFormat::ListStyleUndefined;

inline bool isBulletStyle(QTextListFormat::Style s)
{
    return s == QTextListFormat::ListDisc || s == QTextListFormat::ListCircle
        || s == QTextListFormat::ListSquare || s == kHollowSquare;
}

// level is zero-based. Task items always use Disc, as Qt draws a checkbox instead.
inline QTextListFormat::Style bulletStyle(int level, bool task = false)
{
    if (task)
        return QTextListFormat::ListDisc;
    static const QTextListFormat::Style styles[] = {
        QTextListFormat::ListDisc, // filled bullet
        QTextListFormat::ListCircle, // outlined bullet
        QTextListFormat::ListSquare, // filled square
        kHollowSquare // outlined square
    };
    return styles[qMax(0, level) % 4];
}

// ---- fonts and base formats -----------------------------------------------

inline QStringList monoFamilies()
{
    return { QStringLiteral("DejaVu Sans Mono"), QStringLiteral("Ubuntu Mono"), QStringLiteral("Monospace") };
}
inline QString baseFamily() { return QGuiApplication::font().family(); }

inline qreal headingScale(int level)
{
    static const qreal s[] = { 1.0, 2.0, 1.6, 1.35, 1.15, 1.0, 0.9 };
    return s[qBound(0, level, 6)];
}

inline QTextCharFormat baseCharFormat()
{
    QTextCharFormat f;
    f.setFontPointSize(kBasePt);
    f.setFontWeight(QFont::Normal);
    return f;
}

inline QTextCharFormat headingCharFormat(int level)
{
    QTextCharFormat f;
    f.setFontPointSize(kBasePt * headingScale(level));
    f.setFontWeight(QFont::Bold);
    return f;
}

inline QTextCharFormat codeBlockCharFormat()
{
    QTextCharFormat f;
    f.setFontFamilies(monoFamilies());
    f.setFontFixedPitch(true);
    f.setFontPointSize(kBasePt - 1);
    f.setFontWeight(QFont::Normal);
    return f;
}

// ---- inline format helpers ------------------------------------------------

inline void setBold(QTextCharFormat& f, bool on) { f.setFontWeight(on ? QFont::Bold : QFont::Normal); }

inline void setHighlight(QTextCharFormat& f, bool on)
{
    f.setProperty(PropHighlight, QVariant(on));
    f.setBackground(on ? QBrush(highlightBg()) : QBrush(Qt::NoBrush));
}

inline void setInlineCode(QTextCharFormat& f, bool on)
{
    f.setProperty(PropInlineCode, QVariant(on));
    if (on) {
        f.setFontFamilies(monoFamilies());
        f.setFontFixedPitch(true);
        f.setBackground(codeBg());
    } else {
        f.setFontFamilies(QStringList { baseFamily() });
        f.setFontFixedPitch(false);
        f.setBackground(Qt::NoBrush);
    }
}

inline void setLink(QTextCharFormat& f, const QString& href, int kind, const QString& ref = QString())
{
    f.setAnchor(true);
    f.setAnchorHref(href);
    f.setProperty(PropLinkKind, QVariant(kind));
    f.setProperty(PropLinkRef, ref);
    f.setForeground(linkColor());
    f.setFontUnderline(true);
}

inline void clearLink(QTextCharFormat& f)
{
    f.setAnchor(false);
    f.setAnchorHref(QString());
    f.setProperty(PropLinkKind, QVariant(LinkInline));
    f.setForeground(theme().text);
    f.setFontUnderline(false);
}

inline QTextCharFormat footnoteRefFormat(const QTextCharFormat& base, const QString& id)
{
    QTextCharFormat f = base;
    f.setProperty(PropFootnoteRef, id);
    f.setVerticalAlignment(QTextCharFormat::AlignSuperScript);
    f.setForeground(linkColor());
    return f;
}

// ---- images ---------------------------------------------------------------

// Registers an image resource (real file, or a placeholder if it can't be loaded).
inline QSize ensureImageResource(QTextDocument* doc, const QString& url, const QString& alt)
{
    const QUrl u(url);
    QImage img;
    if (!u.scheme().startsWith(QLatin1String("http"))) {
        const QUrl resolved = doc->baseUrl().resolved(u);
        img = QImage(resolved.isLocalFile() ? resolved.toLocalFile() : url);
    }
    if (img.isNull()) {
        const QFont f = QGuiApplication::font();
        const QFontMetrics fm(f);
        const QString label = QStringLiteral("[image] ") + (alt.isEmpty() ? url : alt);
        img = QImage(fm.horizontalAdvance(label) + 24, fm.height() + 16, QImage::Format_ARGB32);
        img.fill(codeBg());
        QPainter p(&img);
        p.setPen(mutedColor());
        p.setFont(f);
        p.drawText(img.rect(), Qt::AlignCenter, label);
    }
    doc->addResource(QTextDocument::ImageResource, u, img);
    return img.size();
}

// ---- block styling --------------------------------------------------------

inline void restyleBlock(QTextBlock b)
{
    QTextBlockFormat bf = b.blockFormat();
    const int q = bf.intProperty(PropQuoteLevel);
    const bool code = bf.boolProperty(PropCodeBlock);
    const bool rule = bf.boolProperty(PropIsRule);
    const bool foot = bf.hasProperty(PropFootnoteDef);

    qreal top = 0, bottom = 8, left = q * kQuoteIndent;
    if (code) {
        const QTextBlock n = b.next();
        bottom = (n.isValid() && n.blockFormat().boolProperty(PropCodeBlock)) ? 0 : 8;
        left += 12;
    } else if (rule) {
        top = 10;
        bottom = 10;
    } else if (bf.headingLevel() > 0) {
        top = 12;
        bottom = 6;
    } else if (b.textList()) {
        bottom = 3;
    } else if (foot) {
        left += 36;
        bottom = 4;
    }
    if (q > 0) {
        top = qMax<qreal>(top, 4);
        bottom = qMax<qreal>(bottom, 4);
    }
    bf.setTopMargin(top);
    bf.setBottomMargin(bottom);
    bf.setLeftMargin(left);
    if (code)
        bf.setBackground(blockBg());
    else if (q > 0)
        bf.setBackground(quoteBg());
    else
        bf.clearBackground();

    QTextCursor c(b);
    c.setBlockFormat(bf);
}

inline void restyleAll(QTextDocument* doc)
{
    for (QTextBlock b = doc->begin(); b.isValid(); b = b.next())
        restyleBlock(b);
}

inline Qt::Alignment alignFor(const QString& a)
{
    if (a == QLatin1String("c"))
        return Qt::AlignHCenter;
    if (a == QLatin1String("r"))
        return Qt::AlignRight;
    return Qt::AlignLeft;
}

// Creates a table at the cursor. aligns[c] in {"", "l", "c", "r"}.
inline QTextTable* createTable(QTextCursor& cur, int rows, int cols, const QStringList& aligns = { })
{
    QTextTableFormat tf;
    tf.setBorder(1);
    tf.setBorderBrush(borderColor());
    tf.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
    tf.setCellPadding(6);
    tf.setCellSpacing(0);
    tf.setHeaderRowCount(1);
    QTextTable* t = cur.insertTable(rows, cols, tf);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            QTextTableCell cell = t->cellAt(r, c);
            QTextCursor cc = cell.firstCursorPosition();
            QTextBlockFormat bf;
            const QString a = aligns.value(c);
            bf.setProperty(PropColAlign, a);
            bf.setAlignment(alignFor(a));
            cc.setBlockFormat(bf);
            QTextCharFormat f = baseCharFormat();
            if (r == 0)
                setBold(f, true);
            cc.setBlockCharFormat(f);
            if (r == 0) {
                QTextTableCellFormat cf = cell.format().toTableCellFormat();
                cf.setBackground(headerBg());
                cell.setFormat(cf);
            }
        }
    }
    return t;
}

// ---- theme changes --------------------------------------------------------

inline void recolorTables(QTextFrame* frame)
{
    const auto children = frame->childFrames();
    for (QTextFrame* child : children) {
        if (auto* t = qobject_cast<QTextTable*>(child)) {
            QTextTableFormat tf = t->format();
            tf.setBorderBrush(borderColor());
            t->setFormat(tf);
            for (int r = 0; r < tf.headerRowCount() && r < t->rows(); ++r) {
                for (int c = 0; c < t->columns(); ++c) {
                    QTextTableCell cell = t->cellAt(r, c);
                    QTextTableCellFormat cf = cell.format().toTableCellFormat();
                    cf.setBackground(headerBg());
                    cell.setFormat(cf);
                }
            }
        }
        recolorTables(child);
    }
}

// Re-applies theme colours to existing content. This clears the undo history,
// but does not change the document's "modified" state.
inline void recolorDocument(QTextDocument* doc)
{
    const bool modified = doc->isModified();
    doc->setUndoRedoEnabled(false);

    struct Span {
        int position;
        int length;
        QTextCharFormat format;
    };
    QVector<Span> spans;
    for (QTextBlock b = doc->begin(); b.isValid(); b = b.next()) {
        for (QTextBlock::iterator it = b.begin(); !it.atEnd(); ++it) {
            const QTextFragment fr = it.fragment();
            if (fr.isValid())
                spans.push_back({ fr.position(), fr.length(), fr.charFormat() });
        }
    }

    QTextCursor c(doc);
    c.beginEditBlock();
    for (const Span& span : std::as_const(spans)) {
        QTextCharFormat f = span.format;
        if (f.isImageFormat()) {
            const QTextImageFormat im = f.toImageFormat();
            ensureImageResource(doc, im.name(), im.stringProperty(PropAlt));
            continue;
        }
        f.clearForeground();
        f.clearBackground();
        if ((f.isAnchor() && !f.anchorHref().isEmpty()) || f.hasProperty(PropFootnoteRef))
            f.setForeground(linkColor());
        if (f.boolProperty(PropInlineCode))
            f.setBackground(codeBg());
        else if (f.boolProperty(PropHighlight))
            f.setBackground(highlightBg());
        c.setPosition(span.position);
        c.setPosition(span.position + span.length, QTextCursor::KeepAnchor);
        c.setCharFormat(f);
    }
    for (QTextBlock b = doc->begin(); b.isValid(); b = b.next()) {
        QTextCharFormat f = b.charFormat();
        if (!QTextCursor(b).currentTable()
            && (f.hasProperty(QTextFormat::ForegroundBrush) || f.hasProperty(QTextFormat::BackgroundBrush))) {
            f.clearForeground();
            f.clearBackground();
            QTextCursor(b).setBlockCharFormat(f);
        }
    }
    restyleAll(doc);
    recolorTables(doc->rootFrame());
    c.endEditBlock();

    doc->setUndoRedoEnabled(true);
    doc->setModified(modified);
    doc->markContentsDirty(0, doc->characterCount());
}

} // namespace md

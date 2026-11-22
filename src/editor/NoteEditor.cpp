#include "NoteEditor.h"
#include "CodeHighlighter.h"
#include "markdown/MarkdownParser.h"
#include "markdown/MarkdownWriter.h"
#include "markdown/MdFormat.h"

#include <QAbstractTextDocumentLayout>
#include <QDesktopServices>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMimeData>
#include <QPainter>
#include <QScrollBar>
#include <QSet>
#include <QTextDocumentFragment>
#include <QTextLayout>
#include <QTextList>
#include <QTextTable>
#include <QVariantMap>

using namespace md;
namespace {
constexpr int kPageWidth = 760; // width of the writing area in pixels
constexpr int kPageGap = 16; // minimum background visible around the page
const QColor kDeskColor(0x16, 0x18, 0x1c); // application background
const QColor kPageColor(0x24, 0x28, 0x2f); // lighter writing area
}

NoteEditor::NoteEditor(QWidget* parent)
    : QTextEdit(parent)
{
    setAcceptRichText(true);
    setFrameShape(QFrame::NoFrame);
    // The viewport is the "page"; the area around it is the darker background.
    QPalette pal = palette();
    pal.setColor(QPalette::Window, kDeskColor);
    pal.setColor(QPalette::Base, kPageColor);
    setPalette(pal);
    setBackgroundRole(QPalette::Window);
    setAutoFillBackground(true);
    viewport()->setBackgroundRole(QPalette::Base);
    viewport()->setAutoFillBackground(true);

    setLineWrapMode(QTextEdit::WidgetWidth);
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere); // also wraps long URLs
    // Always shown, so the page doesn't change width when a note starts scrolling.
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    QFont f = font();
    f.setPointSizeF(kBasePt);
    document()->setDefaultFont(f);
    document()->setDocumentMargin(28);
    new CodeHighlighter(document());

    connect(this, &QTextEdit::cursorPositionChanged, this, &NoteEditor::stateChanged);
    connect(this, &QTextEdit::currentCharFormatChanged, this, &NoteEditor::stateChanged);
    importMarkdown(QString());
}

QString NoteEditor::exportMarkdown() const { return MarkdownWriter(m_opts).write(document()); }

void NoteEditor::importMarkdown(const QString& markdown)
{
    MarkdownParser::parse(markdown, document());
    setTextCursor(QTextCursor(document()));
}

void NoteEditor::setBaseDirectory(const QString& dir)
{
    document()->setBaseUrl(QUrl::fromLocalFile(dir + QLatin1Char('/')));
}

int NoteEditor::headingLevel() const { return textCursor().blockFormat().headingLevel(); }
bool NoteEditor::isSetextHeading() const { return textCursor().blockFormat().boolProperty(PropSetext); }

QVector<QTextBlock> NoteEditor::selectedBlocks() const
{
    const QTextCursor c = textCursor();
    QTextBlock b = document()->findBlock(c.selectionStart());
    QTextBlock last = document()->findBlock(c.selectionEnd());
    if (c.hasSelection() && last != b && last.position() == c.selectionEnd())
        last = last.previous();
    QVector<QTextBlock> v;
    for (; b.isValid(); b = b.next()) {
        v << b;
        if (b == last)
            break;
    }
    return v;
}

// ---------------------------------------------------------------- inline

void NoteEditor::toggleBold(bool on)
{
    QTextCharFormat f;
    setBold(f, on);
    mergeCurrentCharFormat(f);
}
void NoteEditor::toggleItalic(bool on)
{
    QTextCharFormat f;
    f.setFontItalic(on);
    mergeCurrentCharFormat(f);
}
void NoteEditor::toggleStrike(bool on)
{
    QTextCharFormat f;
    f.setFontStrikeOut(on);
    mergeCurrentCharFormat(f);
}
void NoteEditor::toggleHighlight(bool on)
{
    QTextCharFormat f;
    setHighlight(f, on);
    mergeCurrentCharFormat(f);
}
void NoteEditor::toggleInlineCode(bool on)
{
    QTextCharFormat f;
    setInlineCode(f, on);
    mergeCurrentCharFormat(f);
}
void NoteEditor::toggleSubscript(bool on)
{
    QTextCharFormat f;
    f.setVerticalAlignment(on ? QTextCharFormat::AlignSubScript : QTextCharFormat::AlignNormal);
    mergeCurrentCharFormat(f);
}
void NoteEditor::toggleSuperscript(bool on)
{
    QTextCharFormat f;
    f.setVerticalAlignment(on ? QTextCharFormat::AlignSuperScript : QTextCharFormat::AlignNormal);
    mergeCurrentCharFormat(f);
}

void NoteEditor::clearFormatting()
{
    QTextCharFormat f;
    setBold(f, headingLevel() > 0);
    f.setFontItalic(false);
    f.setFontStrikeOut(false);
    setHighlight(f, false);
    setInlineCode(f, false);
    f.setVerticalAlignment(QTextCharFormat::AlignNormal);
    clearLink(f);
    mergeCurrentCharFormat(f);
}

// ---------------------------------------------------------------- blocks

void NoteEditor::setHeading(int level, bool setext)
{
    QTextCursor c = textCursor();
    c.beginEditBlock();
    for (QTextBlock b : selectedBlocks()) {
        const int prev = b.blockFormat().headingLevel();
        QTextBlockFormat bf = b.blockFormat();
        bf.setProperty(PropCodeBlock, QVariant(false));
        bf.setHeadingLevel(level);
        if (setext && level > 0 && level <= 2)
            bf.setProperty(PropSetext, QVariant(true));
        else
            bf.clearProperty(PropSetext);
        QTextCursor bc(b);
        bc.setBlockFormat(bf);

        QTextCharFormat sz;
        sz.setFontPointSize(level > 0 ? kBasePt * headingScale(level) : kBasePt);
        if (level > 0 || prev > 0)
            sz.setFontWeight(level > 0 ? QFont::Bold : QFont::Normal);
        bc.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        bc.mergeCharFormat(sz);
        QTextCharFormat bcf = b.charFormat();
        bcf.merge(sz);
        QTextCursor(b).setBlockCharFormat(bcf);
        restyleBlock(b);
    }
    c.endEditBlock();
    emit stateChanged();
}

void NoteEditor::insertHardBreak()
{
    textCursor().insertText(QString(QChar(QChar::LineSeparator)));
}

void NoteEditor::insertRule(const QString& style)
{
    QTextCursor c = textCursor();
    c.beginEditBlock();
    c.movePosition(QTextCursor::EndOfBlock);
    if (!c.block().text().isEmpty())
        c.insertBlock(QTextBlockFormat(), baseCharFormat());
    QTextBlockFormat bf;
    bf.setProperty(PropIsRule, QVariant(true));
    bf.setProperty(PropRuleStyle, style);
    c.setBlockFormat(bf);
    restyleBlock(c.block());
    c.insertBlock(QTextBlockFormat(), baseCharFormat());
    restyleBlock(c.block());
    c.endEditBlock();
    setTextCursor(c);
}

void NoteEditor::toggleList(QTextListFormat::Style style, bool task)
{
    const auto blocks = selectedBlocks();
    if (blocks.isEmpty())
        return;
    QTextCursor c = textCursor();
    c.beginEditBlock();

    QTextList* cur = blocks.first().textList();
    const bool curTask = blocks.first().blockFormat().marker() != QTextBlockFormat::MarkerType::NoMarker;
    const bool ordered = style == QTextListFormat::ListDecimal;
    const bool same = cur && curTask == task
        && (ordered ? cur->format().style() == style : isBulletStyle(cur->format().style()));

    if (same) {
        for (QTextBlock b : blocks) {
            if (QTextList* l = b.textList()) {
                l->remove(b);
                QTextBlockFormat bf = b.blockFormat();
                bf.setIndent(0);
                bf.setMarker(QTextBlockFormat::MarkerType::NoMarker);
                QTextCursor(b).setBlockFormat(bf);
                restyleBlock(b);
            }
        }
    } else {
        QTextList* list = nullptr;
        QTextListFormat lf;
        lf.setIndent(cur ? cur->format().indent() : 1);
        lf.setStyle(ordered ? style : bulletStyle(lf.indent()));
        for (QTextBlock b : blocks) {
            QTextBlockFormat bf = b.blockFormat();
            bf.setMarker(task ? (curTask && b.blockFormat().marker() == QTextBlockFormat::MarkerType::Checked
                                        ? QTextBlockFormat::MarkerType::Checked
                                        : QTextBlockFormat::MarkerType::Unchecked)
                              : QTextBlockFormat::MarkerType::NoMarker);
            QTextCursor bc(b);
            bc.setBlockFormat(bf);
            if (!list)
                list = bc.createList(lf);
            else
                list->add(b);
            restyleBlock(b);
        }
    }
    c.endEditBlock();
    emit stateChanged();
}

void NoteEditor::shiftListItem(int delta)
{
    QTextCursor c = textCursor();
    c.beginEditBlock();
    for (QTextBlock b : selectedBlocks()) {
        QTextList* l = b.textList();
        if (!l)
            continue;
        QTextListFormat lf = l->format();
        const int nl = lf.indent() + delta;
        if (nl > kMaxListIndent)
            continue;
        if (nl < 1) {
            l->remove(b);
            QTextBlockFormat bf = b.blockFormat();
            bf.setIndent(0);
            bf.setMarker(QTextBlockFormat::MarkerType::NoMarker);
            QTextCursor(b).setBlockFormat(bf);
            restyleBlock(b);
            continue;
        }
        lf.setIndent(nl);
        if (isBulletStyle(lf.style()))
            lf.setStyle(bulletStyle(nl));
        QTextList* target = nullptr;
        for (QTextBlock p = b.previous(); p.isValid(); p = p.previous()) {
            QTextList* pl = p.textList();
            if (!pl)
                break;
            const int pi = pl->format().indent();
            if (pi == nl && pl->format().style() == lf.style()) {
                target = pl;
                break;
            }
            if (pi < nl)
                break;
        }
        if (target)
            target->add(b);
        else
            QTextCursor(b).createList(lf);
        restyleBlock(b);
    }
    c.endEditBlock();
}

void NoteEditor::changeQuote(int delta)
{
    QTextCursor c = textCursor();
    c.beginEditBlock();
    for (QTextBlock b : selectedBlocks()) {
        QTextBlockFormat bf = b.blockFormat();
        const int q = qMax(0, bf.intProperty(PropQuoteLevel) + delta);
        if (q > 0)
            bf.setProperty(PropQuoteLevel, QVariant(q));
        else
            bf.clearProperty(PropQuoteLevel);
        QTextCursor(b).setBlockFormat(bf);
        restyleBlock(b);
    }
    c.endEditBlock();
}

void NoteEditor::toggleCodeBlock()
{
    const auto blocks = selectedBlocks();
    bool all = true;
    for (const QTextBlock& b : blocks)
        all = all && b.blockFormat().boolProperty(PropCodeBlock);

    QString lang;
    if (!all) {
        bool ok = false;
        lang = QInputDialog::getText(this, tr("Code block"), tr("Language (optional, e.g. javascript):"),
            QLineEdit::Normal, m_lastLang, &ok)
                   .trimmed();
        if (!ok)
            return;
        m_lastLang = lang;
    }

    QTextCursor c = textCursor();
    c.beginEditBlock();
    for (QTextBlock b : blocks) {
        QTextBlockFormat bf = b.blockFormat();
        QTextCharFormat cf;
        if (all) {
            bf.clearProperty(PropCodeBlock);
            bf.clearProperty(PropCodeLang);
            bf.clearProperty(PropCodeFence);
            cf.setFontFamilies(QStringList { baseFamily() });
            cf.setFontFixedPitch(false);
            cf.setFontPointSize(kBasePt);
        } else {
            bf.setHeadingLevel(0);
            bf.clearProperty(PropSetext);
            bf.setProperty(PropCodeBlock, QVariant(true));
            bf.setProperty(PropCodeLang, lang);
            bf.setProperty(PropCodeFence, QStringLiteral("```"));
            cf = codeBlockCharFormat();
        }
        QTextCursor bc(b);
        bc.setBlockFormat(bf);
        bc.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        bc.mergeCharFormat(cf);
        QTextCharFormat bcf = b.charFormat();
        bcf.merge(cf);
        QTextCursor(b).setBlockCharFormat(bcf);
    }
    for (QTextBlock b : blocks)
        restyleBlock(b);
    c.endEditBlock();
}

void NoteEditor::setCodeLanguage()
{
    const auto blocks = selectedBlocks();
    bool ok = false;
    const QString lang = QInputDialog::getText(this, tr("Code language"), tr("Language:"),
        QLineEdit::Normal, m_lastLang, &ok)
                             .trimmed();
    if (!ok)
        return;
    m_lastLang = lang;
    for (QTextBlock b : blocks) {
        if (!b.blockFormat().boolProperty(PropCodeBlock))
            continue;
        QTextBlockFormat bf = b.blockFormat();
        bf.setProperty(PropCodeLang, lang);
        QTextCursor(b).setBlockFormat(bf);
    }
    document()->markContentsDirty(0, document()->characterCount()); // re-highlight
}

// ---------------------------------------------------------------- links

void NoteEditor::insertStyledLink(int kind, const QString& url, const QString& ref, const QString& label)
{
    QTextCursor c = textCursor();
    const QTextCharFormat prev = currentCharFormat();
    QTextCharFormat f = prev;
    setLink(f, url, kind, ref);
    c.insertText(label, f);
    setTextCursor(c);
    QTextCharFormat plain = prev;
    clearLink(plain);
    setCurrentCharFormat(plain);
}

void NoteEditor::insertLink()
{
    bool ok = false;
    const QString url = QInputDialog::getText(this, tr("Insert link"), tr("URL:"),
        QLineEdit::Normal, QString(), &ok)
                            .trimmed();
    if (!ok || url.isEmpty())
        return;
    const QTextCursor c = textCursor();
    insertStyledLink(LinkInline, url, QString(), c.hasSelection() ? c.selectedText() : url);
}

void NoteEditor::insertReferenceLink()
{
    bool ok = false;
    const QString id = QInputDialog::getText(this, tr("Reference link"), tr("Reference id:"),
        QLineEdit::Normal, QStringLiteral("ref1"), &ok)
                           .trimmed();
    if (!ok || id.isEmpty())
        return;
    QVariantMap refs = document()->property("md.refs").toMap();
    if (!refs.contains(id)) {
        const QString url = QInputDialog::getText(this, tr("Reference link"), tr("URL for [%1]:").arg(id),
            QLineEdit::Normal, QString(), &ok)
                                .trimmed();
        if (!ok || url.isEmpty())
            return;
        refs.insert(id, url);
        document()->setProperty("md.refs", refs);
    }
    const QTextCursor c = textCursor();
    insertStyledLink(LinkReference, refs.value(id).toString(), id, c.hasSelection() ? c.selectedText() : id);
}

void NoteEditor::insertAutolink()
{
    bool ok = false;
    const QString url = QInputDialog::getText(this, tr("Autolink"), tr("URL or e-mail:"),
        QLineEdit::Normal, QStringLiteral("https://"), &ok)
                            .trimmed();
    if (!ok || url.isEmpty())
        return;
    insertStyledLink(LinkAuto, url, QString(), url);
}

void NoteEditor::removeLink()
{
    QTextCursor c = textCursor();
    if (!c.hasSelection()) {
        // extend over the whole anchor under the cursor
        const QString href = currentCharFormat().anchorHref();
        if (href.isEmpty())
            return;
        c.select(QTextCursor::WordUnderCursor);
    }
    QTextCharFormat f;
    clearLink(f);
    c.mergeCharFormat(f);
}

void NoteEditor::insertImage()
{
    bool ok = false;
    const QString url = QInputDialog::getText(this, tr("Insert image"), tr("Image path or URL:"),
        QLineEdit::Normal, QString(), &ok)
                            .trimmed();
    if (!ok || url.isEmpty())
        return;
    const QString alt = QInputDialog::getText(this, tr("Insert image"), tr("Alt text:"),
        QLineEdit::Normal, QString(), &ok)
                            .trimmed();
    if (!ok)
        return;
    QTextImageFormat im;
    im.setName(url);
    im.setProperty(PropAlt, alt);
    const QSize sz = ensureImageResource(document(), url, alt);
    if (sz.width() > 640)
        im.setWidth(640);
    textCursor().insertImage(im);
}

void NoteEditor::insertFootnote()
{
    bool ok = false;
    const QString text = QInputDialog::getText(this, tr("Footnote"), tr("Footnote text:"),
        QLineEdit::Normal, QString(), &ok);
    if (!ok)
        return;

    int n = 1;
    QSet<QString> used;
    for (QTextBlock b = document()->begin(); b.isValid(); b = b.next())
        if (b.blockFormat().hasProperty(PropFootnoteDef))
            used.insert(b.blockFormat().stringProperty(PropFootnoteDef));
    while (used.contains(QString::number(n)))
        ++n;
    const QString id = QString::number(n);

    QTextCursor c = textCursor();
    const QTextCharFormat prev = currentCharFormat();
    c.beginEditBlock();
    c.insertText(id, footnoteRefFormat(baseCharFormat(), id));

    QTextCursor e(document());
    e.movePosition(QTextCursor::End);
    QTextBlockFormat bf;
    bf.setProperty(PropFootnoteDef, id);
    if (!e.block().text().isEmpty() || e.block().textList())
        e.insertBlock(bf, baseCharFormat());
    else {
        e.setBlockFormat(bf);
        e.setBlockCharFormat(baseCharFormat());
    }
    e.insertText(text, baseCharFormat());
    restyleBlock(e.block());
    c.endEditBlock();
    setTextCursor(c);
    setCurrentCharFormat(prev);
}

// ---------------------------------------------------------------- tables

void NoteEditor::insertTable()
{
    bool ok = false;
    const int rows = QInputDialog::getInt(this, tr("Insert table"), tr("Rows (incl. header):"), 3, 1, 100, 1, &ok);
    if (!ok)
        return;
    const int cols = QInputDialog::getInt(this, tr("Insert table"), tr("Columns:"), 3, 1, 20, 1, &ok);
    if (!ok)
        return;
    QTextCursor c = textCursor();
    c.beginEditBlock();
    c.movePosition(QTextCursor::EndOfBlock);
    if (!c.block().text().isEmpty() || c.currentTable())
        c.insertBlock(QTextBlockFormat(), baseCharFormat());
    QTextTable* t = createTable(c, rows, cols);
    c.endEditBlock();
    setTextCursor(t->cellAt(0, 0).firstCursorPosition());
}

void NoteEditor::setColumnAlignment(const QString& a)
{
    QTextTable* t = textCursor().currentTable();
    if (!t)
        return;
    const int col = t->cellAt(textCursor()).column();
    textCursor().beginEditBlock();
    for (int r = 0; r < t->rows(); ++r) {
        QTextCursor cc = t->cellAt(r, col).firstCursorPosition();
        QTextBlockFormat bf = cc.blockFormat();
        bf.setProperty(PropColAlign, a);
        bf.setAlignment(alignFor(a));
        cc.setBlockFormat(bf);
    }
    textCursor().endEditBlock();
}

void NoteEditor::nextCell(bool forward)
{
    QTextTable* t = textCursor().currentTable();
    if (!t)
        return;
    const QTextTableCell cell = t->cellAt(textCursor());
    int r = cell.row(), c = cell.column() + (forward ? 1 : -1);
    if (c >= t->columns()) {
        c = 0;
        ++r;
    }
    if (c < 0) {
        c = t->columns() - 1;
        --r;
    }
    if (r < 0)
        return;
    if (r >= t->rows()) {
        t->appendRows(1);
        for (int k = 0; k < t->columns(); ++k) {
            QTextCursor cc = t->cellAt(r, k).firstCursorPosition();
            QTextBlockFormat bf = cc.blockFormat();
            const QString a = t->cellAt(0, k).firstCursorPosition().blockFormat().stringProperty(PropColAlign);
            bf.setProperty(PropColAlign, a);
            bf.setAlignment(alignFor(a));
            cc.setBlockFormat(bf);
            cc.setBlockCharFormat(baseCharFormat());
        }
    }
    setTextCursor(t->cellAt(r, c).firstCursorPosition());
}

// ---------------------------------------------------------------- events

void NoteEditor::keyPressEvent(QKeyEvent* e)
{
    QTextCursor c = textCursor();
    const QTextBlock b = c.block();
    const QTextBlockFormat bf = b.blockFormat();
    const bool inCode = bf.boolProperty(PropCodeBlock);
    const int quote = bf.intProperty(PropQuoteLevel);

    if (e->text() == QLatin1String(" ")
        && !(e->modifiers() & (Qt::ControlModifier | Qt::AltModifier))
        && tryAutoBullet())
        return;
    switch (e->key()) {
    case Qt::Key_Tab:
        if (e->modifiers() & Qt::ControlModifier)
            break;
        if (c.currentTable()) {
            nextCell(true);
            return;
        }
        if (b.textList()) {
            shiftListItem(+1);
            return;
        }
        if (inCode) {
            c.insertText(QStringLiteral("    "));
            return;
        }
        break;
    case Qt::Key_Backtab:
        if (c.currentTable()) {
            nextCell(false);
            return;
        }
        if (b.textList()) {
            shiftListItem(-1);
            return;
        }
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter: {
        if (e->modifiers() & Qt::ShiftModifier) {
            insertHardBreak();
            return;
        }
        if (inCode) {
            const QTextBlock n = b.next();
            const bool lastLine = !(n.isValid() && n.blockFormat().boolProperty(PropCodeBlock));
            if (b.text().isEmpty() && lastLine) { // Enter on an empty last line leaves the block
                QTextBlockFormat nb = bf;
                nb.clearProperty(PropCodeBlock);
                nb.clearProperty(PropCodeLang);
                nb.clearProperty(PropCodeFence);
                QTextCursor bc(b);
                bc.setBlockFormat(nb);
                bc.setBlockCharFormat(baseCharFormat());
                restyleBlock(b);
                setCurrentCharFormat(baseCharFormat());
                return;
            }
            break;
        }
        if (b.textList() && b.text().isEmpty()) {
            shiftListItem(-1);
            return;
        }
        if (quote > 0 && b.text().isEmpty() && !b.textList()) {
            changeQuote(-1);
            return;
        }
        const bool special = bf.headingLevel() > 0 || bf.boolProperty(PropIsRule) || bf.hasProperty(PropFootnoteDef);
        if (special && c.atBlockEnd() && !c.hasSelection()) {
            QTextBlockFormat nb;
            if (quote > 0)
                nb.setProperty(PropQuoteLevel, QVariant(quote));
            c.insertBlock(nb, baseCharFormat());
            restyleBlock(c.block());
            setTextCursor(c);
            return;
        }
        QTextEdit::keyPressEvent(e);
        QTextBlock nb = textCursor().block();
        if (nb.blockFormat().marker() == QTextBlockFormat::MarkerType::Checked && nb.text().isEmpty()) {
            QTextBlockFormat f = nb.blockFormat();
            f.setMarker(QTextBlockFormat::MarkerType::Unchecked);
            QTextCursor(nb).setBlockFormat(f);
        }
        restyleBlock(nb);
        return;
    }
    default:
        break;
    }
    QTextEdit::keyPressEvent(e);
}

void NoteEditor::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        const QTextBlock b = cursorForPosition(e->pos()).block();
        if (b.isValid() && b.textList() && b.blockFormat().marker() != QTextBlockFormat::MarkerType::NoMarker) {
            const QRectF r = document()->documentLayout()->blockBoundingRect(b).translated(-horizontalScrollBar()->value(), -verticalScrollBar()->value());
            const QPoint p = e->pos();
            if (p.x() < r.left() && p.x() > r.left() - 36 && p.y() >= r.top() && p.y() <= r.bottom()) {
                QTextBlockFormat bf = b.blockFormat();
                bf.setMarker(bf.marker() == QTextBlockFormat::MarkerType::Checked
                        ? QTextBlockFormat::MarkerType::Unchecked
                        : QTextBlockFormat::MarkerType::Checked);
                QTextCursor(b).setBlockFormat(bf);
                e->accept();
                return;
            }
        }
    }
    QTextEdit::mousePressEvent(e);
}

void NoteEditor::mouseReleaseEvent(QMouseEvent* e)
{
    QTextEdit::mouseReleaseEvent(e);
    if (e->button() == Qt::LeftButton && (e->modifiers() & Qt::ControlModifier)) {
        const QString href = anchorAt(e->pos());
        if (!href.isEmpty())
            QDesktopServices::openUrl(QUrl(href));
    }
}

void NoteEditor::paintEvent(QPaintEvent* ev)
{
    QTextEdit::paintEvent(ev);

    QPainter p(viewport());
    const QPointF off(horizontalScrollBar()->value(), verticalScrollBar()->value());
    const qreal margin = document()->documentMargin();
    QAbstractTextDocumentLayout* layout = document()->documentLayout();

    for (QTextBlock b = document()->begin(); b.isValid(); b = b.next()) {
        const QRectF r = layout->blockBoundingRect(b).translated(-off);
        if (r.bottom() < 0 || r.top() > viewport()->height())
            continue;
        const QTextBlockFormat bf = b.blockFormat();
        if (const QTextList* list = b.textList(); list
            && bf.marker() == QTextBlockFormat::MarkerType::NoMarker
            && list->format().style() == QTextListFormat::ListSquare
            && isOutlinedSquareLevel(list->format().indent()))
            paintOutlinedSquare(p, b, r);

        const int q = bf.intProperty(PropQuoteLevel);
        for (int k = 0; k < q; ++k)
            p.fillRect(QRectF(margin + 4 + k * kQuoteIndent - off.x(), r.top() - 3, 3, r.height() + 6),
                QColor(0x4c, 0x7c, 0xc9));

        if (bf.boolProperty(PropIsRule)) {
            p.setPen(QPen(mutedColor(), 1));
            const qreal y = r.center().y();
            p.drawLine(QPointF(margin, y), QPointF(viewport()->width() - margin, y));
        }
        if (bf.hasProperty(PropFootnoteDef)) {
            p.setPen(mutedColor());
            p.setFont(document()->defaultFont());
            p.drawText(QPointF(margin + q * kQuoteIndent - off.x(), r.top() + QFontMetricsF(p.font()).ascent()),
                QStringLiteral("[%1]").arg(bf.stringProperty(PropFootnoteDef)));
        }
    }
}

QMimeData* NoteEditor::createMimeDataFromSelection() const
{
    QTextDocument tmp;
    tmp.setProperty("md.refs", document()->property("md.refs"));
    QTextCursor(&tmp).insertFragment(textCursor().selection());
    const QString markdown = MarkdownWriter(m_opts).write(&tmp);
    auto* m = new QMimeData;
    m->setText(markdown);
    m->setData(QStringLiteral("text/markdown"), markdown.toUtf8());
    return m;
}

void NoteEditor::insertFromMimeData(const QMimeData* src)
{
    if (!src->hasText()) {
        QTextEdit::insertFromMimeData(src);
        return;
    }
    QTextDocument tmp;
    tmp.setBaseUrl(document()->baseUrl());
    MarkdownParser::parse(src->text(), &tmp);

    QVariantMap refs = document()->property("md.refs").toMap();
    const QVariantMap extra = tmp.property("md.refs").toMap();
    for (auto it = extra.begin(); it != extra.end(); ++it)
        refs.insert(it.key(), it.value());
    document()->setProperty("md.refs", refs);

    QTextCursor c = textCursor();
    c.insertFragment(QTextDocumentFragment(&tmp));
    setTextCursor(c);
    ensureCursorVisible();
}

// ---------------------------------------------------------------- page layout
void NoteEditor::updatePageMargins()
{
    const int spare = width() - verticalScrollBar()->sizeHint().width() - kPageWidth;
    const int left = qMax(kPageGap, spare / 2);
    const int right = qMax(kPageGap, spare - spare / 2);
    setViewportMargins(left, kPageGap, right, kPageGap);
}
bool NoteEditor::event(QEvent* e)
{
    const bool handled = QTextEdit::event(e);
    // This is the resize of the whole widget. (resizeEvent() reports the viewport.)
    if (e->type() == QEvent::Resize)
        updatePageMargins();
    return handled;
}
// ---------------------------------------------------------------- auto bullets
// Called when Space is pressed. Turns a leading "- " into a bullet, or nests
// the item one level deeper if it is already in a list.
bool NoteEditor::tryAutoBullet()
{
    QTextCursor c = textCursor();
    if (c.hasSelection() || c.positionInBlock() != 1 || c.currentTable())
        return false;
    const QTextBlock b = c.block();
    const QTextBlockFormat bf = b.blockFormat();
    if (!b.text().startsWith(QLatin1Char('-')) || bf.boolProperty(PropCodeBlock)
        || bf.headingLevel() > 0 || bf.boolProperty(PropIsRule) || bf.hasProperty(PropFootnoteDef))
        return false;
    const QTextList* list = b.textList();
    if (list && list->format().indent() >= kMaxListIndent)
        return false;

    // Insert the space as a separate step, so Ctrl+Z restores a literal "- ".
    c.insertText(QStringLiteral(" "));

    c.beginEditBlock();
    c.movePosition(QTextCursor::StartOfBlock);
    c.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 2);
    c.removeSelectedText();
    if (list) {
        shiftListItem(+1);
        // "- " inside a numbered list gives a nested bullet, not a nested number.
        const QTextList* nested = textCursor().block().textList();
        if (nested && !isBulletStyle(nested->format().style()))
            toggleList(QTextListFormat::ListDisc, false);
    } else {
        toggleList(QTextListFormat::ListDisc, false);
    }
    c.endEditBlock();
    return true;
}
void NoteEditor::paintOutlinedSquare(QPainter& p, const QTextBlock& block, const QRectF& blockRect) const
{
    const QTextLayout* tl = block.layout();
    if (tl->lineCount() == 0)
        return;
    // Same geometry Qt uses for its filled square marker.
    const QFontMetrics fm(QFont(block.charFormat().font(), viewport()));
    const int side = fm.lineSpacing() / 3;
    const QPoint text = (blockRect.topLeft() + tl->lineAt(0).naturalTextRect().topLeft()).toPoint();
    const QRectF square(text.x() - fm.horizontalAdvance(QLatin1Char(' ')) - side,
        text.y() + fm.height() / 2 - side / 2.0, side, side);

    p.save();
    p.fillRect(square.adjusted(-1, -1, 1, 1), viewport()->palette().color(QPalette::Base));
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(palette().color(QPalette::Text), 1));
    p.drawRect(square.adjusted(0.5, 0.5, -0.5, -0.5));
    p.restore();
}

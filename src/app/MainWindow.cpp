#include "MainWindow.h"
#include "editor/NoteEditor.h"
#include "markdown/MdFormat.h"

#include <QActionGroup>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPair>
#include <QPlainTextEdit>
#include <QSaveFile>
#include <QStatusBar>
#include <QTextStream>
#include <QToolBar>
#include <QVariant>
#include <functional>

MainWindow::MainWindow()
{
    m_editor = new NoteEditor(this);
    setCentralWidget(m_editor);
    resize(1000, 760);
    buildUi();

    m_preview = new QPlainTextEdit(this);
    m_preview->setReadOnly(true);
    m_preview->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_dock = new QDockWidget(tr("Markdown source"), this);
    m_dock->setWidget(m_preview);
    addDockWidget(Qt::RightDockWidgetArea, m_dock);
    m_dock->hide();

    m_previewTimer.setSingleShot(true);
    m_previewTimer.setInterval(250);
    connect(m_editor->document(), &QTextDocument::contentsChanged, &m_previewTimer, qOverload<>(&QTimer::start));
    connect(&m_previewTimer, &QTimer::timeout, this, &MainWindow::refreshPreview);
    connect(m_dock, &QDockWidget::visibilityChanged, this, &MainWindow::refreshPreview);
    connect(m_editor, &NoteEditor::stateChanged, this, &MainWindow::syncActions);
    connect(m_editor->document(), &QTextDocument::modificationChanged, this, &MainWindow::updateTitle);
    updateTitle();
    syncActions();
}

void MainWindow::buildUi()
{
    auto* tb = new QToolBar(tr("Format"), this);
    tb->setMovable(false);
    tb->setToolButtonStyle(Qt::ToolButtonTextOnly);
    addToolBar(tb);

    // helper: adds an action to a menu (+ optional toolbar)
    auto add = [this, tb](QMenu* m, const QString& text, const QKeySequence& ks, auto&& fn,
                   bool checkable = false, const QString& tbText = QString()) -> QAction* {
        QAction* a = m->addAction(text);
        if (!ks.isEmpty())
            a->setShortcut(ks);
        a->setCheckable(checkable);
        connect(a, &QAction::triggered, this, fn);
        if (!tbText.isEmpty()) {
            QAction* t = a; // share the same action with the toolbar
            t->setIconText(tbText);
            t->setToolTip(text + (ks.isEmpty() ? QString() : QStringLiteral("  (%1)").arg(ks.toString(QKeySequence::NativeText))));
            tb->addAction(t);
        }
        return a;
    };
    NoteEditor* ed = m_editor;

    // ---- File
    QMenu* file = menuBar()->addMenu(tr("&File"));
    add(file, tr("&New"), QKeySequence::New, [this] { newFile(); });
    add(file, tr("&Open…"), QKeySequence::Open, [this] { openFile(); });
    add(file, tr("&Save"), QKeySequence::Save, [this] { saveFile(); });
    add(file, tr("Save &As…"), QKeySequence::SaveAs, [this] { saveFileAs(); });
    file->addSeparator();
    add(file, tr("&Quit"), QKeySequence::Quit, [this] { close(); });

    // ---- Edit
    QMenu* edit = menuBar()->addMenu(tr("&Edit"));
    add(edit, tr("&Undo"), QKeySequence::Undo, [ed] { ed->undo(); });
    add(edit, tr("&Redo"), QKeySequence::Redo, [ed] { ed->redo(); });
    edit->addSeparator();
    add(edit, tr("Cu&t"), QKeySequence::Cut, [ed] { ed->cut(); });
    add(edit, tr("&Copy"), QKeySequence::Copy, [ed] { ed->copy(); });
    add(edit, tr("&Paste"), QKeySequence::Paste, [ed] { ed->paste(); });
    add(edit, tr("Select &All"), QKeySequence::SelectAll, [ed] { ed->selectAll(); });

    // ---- Format (inline)
    QMenu* fmt = menuBar()->addMenu(tr("F&ormat"));
    m_bold = add(fmt, tr("Bold"), QKeySequence(Qt::CTRL | Qt::Key_B), [ed](bool on) { ed->toggleBold(on); }, true, tr("B"));
    m_italic = add(fmt, tr("Italic"), QKeySequence(Qt::CTRL | Qt::Key_I), [ed](bool on) { ed->toggleItalic(on); }, true, tr("I"));
    m_strike = add(fmt, tr("Strikethrough"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_X), [ed](bool on) { ed->toggleStrike(on); }, true, tr("S"));
    m_hl = add(fmt, tr("Highlight"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H), [ed](bool on) { ed->toggleHighlight(on); }, true, tr("HL"));
    m_code = add(fmt, tr("Inline code"), QKeySequence(Qt::CTRL | Qt::Key_QuoteLeft), [ed](bool on) { ed->toggleInlineCode(on); }, true, tr("</>"));
    m_sub = add(fmt, tr("Subscript"), QKeySequence(Qt::CTRL | Qt::Key_Comma), [ed](bool on) { ed->toggleSubscript(on); }, true, tr("x₂"));
    m_sup = add(fmt, tr("Superscript"), QKeySequence(Qt::CTRL | Qt::Key_Period), [ed](bool on) { ed->toggleSuperscript(on); }, true, tr("x²"));
    fmt->addSeparator();
    add(fmt, tr("Clear formatting"), QKeySequence(), [ed] { ed->clearFormatting(); });

    // ---- Paragraph
    QMenu* para = menuBar()->addMenu(tr("&Paragraph"));
    m_heading = new QComboBox(this);
    m_heading->addItems({ tr("Paragraph"), tr("Heading 1"), tr("Heading 2"), tr("Heading 3"), tr("Heading 4"),
        tr("Heading 5"), tr("Heading 6"), tr("Heading 1 (setext ===)"), tr("Heading 2 (setext ---)") });
    tb->insertWidget(tb->actions().isEmpty() ? nullptr : tb->actions().first(), m_heading);
    connect(m_heading, qOverload<int>(&QComboBox::activated), this, [ed](int i) {
        if (i <= 6)
            ed->setHeading(i, false);
        else
            ed->setHeading(i == 7 ? 1 : 2, true);
        ed->setFocus();
    });
    QMenu* headings = para->addMenu(tr("Heading"));
    add(headings, tr("Paragraph"), QKeySequence(Qt::CTRL | Qt::Key_0), [ed] { ed->setHeading(0); });
    for (int i = 1; i <= 6; ++i)
        add(headings, tr("Heading %1").arg(i), QKeySequence(Qt::CTRL | (Qt::Key_0 + i)), [ed, i] { ed->setHeading(i); });
    headings->addSeparator();
    add(headings, tr("Heading 1 (setext)"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_1), [ed] { ed->setHeading(1, true); });
    add(headings, tr("Heading 2 (setext)"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_2), [ed] { ed->setHeading(2, true); });

    para->addSeparator();
    add(para, tr("Hard line break"), QKeySequence(), [ed] { ed->insertHardBreak(); });
    QMenu* rules = para->addMenu(tr("Horizontal rule"));
    add(rules, tr("---"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R), [ed] { ed->insertRule(QStringLiteral("---")); });
    add(rules, tr("***"), QKeySequence(), [ed] { ed->insertRule(QStringLiteral("***")); });
    add(rules, tr("___"), QKeySequence(), [ed] { ed->insertRule(QStringLiteral("___")); });

    para->addSeparator();
    add(para, tr("Bulleted list"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_U), [ed] { ed->toggleList(QTextListFormat::ListDisc, false); }, false, tr("•"));
    add(para, tr("Numbered list"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_O), [ed] { ed->toggleList(QTextListFormat::ListDecimal, false); }, false, tr("1."));
    add(para, tr("Task list"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_T), [ed] { ed->toggleList(QTextListFormat::ListDisc, true); }, false, tr("☑"));
    add(para, tr("Nest item (Tab)"), QKeySequence(), [ed] { ed->shiftListItem(+1); });
    add(para, tr("Un-nest item (Shift+Tab)"), QKeySequence(), [ed] { ed->shiftListItem(-1); });

    para->addSeparator();
    add(para, tr("Blockquote deeper"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Q), [ed] { ed->changeQuote(+1); }, false, tr("❝"));
    add(para, tr("Blockquote shallower"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Q), [ed] { ed->changeQuote(-1); });
    add(para, tr("Code block"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C), [ed] { ed->toggleCodeBlock(); }, false, tr("{ }"));
    add(para, tr("Code block language…"), QKeySequence(), [ed] { ed->setCodeLanguage(); });

    // ---- Insert
    QMenu* ins = menuBar()->addMenu(tr("&Insert"));
    add(ins, tr("Link…"), QKeySequence(Qt::CTRL | Qt::Key_L), [ed] { ed->insertLink(); }, false, tr("🔗"));
    add(ins, tr("Reference link…"), QKeySequence(), [ed] { ed->insertReferenceLink(); });
    add(ins, tr("Autolink…"), QKeySequence(), [ed] { ed->insertAutolink(); });
    add(ins, tr("Remove link"), QKeySequence(), [ed] { ed->removeLink(); });
    add(ins, tr("Image…"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I), [ed] { ed->insertImage(); });
    add(ins, tr("Footnote…"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F), [ed] { ed->insertFootnote(); });
    ins->addSeparator();
    add(ins, tr("Table…"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), [ed] { ed->insertTable(); }, false, tr("▦"));
    QMenu* align = ins->addMenu(tr("Column alignment"));
    add(align, tr("Default"), QKeySequence(), [ed] { ed->setColumnAlignment(QString()); });
    add(align, tr("Left  (:--)"), QKeySequence(), [ed] { ed->setColumnAlignment(QStringLiteral("l")); });
    add(align, tr("Center (:-:)"), QKeySequence(), [ed] { ed->setColumnAlignment(QStringLiteral("c")); });
    add(align, tr("Right (--:)"), QKeySequence(), [ed] { ed->setColumnAlignment(QStringLiteral("r")); });

    // ---- Markdown output options
    QMenu* out = menuBar()->addMenu(tr("&Markdown output"));
    auto group = [this, out](const QString& title, const QVector<QPair<QString, QVariant>>& items,
                     int defaultIndex, std::function<void(const QVariant&)> apply) {
        QMenu* m = out->addMenu(title);
        auto* g = new QActionGroup(m);
        for (int i = 0; i < items.size(); ++i) {
            QAction* a = m->addAction(items[i].first);
            a->setCheckable(true);
            a->setChecked(i == defaultIndex);
            g->addAction(a);
            const QVariant v = items[i].second;
            connect(a, &QAction::triggered, this, [this, apply, v] { apply(v); refreshPreview(); });
        }
    };
    group(tr("Emphasis marker"), { { tr("* and **"), QChar('*') }, { tr("_ and __"), QChar('_') } }, 0,
        [ed](const QVariant& v) { ed->writerOptions().emphasis = v.toChar(); });
    group(tr("Bullet character"), { { tr("-"), QChar('-') }, { tr("*"), QChar('*') }, { tr("+"), QChar('+') } }, 0,
        [ed](const QVariant& v) { ed->writerOptions().bullet = v.toChar(); });
    group(tr("Hard line break"), { { tr("Two trailing spaces"), false }, { tr("Backslash"), true } }, 0,
        [ed](const QVariant& v) { ed->writerOptions().backslashBreak = v.toBool(); });
    group(tr("Nested list indent"), { { tr("4 spaces"), 4 }, { tr("2 spaces"), 2 } }, 0,
        [ed](const QVariant& v) { ed->writerOptions().indentWidth = v.toInt(); });

    // ---- View
    QMenu* view = menuBar()->addMenu(tr("&View"));
    QTimer::singleShot(0, this, [this, view] { view->addAction(m_dock->toggleViewAction()); });
}

void MainWindow::syncActions()
{
    const QTextCharFormat f = m_editor->currentCharFormat();
    m_bold->setChecked(f.fontWeight() >= QFont::Bold);
    m_italic->setChecked(f.fontItalic());
    m_strike->setChecked(f.fontStrikeOut());
    m_hl->setChecked(f.boolProperty(md::PropHighlight));
    m_code->setChecked(f.boolProperty(md::PropInlineCode));
    m_sub->setChecked(f.verticalAlignment() == QTextCharFormat::AlignSubScript);
    m_sup->setChecked(f.verticalAlignment() == QTextCharFormat::AlignSuperScript);

    int idx = m_editor->headingLevel();
    if (idx > 0 && m_editor->isSetextHeading() && idx <= 2)
        idx = idx == 1 ? 7 : 8;
    m_heading->setCurrentIndex(idx);
}

void MainWindow::refreshPreview()
{
    if (m_dock && m_dock->isVisible())
        m_preview->setPlainText(m_editor->exportMarkdown());
}

void MainWindow::updateTitle()
{
    const QString name = m_path.isEmpty() ? tr("Untitled") : QFileInfo(m_path).fileName();
    setWindowTitle(QStringLiteral("%1%2 — mdnote").arg(name, m_editor->document()->isModified() ? "*" : ""));
}

bool MainWindow::maybeSave()
{
    if (!m_editor->document()->isModified())
        return true;
    const auto r = QMessageBox::question(this, tr("Unsaved changes"), tr("Save changes before continuing?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (r == QMessageBox::Save)
        return saveFile();
    return r == QMessageBox::Discard;
}

void MainWindow::newFile()
{
    if (!maybeSave())
        return;
    m_editor->importMarkdown(QString());
    m_path.clear();
    updateTitle();
}

void MainWindow::openFile()
{
    if (!maybeSave())
        return;
    const QString p = QFileDialog::getOpenFileName(this, tr("Open Markdown"), QString(),
        tr("Markdown (*.md *.markdown *.txt);;All files (*)"));
    if (p.isEmpty())
        return;
    QFile f(p);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Open"), tr("Cannot read %1").arg(p));
        return;
    }
    m_editor->setBaseDirectory(QFileInfo(p).absolutePath());
    m_editor->importMarkdown(QString::fromUtf8(f.readAll()));
    m_path = p;
    updateTitle();
}

bool MainWindow::writeTo(const QString& path)
{
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Save"), tr("Cannot write %1").arg(path));
        return false;
    }
    f.write(m_editor->exportMarkdown().toUtf8());
    if (!f.commit())
        return false;
    m_path = path;
    m_editor->document()->setModified(false);
    updateTitle();
    return true;
}

bool MainWindow::saveFile() { return m_path.isEmpty() ? saveFileAs() : writeTo(m_path); }

bool MainWindow::saveFileAs()
{
    const QString p = QFileDialog::getSaveFileName(this, tr("Save Markdown"), m_path,
        tr("Markdown (*.md);;All files (*)"));
    return p.isEmpty() ? false : writeTo(p);
}

void MainWindow::closeEvent(QCloseEvent* e)
{
    if (maybeSave())
        e->accept();
    else
        e->ignore();
}

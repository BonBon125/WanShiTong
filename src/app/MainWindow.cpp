#include "MainWindow.h"
#include "FileExplorer.h"
#include "FolderView.h"
#include "WorkspaceDialog.h"
#include "editor/NoteEditor.h"
#include "markdown/MdFormat.h"
#include "theme/Icons.h"

#include <QAction>
#include <QActionGroup>
#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPair>
#include <QPlainTextEdit>
#include <QSaveFile>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QVariant>
#include <functional>

MainWindow::MainWindow(const QString& workspace)
{
    resize(1200, 800);
    buildLayout();
    buildActions();

    m_previewTimer.setSingleShot(true);
    m_previewTimer.setInterval(250);
    connect(m_editor->document(), &QTextDocument::contentsChanged, &m_previewTimer, qOverload<>(&QTimer::start));
    connect(&m_previewTimer, &QTimer::timeout, this, &MainWindow::refreshPreview);
    connect(m_dock, &QDockWidget::visibilityChanged, this, &MainWindow::refreshPreview);
    connect(m_editor, &NoteEditor::stateChanged, this, &MainWindow::syncActions);
    connect(m_editor->document(), &QTextDocument::modificationChanged, this, &MainWindow::updateTitle);

    connect(m_explorer, &FileExplorer::noteActivated, this, &MainWindow::openNote);
    connect(m_explorer, &FileExplorer::folderActivated, this, &MainWindow::showFolder);
    connect(m_explorer, &FileExplorer::newNoteRequested, this, &MainWindow::newNote);
    connect(m_explorer, &FileExplorer::newFolderRequested, this, &MainWindow::newFolder);
    connect(m_folderView, &FolderView::noteActivated, this, &MainWindow::openNote);
    connect(m_folderView, &FolderView::folderActivated, this, &MainWindow::showFolder);
    connect(m_folderView, &FolderView::newNoteRequested, this, &MainWindow::newNote);
    connect(m_folderView, &FolderView::newFolderRequested, this, &MainWindow::newFolder);
    connect(m_pathBar, &QLabel::linkActivated, this, &MainWindow::showFolder, Qt::QueuedConnection);

    QSettings settings;
    restoreGeometry(settings.value("window/geometry").toByteArray());
    m_splitter->restoreState(settings.value("window/splitter").toByteArray());
    const bool sidebar = settings.value("window/sidebar", true).toBool();
    m_sidebarAction->setChecked(sidebar);
    m_explorer->setVisible(sidebar);

    setWorkspace(workspace);
    syncActions();
}

void MainWindow::buildLayout()
{
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(1);
    m_splitter->setChildrenCollapsible(false);

    m_explorer = new FileExplorer(m_splitter);

    auto* content = new QWidget(m_splitter);
    content->setObjectName("contentArea");
    content->setAttribute(Qt::WA_StyledBackground);

    m_pathBar = new QLabel(content);
    m_pathBar->setObjectName("pathBar");
    m_pathBar->setTextFormat(Qt::RichText);

    m_stack = new QStackedWidget(content);
    m_editor = new NoteEditor(m_stack);
    m_folderView = new FolderView(m_stack);
    m_stack->addWidget(m_editor);
    m_stack->addWidget(m_folderView);

    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_pathBar);
    layout->addWidget(m_stack, 1);

    m_splitter->addWidget(m_explorer);
    m_splitter->addWidget(content);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({ 240, 960 });
    setCentralWidget(m_splitter);

    m_preview = new QPlainTextEdit(this);
    m_preview->setReadOnly(true);
    m_preview->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_dock = new QDockWidget(tr("Markdown source"), this);
    m_dock->setWidget(m_preview);
    addDockWidget(Qt::RightDockWidgetArea, m_dock);
    m_dock->hide();
}

void MainWindow::buildActions()
{
    using icons::Glyph;
    NoteEditor* ed = m_editor;

    auto* tb = new QToolBar(tr("Toolbar"), this);
    tb->setMovable(false);
    tb->setIconSize(QSize(18, 18));
    tb->setToolButtonStyle(Qt::ToolButtonIconOnly);
    tb->setContextMenuPolicy(Qt::PreventContextMenu);
    addToolBar(tb);

    // Adds an action to a menu.
    auto add = [this](QMenu* m, const QString& text, const QKeySequence& ks, auto fn) -> QAction* {
        QAction* a = m->addAction(text);
        if (!ks.isEmpty())
            a->setShortcut(ks);
        connect(a, &QAction::triggered, this, fn);
        return a;
    };
    // Same, for actions that only make sense while a note is open.
    auto addEd = [this, &add](QMenu* m, const QString& text, const QKeySequence& ks, auto fn) -> QAction* {
        QAction* a = add(m, text, ks, fn);
        m_editorActions << a;
        return a;
    };
    // Gives an action an icon and a tooltip (call before adding to the toolbar).
    auto decorate = [](QAction* a, Glyph g) {
        a->setIcon(icons::get(g));
        a->setIconVisibleInMenu(false);
        QString tip = a->text();
        tip.remove(QLatin1Char('&'));
        if (!a->shortcut().isEmpty())
            tip += QStringLiteral("  (%1)").arg(a->shortcut().toString(QKeySequence::NativeText));
        a->setToolTip(tip);
    };
    auto tool = [tb, &decorate](QAction* a, Glyph g) {
        decorate(a, g);
        tb->addAction(a);
    };

    // ---- File
    QMenu* file = menuBar()->addMenu(tr("&File"));
    QAction* newNoteAction = add(file, tr("&New note"), QKeySequence::New, [this] { newNote(); });
    QAction* newFolderAction = add(file, tr("New &folder"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), [this] { newFolder(); });
    add(file, tr("&Open file…"), QKeySequence::Open, [this] { openFileDialog(); });
    add(file, tr("Switch notes &directory…"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O), [this] { switchWorkspace(); });
    file->addSeparator();
    addEd(file, tr("&Save"), QKeySequence::Save, [this] { saveFile(); });
    addEd(file, tr("Save &As…"), QKeySequence::SaveAs, [this] { saveFileAs(); });
    file->addSeparator();
    add(file, tr("&Quit"), QKeySequence::Quit, [this] { close(); });

    // ---- Edit
    QMenu* edit = menuBar()->addMenu(tr("&Edit"));
    addEd(edit, tr("&Undo"), QKeySequence::Undo, [ed] { ed->undo(); });
    addEd(edit, tr("&Redo"), QKeySequence::Redo, [ed] { ed->redo(); });
    edit->addSeparator();
    addEd(edit, tr("Cu&t"), QKeySequence::Cut, [ed] { ed->cut(); });
    addEd(edit, tr("&Copy"), QKeySequence::Copy, [ed] { ed->copy(); });
    addEd(edit, tr("&Paste"), QKeySequence::Paste, [ed] { ed->paste(); });
    addEd(edit, tr("Select &All"), QKeySequence::SelectAll, [ed] { ed->selectAll(); });

    // ---- Format (inline)
    QMenu* fmt = menuBar()->addMenu(tr("F&ormat"));
    m_bold = addEd(fmt, tr("Bold"), QKeySequence(Qt::CTRL | Qt::Key_B), [ed](bool on) { ed->toggleBold(on); });
    m_italic = addEd(fmt, tr("Italic"), QKeySequence(Qt::CTRL | Qt::Key_I), [ed](bool on) { ed->toggleItalic(on); });
    m_strike = addEd(fmt, tr("Strikethrough"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_X), [ed](bool on) { ed->toggleStrike(on); });
    m_hl = addEd(fmt, tr("Highlight"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H), [ed](bool on) { ed->toggleHighlight(on); });
    m_code = addEd(fmt, tr("Inline code"), QKeySequence(Qt::CTRL | Qt::Key_QuoteLeft), [ed](bool on) { ed->toggleInlineCode(on); });
    m_sub = addEd(fmt, tr("Subscript"), QKeySequence(Qt::CTRL | Qt::Key_Comma), [ed](bool on) { ed->toggleSubscript(on); });
    m_sup = addEd(fmt, tr("Superscript"), QKeySequence(Qt::CTRL | Qt::Key_Period), [ed](bool on) { ed->toggleSuperscript(on); });
    for (QAction* a : { m_bold, m_italic, m_strike, m_hl, m_code, m_sub, m_sup })
        a->setCheckable(true);
    fmt->addSeparator();
    addEd(fmt, tr("Clear formatting"), QKeySequence(), [ed] { ed->clearFormatting(); });

    // ---- Paragraph
    QMenu* para = menuBar()->addMenu(tr("&Paragraph"));
    QMenu* headings = para->addMenu(tr("Heading"));
    addEd(headings, tr("Paragraph"), QKeySequence(Qt::CTRL | Qt::Key_0), [ed] { ed->setHeading(0); });
    for (int i = 1; i <= 6; ++i)
        addEd(headings, tr("Heading %1").arg(i), QKeySequence(Qt::CTRL | (Qt::Key_0 + i)), [ed, i] { ed->setHeading(i); });
    headings->addSeparator();
    addEd(headings, tr("Heading 1 (setext)"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_1), [ed] { ed->setHeading(1, true); });
    addEd(headings, tr("Heading 2 (setext)"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_2), [ed] { ed->setHeading(2, true); });
    para->addSeparator();
    addEd(para, tr("Hard line break"), QKeySequence(), [ed] { ed->insertHardBreak(); });
    QMenu* rules = para->addMenu(tr("Horizontal rule"));
    addEd(rules, tr("---"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R), [ed] { ed->insertRule(QStringLiteral("---")); });
    addEd(rules, tr("***"), QKeySequence(), [ed] { ed->insertRule(QStringLiteral("***")); });
    addEd(rules, tr("___"), QKeySequence(), [ed] { ed->insertRule(QStringLiteral("___")); });
    para->addSeparator();
    QAction* bullets = addEd(para, tr("Bulleted list"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_U), [ed] { ed->toggleList(QTextListFormat::ListDisc, false); });
    QAction* numbers = addEd(para, tr("Numbered list"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_O), [ed] { ed->toggleList(QTextListFormat::ListDecimal, false); });
    QAction* tasks = addEd(para, tr("Task list"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_T), [ed] { ed->toggleList(QTextListFormat::ListDisc, true); });
    addEd(para, tr("Nest item (Tab)"), QKeySequence(), [ed] { ed->shiftListItem(+1); });
    addEd(para, tr("Un-nest item (Shift+Tab)"), QKeySequence(), [ed] { ed->shiftListItem(-1); });
    para->addSeparator();
    QAction* quote = addEd(para, tr("Blockquote deeper"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Q), [ed] { ed->changeQuote(+1); });
    addEd(para, tr("Blockquote shallower"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Q), [ed] { ed->changeQuote(-1); });
    QAction* codeBlock = addEd(para, tr("Code block"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C), [ed] { ed->toggleCodeBlock(); });
    addEd(para, tr("Code block language…"), QKeySequence(), [ed] { ed->setCodeLanguage(); });

    // ---- Insert
    QMenu* ins = menuBar()->addMenu(tr("&Insert"));
    QAction* link = addEd(ins, tr("Link…"), QKeySequence(Qt::CTRL | Qt::Key_L), [ed] { ed->insertLink(); });
    addEd(ins, tr("Reference link…"), QKeySequence(), [ed] { ed->insertReferenceLink(); });
    addEd(ins, tr("Autolink…"), QKeySequence(), [ed] { ed->insertAutolink(); });
    addEd(ins, tr("Remove link"), QKeySequence(), [ed] { ed->removeLink(); });
    QAction* image = addEd(ins, tr("Image…"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I), [ed] { ed->insertImage(); });
    addEd(ins, tr("Footnote…"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F), [ed] { ed->insertFootnote(); });
    ins->addSeparator();
    QAction* table = addEd(ins, tr("Table…"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), [ed] { ed->insertTable(); });
    QMenu* align = ins->addMenu(tr("Column alignment"));
    addEd(align, tr("Default"), QKeySequence(), [ed] { ed->setColumnAlignment(QString()); });
    addEd(align, tr("Left  (:--)"), QKeySequence(), [ed] { ed->setColumnAlignment(QStringLiteral("l")); });
    addEd(align, tr("Center (:-:)"), QKeySequence(), [ed] { ed->setColumnAlignment(QStringLiteral("c")); });
    addEd(align, tr("Right (--:)"), QKeySequence(), [ed] { ed->setColumnAlignment(QStringLiteral("r")); });

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
    m_sidebarAction = view->addAction(tr("File explorer"));
    m_sidebarAction->setCheckable(true);
    m_sidebarAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Backslash));
    connect(m_sidebarAction, &QAction::toggled, m_explorer, &QWidget::setVisible);
    QAction* source = m_dock->toggleViewAction();
    source->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_M));
    view->addAction(source);

    // ---- Toolbar
    tool(m_sidebarAction, Glyph::Sidebar);
    tool(newNoteAction, Glyph::NewNote);
    tool(newFolderAction, Glyph::NewFolder);
    tb->addSeparator();

    m_heading = new QComboBox(tb);
    m_heading->addItems({ tr("Paragraph"), tr("Heading 1"), tr("Heading 2"), tr("Heading 3"), tr("Heading 4"),
        tr("Heading 5"), tr("Heading 6"), tr("Heading 1 (setext ===)"), tr("Heading 2 (setext ---)") });
    m_heading->setFocusPolicy(Qt::NoFocus);
    connect(m_heading, qOverload<int>(&QComboBox::activated), this, [ed](int i) {
        if (i <= 6)
            ed->setHeading(i, false);
        else
            ed->setHeading(i == 7 ? 1 : 2, true);
        ed->setFocus();
    });
    tb->addWidget(m_heading);
    tb->addSeparator();

    tool(m_bold, Glyph::Bold);
    tool(m_italic, Glyph::Italic);
    tool(m_strike, Glyph::Strike);
    tool(m_hl, Glyph::Highlight);
    tool(m_code, Glyph::Code);
    tool(m_sub, Glyph::Subscript);
    tool(m_sup, Glyph::Superscript);
    tb->addSeparator();
    tool(bullets, Glyph::BulletList);
    tool(numbers, Glyph::NumberList);
    tool(tasks, Glyph::TaskList);
    tool(quote, Glyph::Quote);
    tool(codeBlock, Glyph::CodeBlock);
    tb->addSeparator();
    tool(link, Glyph::Link);
    tool(image, Glyph::Image);
    tool(table, Glyph::Table);

    auto* spacer = new QWidget(tb);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tb->addWidget(spacer);
    tool(source, Glyph::Source);
}

// ---------------------------------------------------------------- navigation

void MainWindow::setWorkspace(const QString& dir)
{
    m_root = QDir(dir).absolutePath();
    WorkspaceDialog::remember(m_root);
    m_explorer->setRootPath(m_root);
    displayFolder(m_root);
}

bool MainWindow::isEditing() const { return m_stack->currentWidget() == m_editor; }

QString MainWindow::currentFolder() const { return m_folder.isEmpty() ? m_root : m_folder; }

void MainWindow::setEditing(bool editing)
{
    if (editing)
        m_stack->setCurrentWidget(m_editor);
    else
        m_stack->setCurrentWidget(m_folderView);
    for (QAction* a : std::as_const(m_editorActions))
        a->setEnabled(editing);
    m_heading->setEnabled(editing);
    if (editing)
        m_editor->setFocus();
    updatePathBar();
    updateTitle();
    refreshPreview();
}

bool MainWindow::openNote(const QString& path)
{
    if (isEditing() && path == m_path)
        return true;
    if (!maybeSave()) {
        m_explorer->select(m_path);
        return false;
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Open"), tr("Cannot read %1").arg(path));
        return false;
    }
    m_editor->setBaseDirectory(QFileInfo(path).absolutePath());
    m_editor->importMarkdown(QString::fromUtf8(f.readAll()));
    m_path = path;
    m_folder = QFileInfo(path).absolutePath();
    setEditing(true);
    m_explorer->select(path);
    syncActions();
    return true;
}

bool MainWindow::showFolder(const QString& dir)
{
    if (!maybeSave()) {
        m_explorer->select(m_path);
        return false;
    }
    displayFolder(dir);
    return true;
}

void MainWindow::displayFolder(const QString& dir)
{
    // Clear the editor so discarded changes don't trigger another prompt later.
    m_editor->importMarkdown(QString());
    m_path.clear();
    m_folder = dir;
    m_folderView->showFolder(dir);
    setEditing(false);
    m_explorer->select(dir);
}

void MainWindow::updatePathBar()
{
    const QString target = isEditing() ? m_path : m_folder;
    const QDir root(m_root);
    const QString relative = root.relativeFilePath(target);

    auto current = [](const QString& name) {
        return QStringLiteral("<span style='color:#dde1e6'>%1</span>").arg(name.toHtmlEscaped());
    };
    auto crumb = [](const QString& path, const QString& name) {
        return QStringLiteral("<a href=\"%1\" style='color:#8b95a1; text-decoration:none'>%2</a>")
            .arg(path.toHtmlEscaped(), name.toHtmlEscaped());
    };

    // A file opened from outside the notes directory.
    if (relative == QLatin1String("..") || relative.startsWith(QLatin1String("../")) || QDir::isAbsolutePath(relative)) {
        m_pathBar->setText(current(QDir::toNativeSeparators(target)));
        return;
    }

    QStringList parts;
    if (relative != QLatin1String("."))
        parts = relative.split(QLatin1Char('/'), Qt::SkipEmptyParts);

    const QString rootName = QFileInfo(m_root).fileName();
    QStringList html;
    html << (parts.isEmpty() ? current(rootName) : crumb(m_root, rootName));
    QString path = m_root;
    for (int i = 0; i < parts.size(); ++i) {
        path = QDir(path).filePath(parts[i]);
        if (i + 1 < parts.size())
            html << crumb(path, parts[i]);
        else
            html << current(isEditing() ? QFileInfo(parts[i]).completeBaseName() : parts[i]);
    }
    m_pathBar->setText(html.join(QStringLiteral("<span style='color:#565f6b'>&nbsp;&nbsp;/&nbsp;&nbsp;</span>")));
}

// ---------------------------------------------------------------- files

QString MainWindow::askName(const QString& title, const QString& label)
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, title, label, QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok || name.isEmpty())
        return QString();
    if (name.contains(QLatin1Char('/')) || name.contains(QLatin1Char('\\')) || name.startsWith(QLatin1Char('.'))) {
        QMessageBox::warning(this, title, tr("Names can't contain slashes or start with a dot."));
        return QString();
    }
    return name;
}

void MainWindow::newNote(const QString& dir)
{
    const QString parent = dir.isEmpty() ? currentFolder() : dir;
    QString name = askName(tr("New note"), tr("Note name:"));
    if (name.isEmpty())
        return;
    if (!name.endsWith(QLatin1String(".md"), Qt::CaseInsensitive))
        name += QLatin1String(".md");

    const QString path = QDir(parent).filePath(name);
    if (QFileInfo::exists(path)) {
        QMessageBox::warning(this, tr("New note"), tr("%1 already exists.").arg(name));
        return;
    }
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        QMessageBox::warning(this, tr("New note"), tr("Cannot create %1").arg(path));
        return;
    }
    f.close();
    openNote(path);
}

void MainWindow::newFolder(const QString& dir)
{
    const QString parent = dir.isEmpty() ? currentFolder() : dir;
    const QString name = askName(tr("New folder"), tr("Folder name:"));
    if (name.isEmpty())
        return;
    if (!QDir(parent).mkdir(name)) {
        QMessageBox::warning(this, tr("New folder"), tr("Cannot create %1. Does it already exist?").arg(name));
        return;
    }
    showFolder(QDir(parent).filePath(name));
}

void MainWindow::openFileDialog()
{
    const QString p = QFileDialog::getOpenFileName(this, tr("Open Markdown"), currentFolder(),
        tr("Markdown (*.md *.markdown *.txt);;All files (*)"));
    if (!p.isEmpty())
        openNote(p);
}

void MainWindow::switchWorkspace()
{
    if (!maybeSave())
        return;
    // Saved or discarded: either way, nothing is pending any more.
    m_editor->document()->setModified(false);
    WorkspaceDialog chooser(this);
    if (chooser.exec() == QDialog::Accepted)
        setWorkspace(chooser.selectedPath());
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
    m_folder = QFileInfo(path).absolutePath();
    m_editor->setBaseDirectory(m_folder);
    m_editor->document()->setModified(false);
    updatePathBar();
    updateTitle();
    statusBar()->showMessage(tr("Saved"), 2000);
    return true;
}

bool MainWindow::saveFile()
{
    if (!isEditing())
        return true;
    return m_path.isEmpty() ? saveFileAs() : writeTo(m_path);
}

bool MainWindow::saveFileAs()
{
    if (!isEditing())
        return true;
    const QString p = QFileDialog::getSaveFileName(this, tr("Save Markdown"), m_path,
        tr("Markdown (*.md);;All files (*)"));
    if (p.isEmpty() || !writeTo(p))
        return false;
    m_explorer->select(p);
    return true;
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

// ---------------------------------------------------------------- state

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
    if (m_dock->isVisible())
        m_preview->setPlainText(isEditing() ? m_editor->exportMarkdown() : QString());
}

void MainWindow::updateTitle()
{
    const bool editing = isEditing();
    const QString name = QFileInfo(editing ? m_path : m_folder).fileName();
    const bool modified = editing && m_editor->document()->isModified();
    setWindowTitle(QStringLiteral("%1%2 — mdnote").arg(name, modified ? "*" : ""));
}

void MainWindow::closeEvent(QCloseEvent* e)
{
    if (!maybeSave()) {
        e->ignore();
        return;
    }
    QSettings settings;
    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/splitter", m_splitter->saveState());
    settings.setValue("window/sidebar", m_sidebarAction->isChecked());
    e->accept();
}

#include "MainWindow.h"
#include "WelcomeDialog.h"
#include "editor/NoteEditor.h"
#include "explorer/FileExplorer.h"
#include "explorer/FolderView.h"
#include "markdown/MdFormat.h"
#include "theme/Themes.h"
#include "ui/Icons.h"
#include <QActionGroup>
#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QInputDialog>
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
#include <QToolButton>
#include <QVariant>
#include <functional>

namespace {
const char* const kAutoSaveKey = "autoSave";
constexpr int kAutoSaveDelayMs = 1000;

QString cleanPath(const QString& path)
{
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

// True if `path` is `target` itself or lives somewhere inside it.
bool isAffected(const QString& path, const QString& target)
{
    return !path.isEmpty() && (path == target || path.startsWith(target + QLatin1Char('/')));
}
} // namespace

MainWindow::MainWindow(const QString& rootDirectory)
{
    m_autoSave = QSettings().value(QLatin1String(kAutoSaveKey), true).toBool();

    m_explorer = new FileExplorer;
    m_editor = new NoteEditor;
    m_folderView = new FolderView;

    m_stack = new QStackedWidget;
    m_stack->addWidget(m_folderView);
    m_stack->addWidget(m_editor);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_explorer);
    splitter->addWidget(m_stack);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(1);
    splitter->setSizes({ 260, 940 });
    setCentralWidget(splitter);

    m_preview = new QPlainTextEdit(this);
    m_preview->setReadOnly(true);
    m_preview->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_dock = new QDockWidget(tr("Markdown source"), this);
    m_dock->setWidget(m_preview);
    m_dock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    addDockWidget(Qt::RightDockWidgetArea, m_dock);
    m_dock->hide();

    buildUi();
    resize(1200, 800);

    m_previewTimer.setSingleShot(true);
    m_previewTimer.setInterval(250);
    m_autoSaveTimer.setSingleShot(true);
    m_autoSaveTimer.setInterval(kAutoSaveDelayMs);
    connect(m_editor->document(), &QTextDocument::contentsChanged, &m_previewTimer, qOverload<>(&QTimer::start));
    connect(m_editor->document(), &QTextDocument::contentsChanged, &m_autoSaveTimer, qOverload<>(&QTimer::start));
    connect(&m_previewTimer, &QTimer::timeout, this, &MainWindow::refreshPreview);
    connect(&m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::autoSave);
    connect(m_dock, &QDockWidget::visibilityChanged, this, &MainWindow::refreshPreview);
    connect(m_editor, &NoteEditor::stateChanged, this, &MainWindow::syncActions);
    connect(m_editor->document(), &QTextDocument::modificationChanged, this, &MainWindow::updateTitle);

    connect(m_explorer, &FileExplorer::openRequested, this, &MainWindow::openPath);
    connect(m_explorer, &FileExplorer::newNoteRequested, this, &MainWindow::createNote);
    connect(m_explorer, &FileExplorer::newFolderRequested, this, &MainWindow::createFolder);
    connect(m_explorer, &FileExplorer::renameRequested, this, &MainWindow::renamePath);
    connect(m_explorer, &FileExplorer::deleteRequested, this, &MainWindow::deletePath);

    // Queued: these actions rebuild the card list that emitted the signal.
    connect(m_folderView, &FolderView::openRequested, this, &MainWindow::openPath, Qt::QueuedConnection);
    connect(m_folderView, &FolderView::newNoteRequested, this, &MainWindow::createNote, Qt::QueuedConnection);
    connect(m_folderView, &FolderView::newFolderRequested, this, &MainWindow::createFolder, Qt::QueuedConnection);
    connect(m_folderView, &FolderView::renameRequested, this, &MainWindow::renamePath, Qt::QueuedConnection);
    connect(m_folderView, &FolderView::deleteRequested, this, &MainWindow::deletePath, Qt::QueuedConnection);

    setRoot(rootDirectory);
    syncActions();
}

void MainWindow::buildUi()
{
    NoteEditor* ed = m_editor;
    bool noteOnly = false;

    // helper: adds an action to a menu
    auto add = [this, &noteOnly](QMenu* m, const QString& text, const QKeySequence& ks, auto&& fn,
                   bool checkable = false) -> QAction* {
        QAction* a = m->addAction(text);
        if (!ks.isEmpty())
            a->setShortcut(ks);
        a->setCheckable(checkable);
        connect(a, &QAction::triggered, this, fn);
        if (noteOnly)
            m_noteActions << a;
        return a;
    };
    // helper: gives an action an icon and a tooltip for the toolbar
    auto decorate = [](QAction* a, Glyph glyph, const QString& tip) {
        a->setIcon(makeIcon(glyph));
        const QString keys = a->shortcut().toString(QKeySequence::NativeText);
        a->setToolTip(keys.isEmpty() ? tip : QStringLiteral("%1  (%2)").arg(tip, keys));
    };

    // ---- File
    QMenu* file = menuBar()->addMenu(tr("&File"));
    QAction* newNote = add(file, tr("&New note…"), QKeySequence::New,
        [this] { createNote(targetDirectory()); });
    add(file, tr("New &folder…"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N),
        [this] { createFolder(targetDirectory()); });
    add(file, tr("&Switch notes directory…"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O),
        [this] { switchRoot(); });
    file->addSeparator();
    QAction* save = add(file, tr("&Save"), QKeySequence::Save, [this] { saveFile(); });
    m_noteActions << save;
    QAction* autoSaveAction = add(file, tr("&Auto-save"), QKeySequence(), [this](bool on) {
        m_autoSave = on;
        QSettings().setValue(QLatin1String(kAutoSaveKey), on);
        autoSave(); }, true);
    autoSaveAction->setChecked(m_autoSave);
    file->addSeparator();
    // No shortcuts here: F2 and Delete belong to the explorer and folder view.
    m_renameAction = add(file, tr("&Rename…"), QKeySequence(), [this] { renamePath(currentItem()); });
    m_deleteAction = add(file, tr("&Delete…"), QKeySequence(), [this] { deletePath(currentItem()); });
    file->addSeparator();
    add(file, tr("&Quit"), QKeySequence::Quit, [this] { close(); });

    noteOnly = true;

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
    m_bold = add(fmt, tr("Bold"), QKeySequence(Qt::CTRL | Qt::Key_B), [ed](bool on) { ed->toggleBold(on); }, true);
    m_italic = add(fmt, tr("Italic"), QKeySequence(Qt::CTRL | Qt::Key_I), [ed](bool on) { ed->toggleItalic(on); }, true);
    m_strike = add(fmt, tr("Strikethrough"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_X), [ed](bool on) { ed->toggleStrike(on); }, true);
    m_hl = add(fmt, tr("Highlight"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H), [ed](bool on) { ed->toggleHighlight(on); }, true);
    m_code = add(fmt, tr("Inline code"), QKeySequence(Qt::CTRL | Qt::Key_QuoteLeft), [ed](bool on) { ed->toggleInlineCode(on); }, true);
    m_sub = add(fmt, tr("Subscript"), QKeySequence(Qt::CTRL | Qt::Key_Comma), [ed](bool on) { ed->toggleSubscript(on); }, true);
    m_sup = add(fmt, tr("Superscript"), QKeySequence(Qt::CTRL | Qt::Key_Period), [ed](bool on) { ed->toggleSuperscript(on); }, true);
    fmt->addSeparator();
    add(fmt, tr("Clear formatting"), QKeySequence(), [ed] { ed->clearFormatting(); });

    // ---- Paragraph
    QMenu* para = menuBar()->addMenu(tr("&Paragraph"));
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
    QAction* rule = add(rules, tr("---"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R), [ed] { ed->insertRule(QStringLiteral("---")); });
    add(rules, tr("***"), QKeySequence(), [ed] { ed->insertRule(QStringLiteral("***")); });
    add(rules, tr("___"), QKeySequence(), [ed] { ed->insertRule(QStringLiteral("___")); });
    para->addSeparator();
    QAction* bullets = add(para, tr("Bulleted list"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_U), [ed] { ed->toggleList(QTextListFormat::ListDisc, false); });
    QAction* numbers = add(para, tr("Numbered list"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_O), [ed] { ed->toggleList(QTextListFormat::ListDecimal, false); });
    QAction* tasks = add(para, tr("Task list"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_T), [ed] { ed->toggleList(QTextListFormat::ListDisc, true); });
    add(para, tr("Nest item (Tab)"), QKeySequence(), [ed] { ed->shiftListItem(+1); });
    add(para, tr("Un-nest item (Shift+Tab)"), QKeySequence(), [ed] { ed->shiftListItem(-1); });
    para->addSeparator();
    QAction* quote = add(para, tr("Blockquote deeper"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Q), [ed] { ed->changeQuote(+1); });
    add(para, tr("Blockquote shallower"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Q), [ed] { ed->changeQuote(-1); });
    QAction* codeBlock = add(para, tr("Code block"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C), [ed] { ed->toggleCodeBlock(); });
    add(para, tr("Code block language…"), QKeySequence(), [ed] { ed->setCodeLanguage(); });

    // ---- Insert
    QMenu* ins = menuBar()->addMenu(tr("&Insert"));
    QAction* link = add(ins, tr("Link…"), QKeySequence(Qt::CTRL | Qt::Key_L), [ed] { ed->insertLink(); });
    add(ins, tr("Reference link…"), QKeySequence(), [ed] { ed->insertReferenceLink(); });
    add(ins, tr("Autolink…"), QKeySequence(), [ed] { ed->insertAutolink(); });
    add(ins, tr("Remove link"), QKeySequence(), [ed] { ed->removeLink(); });
    QAction* image = add(ins, tr("Image…"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I), [ed] { ed->insertImage(); });
    add(ins, tr("Footnote…"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F), [ed] { ed->insertFootnote(); });
    ins->addSeparator();
    QAction* table = add(ins, tr("Table…"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), [ed] { ed->insertTable(); });
    QMenu* align = ins->addMenu(tr("Column alignment"));
    add(align, tr("Default"), QKeySequence(), [ed] { ed->setColumnAlignment(QString()); });
    add(align, tr("Left  (:--)"), QKeySequence(), [ed] { ed->setColumnAlignment(QStringLiteral("l")); });
    add(align, tr("Center (:-:)"), QKeySequence(), [ed] { ed->setColumnAlignment(QStringLiteral("c")); });
    add(align, tr("Right (--:)"), QKeySequence(), [ed] { ed->setColumnAlignment(QStringLiteral("r")); });

    noteOnly = false;

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
    QAction* explorer = view->addAction(tr("File explorer"));
    explorer->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Backslash));
    explorer->setCheckable(true);
    explorer->setChecked(true);
    connect(explorer, &QAction::toggled, m_explorer, &QWidget::setVisible);
    QAction* source = m_dock->toggleViewAction();
    view->addAction(source);
    view->addSeparator();
    QMenu* themes = view->addMenu(tr("Theme"));
    auto* themeGroup = new QActionGroup(themes);
    for (const Theme& theme : availableThemes()) {
        const QString id = QString::fromLatin1(theme.id);
        QAction* a = themes->addAction(QString::fromLatin1(theme.name));
        a->setCheckable(true);
        a->setChecked(id == QLatin1String(activeTheme().id));
        themeGroup->addAction(a);
        connect(a, &QAction::triggered, this, [this, id] { changeTheme(id); });
    }

    // ---- Toolbar
    decorate(explorer, Glyph::Sidebar, tr("Toggle file explorer"));
    decorate(newNote, Glyph::NewNote, tr("New note"));
    decorate(save, Glyph::Save, tr("Save"));
    decorate(m_renameAction, Glyph::Rename, tr("Rename"));
    decorate(m_deleteAction, Glyph::Trash, tr("Delete"));
    decorate(m_bold, Glyph::Bold, tr("Bold"));
    decorate(m_italic, Glyph::Italic, tr("Italic"));
    decorate(m_strike, Glyph::Strike, tr("Strikethrough"));
    decorate(m_hl, Glyph::Highlight, tr("Highlight"));
    decorate(m_code, Glyph::Code, tr("Inline code"));
    decorate(m_sub, Glyph::Subscript, tr("Subscript"));
    decorate(m_sup, Glyph::Superscript, tr("Superscript"));
    decorate(bullets, Glyph::BulletList, tr("Bulleted list"));
    decorate(numbers, Glyph::NumberList, tr("Numbered list"));
    decorate(tasks, Glyph::TaskList, tr("Task list"));
    decorate(quote, Glyph::Quote, tr("Blockquote"));
    decorate(codeBlock, Glyph::CodeBlock, tr("Code block"));
    decorate(link, Glyph::Link, tr("Insert link"));
    decorate(image, Glyph::Image, tr("Insert image"));
    decorate(table, Glyph::Table, tr("Insert table"));
    decorate(rule, Glyph::Rule, tr("Horizontal rule"));
    decorate(source, Glyph::Source, tr("Show Markdown source"));

    m_heading = new QComboBox(this);
    m_heading->addItems({ tr("Paragraph"), tr("Heading 1"), tr("Heading 2"), tr("Heading 3"), tr("Heading 4"),
        tr("Heading 5"), tr("Heading 6"), tr("Heading 1 (setext)"), tr("Heading 2 (setext)") });
    m_heading->setFocusPolicy(Qt::NoFocus);
    m_heading->setMinimumWidth(150);
    connect(m_heading, qOverload<int>(&QComboBox::activated), this, [ed](int i) {
        if (i <= 6)
            ed->setHeading(i, false);
        else
            ed->setHeading(i == 7 ? 1 : 2, true);
        ed->setFocus();
    });

    auto* tb = new QToolBar(tr("Format"), this);
    tb->setMovable(false);
    tb->setContextMenuPolicy(Qt::PreventContextMenu);
    tb->setToolButtonStyle(Qt::ToolButtonIconOnly);
    tb->setIconSize(QSize(20, 20));
    addToolBar(tb);

    tb->addActions({ explorer, newNote, save, m_renameAction, m_deleteAction });
    tb->addSeparator();
    tb->addWidget(m_heading);
    tb->addSeparator();
    tb->addActions({ m_bold, m_italic, m_strike, m_hl, m_code, m_sub, m_sup });
    tb->addSeparator();
    tb->addActions({ bullets, numbers, tasks, quote, codeBlock });
    tb->addSeparator();
    tb->addActions({ link, image, table, rule });
    auto* spacer = new QWidget(tb);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tb->addWidget(spacer);
    auto* themeButton = new QToolButton(tb);
    themeButton->setIcon(makeIcon(Glyph::Theme));
    themeButton->setIconSize(QSize(20, 20));
    themeButton->setToolTip(tr("Colour theme"));
    themeButton->setMenu(themes);
    themeButton->setPopupMode(QToolButton::InstantPopup);
    tb->addWidget(themeButton);
    tb->addAction(source);

    statusBar()->setSizeGripEnabled(false);
}

void MainWindow::changeTheme(const QString& id)
{
    useTheme(id);
    m_editor->applyTheme();
    m_folderView->applyTheme();
}

// ---------------------------------------------------------------- navigation

void MainWindow::setRoot(const QString& directory)
{
    m_root = cleanPath(directory);
    m_explorer->setRoot(m_root);
    m_folderView->setRoot(m_root);
    showFolder(m_root);
}

void MainWindow::switchRoot()
{
    WelcomeDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted || !closeNote())
        return;
    setRoot(dialog.selectedPath());
}

void MainWindow::openPath(const QString& path)
{
    const QFileInfo info(path);
    if (info.isDir())
        showFolder(cleanPath(path));
    else if (info.isFile())
        openNote(cleanPath(path));
}

void MainWindow::openNote(const QString& path)
{
    if (path != m_path) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, tr("Open"), tr("Cannot read %1").arg(path));
            return;
        }
        const QString markdown = QString::fromUtf8(f.readAll());
        if (!closeNote()) {
            m_explorer->select(m_path);
            return;
        }
        m_editor->setBaseDirectory(QFileInfo(path).absolutePath());
        m_editor->importMarkdown(markdown);
        m_path = path;
    }
    m_folder = QFileInfo(path).absolutePath();
    m_stack->setCurrentWidget(m_editor);
    setNoteActionsEnabled(true);
    updateItemActions();
    m_explorer->select(path);
    m_editor->setFocus();
    updateTitle();
    syncActions();
    refreshPreview();
}

void MainWindow::showFolder(const QString& path)
{
    if (!closeNote()) {
        m_explorer->select(m_path);
        return;
    }
    m_folder = path;
    m_folderView->showFolder(path);
    m_stack->setCurrentWidget(m_folderView);
    setNoteActionsEnabled(false);
    updateItemActions();
    m_explorer->select(path);
    updateTitle();
    refreshPreview();
}

bool MainWindow::closeNote()
{
    if (m_path.isEmpty())
        return true;
    if (!maybeSave())
        return false;
    m_path.clear();
    m_editor->importMarkdown(QString());
    return true;
}

QString MainWindow::targetDirectory() const
{
    if (!m_path.isEmpty())
        return QFileInfo(m_path).absolutePath();
    return m_folder.isEmpty() ? m_root : m_folder;
}

QString MainWindow::currentItem() const
{
    if (!m_path.isEmpty())
        return m_path;
    return m_folder == m_root ? QString() : m_folder;
}

void MainWindow::updateItemActions()
{
    const bool hasItem = !currentItem().isEmpty();
    m_renameAction->setEnabled(hasItem);
    m_deleteAction->setEnabled(hasItem);
}

// ---------------------------------------------------------------- file management

QString MainWindow::askName(const QString& title, const QString& initial)
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, title, tr("Name:"), QLineEdit::Normal, initial, &ok).trimmed();
    if (!ok)
        return QString();
    if (name.isEmpty() || name.startsWith(QLatin1Char('.')) || name.contains(QLatin1Char('/'))
        || name.contains(QLatin1Char('\\'))) {
        QMessageBox::warning(this, title, tr("Names cannot be empty, start with a dot, or contain slashes."));
        return QString();
    }
    return name;
}

void MainWindow::createNote(const QString& directory)
{
    QString name = askName(tr("New note"), tr("Untitled"));
    if (name.isEmpty())
        return;
    if (!name.endsWith(QLatin1String(".md"), Qt::CaseInsensitive))
        name += QLatin1String(".md");
    const QString path = QDir(directory).filePath(name);
    QFile f(path);
    // NewOnly makes sure an existing note is never overwritten.
    if (!f.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        QMessageBox::warning(this, tr("New note"),
            QFileInfo::exists(path) ? tr("\"%1\" already exists.").arg(name) : tr("Cannot create %1").arg(path));
        return;
    }
    f.close();
    openPath(path);
}

void MainWindow::createFolder(const QString& directory)
{
    const QString name = askName(tr("New folder"), tr("New folder"));
    if (name.isEmpty())
        return;
    if (!QDir(directory).mkdir(name)) {
        QMessageBox::warning(this, tr("New folder"), tr("Cannot create \"%1\". It may already exist.").arg(name));
        return;
    }
    openPath(QDir(directory).filePath(name));
}

void MainWindow::renamePath(const QString& path)
{
    if (path.isEmpty())
        return;
    const QString oldPath = cleanPath(path);
    if (oldPath == m_root)
        return;
    const QFileInfo info(oldPath);
    QString name = askName(tr("Rename"), info.isDir() ? info.fileName() : info.completeBaseName());
    if (name.isEmpty())
        return;
    if (!info.isDir() && !name.endsWith(QLatin1String(".md"), Qt::CaseInsensitive))
        name += QLatin1String(".md");
    const QString newPath = info.dir().filePath(name);
    if (newPath == oldPath)
        return;
    if (QFileInfo::exists(newPath) || !QDir().rename(oldPath, newPath)) {
        QMessageBox::warning(this, tr("Rename"), tr("Cannot rename to \"%1\". It may already exist.").arg(name));
        return;
    }

    auto remap = [&](QString& p) {
        if (isAffected(p, oldPath))
            p = newPath + p.mid(oldPath.size());
    };
    remap(m_path);
    remap(m_folder);

    if (!m_path.isEmpty()) {
        m_editor->setBaseDirectory(QFileInfo(m_path).absolutePath());
        m_explorer->select(m_path);
        updateTitle();
    } else {
        showFolder(m_folder);
    }
}

void MainWindow::deletePath(const QString& path)
{
    if (path.isEmpty())
        return;
    const QString target = cleanPath(path);
    if (target == m_root)
        return;
    const QFileInfo info(target);
    const bool isFolder = info.isDir() && !info.isSymLink();

    const QString question = isFolder
        ? tr("Delete the folder \"%1\" and everything inside it?\nIt will be moved to the trash.").arg(info.fileName())
        : tr("Delete \"%1\"?\nIt will be moved to the trash.").arg(info.fileName());
    if (QMessageBox::question(this, tr("Delete"), question) != QMessageBox::Yes)
        return;

    if (!QFile::moveToTrash(target)) {
        const auto answer = QMessageBox::warning(this, tr("Delete"),
            tr("\"%1\" could not be moved to the trash.\nDelete it permanently? This cannot be undone.")
                .arg(info.fileName()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
        const bool removed = isFolder ? QDir(target).removeRecursively() : QFile::remove(target);
        if (!removed) {
            QMessageBox::warning(this, tr("Delete"), tr("Could not delete %1").arg(target));
            return;
        }
    }

    const QString parent = info.absolutePath();
    if (isAffected(m_path, target)) {
        // Forget the note first so that auto-save cannot recreate it.
        m_autoSaveTimer.stop();
        m_path.clear();
        m_editor->importMarkdown(QString());
        showFolder(parent);
    } else if (m_path.isEmpty()) {
        showFolder(isAffected(m_folder, target) ? parent : m_folder);
    }
}

// ---------------------------------------------------------------- saving & state

void MainWindow::autoSave()
{
    if (!m_autoSave || m_path.isEmpty() || !m_editor->document()->isModified())
        return;
    if (!QFileInfo::exists(m_path)) {
        statusBar()->showMessage(tr("This file was removed outside mdnote. Press Ctrl+S to recreate it."), 5000);
        return;
    }
    writeTo(m_path, true);
}

bool MainWindow::maybeSave()
{
    m_autoSaveTimer.stop();
    if (m_path.isEmpty() || !m_editor->document()->isModified())
        return true;
    if (m_autoSave && QFileInfo::exists(m_path) && writeTo(m_path, true))
        return true;
    const auto r = QMessageBox::question(this, tr("Unsaved changes"),
        tr("Save changes to \"%1\"?").arg(QFileInfo(m_path).completeBaseName()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (r == QMessageBox::Save)
        return saveFile();
    return r == QMessageBox::Discard;
}

bool MainWindow::writeTo(const QString& path, bool quiet)
{
    auto fail = [&] {
        const QString message = tr("Cannot write %1").arg(path);
        if (quiet)
            statusBar()->showMessage(message, 5000);
        else
            QMessageBox::warning(this, tr("Save"), message);
        return false;
    };
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return fail();
    f.write(m_editor->exportMarkdown().toUtf8());
    if (!f.commit())
        return fail();
    m_editor->document()->setModified(false);
    statusBar()->showMessage(tr("Saved"), 2000);
    updateTitle();
    return true;
}

bool MainWindow::saveFile() { return !m_path.isEmpty() && writeTo(m_path); }

void MainWindow::setNoteActionsEnabled(bool enabled)
{
    for (QAction* a : std::as_const(m_noteActions))
        a->setEnabled(enabled);
    m_heading->setEnabled(enabled);
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
        m_preview->setPlainText(m_path.isEmpty() ? QString() : m_editor->exportMarkdown());
}

void MainWindow::updateTitle()
{
    const QString rootName = QFileInfo(m_root).fileName();
    if (m_path.isEmpty()) {
        setWindowTitle(QStringLiteral("%1 — mdnote").arg(rootName));
        return;
    }
    setWindowTitle(QStringLiteral("%1%2 — %3 — mdnote")
            .arg(QFileInfo(m_path).completeBaseName(),
                m_editor->document()->isModified() ? QStringLiteral("*") : QString(),
                rootName));
}

void MainWindow::changeEvent(QEvent* e)
{
    QMainWindow::changeEvent(e);
    if (e->type() == QEvent::ActivationChange && !isActiveWindow())
        autoSave();
}

void MainWindow::closeEvent(QCloseEvent* e)
{
    if (maybeSave())
        e->accept();
    else
        e->ignore();
}

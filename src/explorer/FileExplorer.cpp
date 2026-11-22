#include "FileExplorer.h"
#include "ui/Icons.h"
#include <QAction>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

namespace {
// Hides the ".md" suffix and supplies our own icons.
class NotesModel : public QFileSystemModel {
public:
    using QFileSystemModel::QFileSystemModel;

    QVariant data(const QModelIndex& index, int role) const override
    {
        if (index.isValid() && index.column() == 0) {
            if (role == Qt::DecorationRole)
                return isDir(index) ? m_folderIcon : m_noteIcon;
            if (role == Qt::DisplayRole && !isDir(index))
                return QFileInfo(QFileSystemModel::data(index, role).toString()).completeBaseName();
        }
        return QFileSystemModel::data(index, role);
    }

private:
    const QIcon m_folderIcon = makeIcon(Glyph::Folder);
    const QIcon m_noteIcon = makeIcon(Glyph::Note);
};
} // namespace

FileExplorer::FileExplorer(QWidget* parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("sidebar"));
    setMinimumWidth(200);

    m_title = new QPushButton(this);
    m_title->setObjectName(QStringLiteral("rootButton"));
    m_title->setCursor(Qt::PointingHandCursor);
    m_title->setFocusPolicy(Qt::NoFocus);

    auto makeButton = [this](Glyph glyph, const QString& tip) {
        auto* b = new QToolButton(this);
        b->setIcon(makeIcon(glyph));
        b->setIconSize(QSize(18, 18));
        b->setToolTip(tip);
        b->setAutoRaise(true);
        return b;
    };
    QToolButton* newNote = makeButton(Glyph::NewNote, tr("New note"));
    QToolButton* newFolder = makeButton(Glyph::NewFolder, tr("New folder"));

    m_model = new NotesModel(this);
    m_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    m_model->setNameFilters({ QStringLiteral("*.md") });
    m_model->setNameFilterDisables(false); // hide other files instead of greying them

    m_tree = new QTreeView(this);
    m_tree->setObjectName(QStringLiteral("explorerTree"));
    m_tree->setModel(m_model);
    m_tree->setHeaderHidden(true);
    for (int column = 1; column < m_model->columnCount(); ++column)
        m_tree->hideColumn(column);
    m_tree->setIndentation(16);
    m_tree->setIconSize(QSize(18, 18));
    m_tree->setAnimated(true);
    m_tree->setUniformRowHeights(true);
    m_tree->setSortingEnabled(true);
    m_tree->sortByColumn(0, Qt::AscendingOrder);
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);

    auto* header = new QHBoxLayout;
    header->setContentsMargins(10, 10, 8, 6);
    header->setSpacing(2);
    header->addWidget(m_title, 1);
    header->addWidget(newNote);
    header->addWidget(newFolder);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addLayout(header);
    layout->addWidget(m_tree, 1);

    auto open = [this](const QModelIndex& index) { emit openRequested(m_model->filePath(index)); };
    connect(m_tree, &QTreeView::clicked, this, open);
    connect(m_tree, &QTreeView::activated, this, open);
    connect(m_tree, &QTreeView::customContextMenuRequested, this, &FileExplorer::showContextMenu);
    connect(m_title, &QPushButton::clicked, this, [this] { emit openRequested(m_root); });
    connect(newNote, &QToolButton::clicked, this, [this] { emit newNoteRequested(selectedDirectory()); });
    connect(newFolder, &QToolButton::clicked, this, [this] { emit newFolderRequested(selectedDirectory()); });

    // F2 and Delete, active only while the tree has focus.
    auto addShortcut = [this](const QKeySequence& keys, auto signal) {
        auto* action = new QAction(m_tree);
        action->setShortcut(keys);
        action->setShortcutContext(Qt::WidgetShortcut);
        connect(action, &QAction::triggered, this, [this, signal] {
            const QString path = selectedItem();
            if (!path.isEmpty())
                emit(this->*signal)(path);
        });
        m_tree->addAction(action);
    };
    addShortcut(QKeySequence(Qt::Key_F2), &FileExplorer::renameRequested);
    addShortcut(QKeySequence(QKeySequence::Delete), &FileExplorer::trashRequested);
}

void FileExplorer::setRoot(const QString& directory)
{
    m_root = directory;
    m_tree->setRootIndex(m_model->setRootPath(directory));
    m_title->setText(QFileInfo(directory).fileName());
    m_title->setToolTip(directory);
}

void FileExplorer::select(const QString& path)
{
    const QModelIndex index = m_model->index(path);
    if (path == m_root || !index.isValid()) {
        m_tree->clearSelection();
        m_tree->setCurrentIndex(QModelIndex());
        return;
    }
    for (QModelIndex p = index.parent(); p.isValid() && p != m_tree->rootIndex(); p = p.parent())
        m_tree->expand(p);
    m_tree->setCurrentIndex(index);
    m_tree->scrollTo(index);
}

QString FileExplorer::selectedItem() const
{
    const QModelIndex index = m_tree->currentIndex();
    return index.isValid() ? m_model->filePath(index) : QString();
}

QString FileExplorer::selectedDirectory() const
{
    const QString path = selectedItem();
    if (path.isEmpty())
        return m_root;
    const QFileInfo info(path);
    return info.isDir() ? path : info.absolutePath();
}

void FileExplorer::showContextMenu(const QPoint& pos)
{
    const QModelIndex index = m_tree->indexAt(pos);
    const QString path = index.isValid() ? m_model->filePath(index) : QString();
    QString directory = m_root;
    if (index.isValid())
        directory = m_model->isDir(index) ? path : QFileInfo(path).absolutePath();

    QMenu menu(this);
    QAction* newNote = menu.addAction(makeIcon(Glyph::NewNote), tr("New note…"));
    QAction* newFolder = menu.addAction(makeIcon(Glyph::NewFolder), tr("New folder…"));
    QAction* rename = nullptr;
    QAction* trash = nullptr;
    if (index.isValid()) {
        menu.addSeparator();
        rename = menu.addAction(tr("Rename…"));
        trash = menu.addAction(tr("Move to trash"));
    }

    QAction* chosen = menu.exec(m_tree->viewport()->mapToGlobal(pos));
    if (!chosen)
        return;
    if (chosen == newNote)
        emit newNoteRequested(directory);
    else if (chosen == newFolder)
        emit newFolderRequested(directory);
    else if (chosen == rename)
        emit renameRequested(path);
    else if (chosen == trash)
        emit trashRequested(path);
}

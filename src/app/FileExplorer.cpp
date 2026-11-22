#include "FileExplorer.h"
#include "theme/Icons.h"

#include <QDir>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QMenu>
#include <QShortcut>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

namespace {
// QFileSystemModel with our own icons and no ".md" suffix on note names.
class NoteModel : public QFileSystemModel {
public:
    using QFileSystemModel::QFileSystemModel;

    QVariant data(const QModelIndex& index, int role) const override
    {
        if (index.isValid() && index.column() == 0) {
            if (role == Qt::DecorationRole) {
                return isDir(index) ? icons::get(icons::Glyph::Folder, QColor(0x7a, 0xa2, 0xf7))
                                    : icons::get(icons::Glyph::Note, QColor(0x8b, 0x95, 0xa1));
            }
            if (role == Qt::DisplayRole && !isDir(index))
                return QFileInfo(filePath(index)).completeBaseName();
        }
        return QFileSystemModel::data(index, role);
    }
};
} // namespace

FileExplorer::FileExplorer(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("explorer");
    setAttribute(Qt::WA_StyledBackground);
    setMinimumWidth(180);

    m_title = new QToolButton(this);
    m_title->setObjectName("workspaceTitle");
    m_title->setCursor(Qt::PointingHandCursor);
    m_title->setToolTip(tr("Show all notes"));
    connect(m_title, &QToolButton::clicked, this, [this] { emit folderActivated(m_root); });

    auto headerButton = [this](icons::Glyph glyph, const QString& tip) {
        auto* b = new QToolButton(this);
        b->setIcon(icons::get(glyph));
        b->setToolTip(tip);
        return b;
    };
    auto* addNote = headerButton(icons::Glyph::NewNote, tr("New note"));
    auto* addFolder = headerButton(icons::Glyph::NewFolder, tr("New folder"));
    // An empty directory means "the folder currently in view".
    connect(addNote, &QToolButton::clicked, this, [this] { emit newNoteRequested(QString()); });
    connect(addFolder, &QToolButton::clicked, this, [this] { emit newFolderRequested(QString()); });

    m_model = new NoteModel(this);
    m_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    m_model->setNameFilters({ QStringLiteral("*.md") });
    m_model->setNameFilterDisables(false); // hide, rather than grey out

    m_tree = new QTreeView(this);
    m_tree->setModel(m_model);
    m_tree->setHeaderHidden(true);
    for (int column = 1; column < m_model->columnCount(); ++column)
        m_tree->hideColumn(column);
    m_tree->setIndentation(14);
    m_tree->setIconSize(QSize(16, 16));
    m_tree->setAnimated(true);
    m_tree->setExpandsOnDoubleClick(false);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tree, &QTreeView::clicked, this, &FileExplorer::activate);
    connect(m_tree, &QTreeView::customContextMenuRequested, this, &FileExplorer::showContextMenu);
    for (const Qt::Key key : { Qt::Key_Return, Qt::Key_Enter }) {
        auto* shortcut = new QShortcut(QKeySequence(key), m_tree);
        shortcut->setContext(Qt::WidgetShortcut);
        connect(shortcut, &QShortcut::activated, this, [this] { activate(m_tree->currentIndex()); });
    }

    auto* header = new QHBoxLayout;
    header->setContentsMargins(8, 8, 8, 4);
    header->setSpacing(2);
    header->addWidget(m_title);
    header->addStretch();
    header->addWidget(addNote);
    header->addWidget(addFolder);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addLayout(header);
    layout->addWidget(m_tree, 1);
}

void FileExplorer::setRootPath(const QString& path)
{
    m_root = path;
    QString name = QFileInfo(path).fileName();
    m_title->setText(name.replace(QLatin1Char('&'), QLatin1String("&&")));
    m_model->setRootPath(path);
    m_tree->setRootIndex(m_model->index(path));
}

void FileExplorer::select(const QString& path)
{
    if (path.isEmpty() || path == m_root || !path.startsWith(m_root)) {
        m_tree->clearSelection();
        m_tree->setCurrentIndex(QModelIndex());
        return;
    }
    const QModelIndex index = m_model->index(path);
    m_tree->setCurrentIndex(index);
    m_tree->scrollTo(index);
}

void FileExplorer::activate(const QModelIndex& index)
{
    if (!index.isValid())
        return;
    const QString path = m_model->filePath(index);
    if (m_model->isDir(index)) {
        m_tree->expand(index);
        emit folderActivated(path);
    } else {
        emit noteActivated(path);
    }
}

void FileExplorer::showContextMenu(const QPoint& pos)
{
    const QModelIndex index = m_tree->indexAt(pos);
    QString dir = m_root;
    if (index.isValid())
        dir = m_model->isDir(index) ? m_model->filePath(index) : m_model->fileInfo(index).absolutePath();

    QMenu menu(this);
    QAction* note = menu.addAction(tr("New note here"));
    QAction* folder = menu.addAction(tr("New folder here"));
    const QAction* chosen = menu.exec(m_tree->viewport()->mapToGlobal(pos));
    if (chosen == note)
        emit newNoteRequested(dir);
    else if (chosen == folder)
        emit newFolderRequested(dir);
}

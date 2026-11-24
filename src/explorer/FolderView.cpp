#include "FolderView.h"
#include "theme/Theme.h"
#include "ui/Icons.h"
#include <QAction>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

namespace {
enum Role {
    PathRole = Qt::UserRole,
    PreviewRole,
    FolderRole
};

constexpr int kCardWidth = 224;
constexpr int kCardHeight = 164;

QFileInfoList listEntries(const QString& directory)
{
    return QDir(directory).entryInfoList({ QStringLiteral("*.md") },
        QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot,
        QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
}

QString countText(const QFileInfoList& entries)
{
    int folders = 0;
    for (const QFileInfo& e : entries)
        folders += e.isDir() ? 1 : 0;
    const int notes = entries.size() - folders;
    return QStringLiteral("%1 %2  ·  %3 %4")
        .arg(folders)
        .arg(folders == 1 ? QStringLiteral("folder") : QStringLiteral("folders"))
        .arg(notes)
        .arg(notes == 1 ? QStringLiteral("note") : QStringLiteral("notes"));
}

QString folderPreview(const QString& path)
{
    const QFileInfoList entries = listEntries(path);
    QStringList lines { countText(entries) };
    for (int i = 0; i < entries.size() && i < 4; ++i)
        lines << QStringLiteral("• ") + (entries[i].isDir() ? entries[i].fileName() : entries[i].completeBaseName());
    return lines.join(QLatin1Char('\n'));
}

// The first few lines of a note with most Markdown syntax removed.
QString notePreview(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    const QString head = QString::fromUtf8(f.read(2000));

    static const QRegularExpression skipped(QStringLiteral(R"re(^\s*(```.*|~~~.*|[=\-*_|: ]{3,})\s*$)re"));
    static const QRegularExpression prefix(QStringLiteral(R"re(^\s*(#{1,6}\s+|(>\s*)+|[-*+]\s+(\[[ xX]\]\s+)?|\d+[.)]\s+))re"));
    static const QRegularExpression link(QStringLiteral(R"re(!?\[([^\]]*)\](\([^)]*\)|\[[^\]]*\])?)re"));
    static const QRegularExpression marks(QStringLiteral(R"re(\*+|__|~+|==|`|\^|\\)re"));

    QStringList lines;
    for (QString line : head.split(QLatin1Char('\n'))) {
        if (skipped.match(line).hasMatch())
            continue;
        line.remove(prefix);
        line.replace(link, QStringLiteral("\\1"));
        line.remove(marks);
        line = line.trimmed();
        if (!line.isEmpty())
            lines << line;
        if (lines.size() == 6)
            break;
    }
    return lines.isEmpty() ? QStringLiteral("Empty note") : lines.join(QLatin1Char('\n'));
}

class CardDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override
    {
        return QSize(kCardWidth, kCardHeight);
    }

    void paint(QPainter* p, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        const md::Theme& t = md::theme();
        const bool hover = option.state & QStyle::State_MouseOver;
        const bool selected = option.state & QStyle::State_Selected;
        const bool folder = index.data(FolderRole).toBool();
        const QRectF card = QRectF(option.rect).adjusted(1, 1, -1, -1);

        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(QPen(selected ? t.accent : t.border, 1));
        p->setBrush(hover ? t.hover : t.surface);
        p->drawRoundedRect(card, 10, 10);

        const QRect inner = option.rect.adjusted(14, 14, -14, -12);
        const QRect iconRect(inner.left(), inner.top(), 22, 22);
        makeIcon(folder ? Glyph::Folder : Glyph::Note).paint(p, iconRect);

        QFont titleFont = option.font;
        titleFont.setBold(true);
        p->setFont(titleFont);
        p->setPen(t.text);
        const QRect titleRect(iconRect.right() + 9, inner.top(), inner.right() - iconRect.right() - 9, 22);
        p->drawText(titleRect, Qt::AlignVCenter | Qt::AlignLeft,
            QFontMetrics(titleFont).elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideRight, titleRect.width()));

        QFont bodyFont = option.font;
        bodyFont.setPointSizeF(qMax<qreal>(7.5, option.font.pointSizeF() - 1));
        p->setFont(bodyFont);
        p->setPen(t.muted);
        const QRect body = inner.adjusted(0, 34, 0, 0);
        p->setClipRect(body);
        p->drawText(body, Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap, index.data(PreviewRole).toString());
        p->restore();
    }
};
} // namespace

FolderView::FolderView(QWidget* parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("folderView"));

    m_breadcrumb = new QLabel(this);
    m_breadcrumb->setObjectName(QStringLiteral("muted"));
    m_breadcrumb->setTextFormat(Qt::RichText);

    m_title = new QLabel(this);
    m_title->setObjectName(QStringLiteral("heading"));
    m_title->setTextFormat(Qt::PlainText);

    m_summary = new QLabel(this);
    m_summary->setObjectName(QStringLiteral("muted"));

    auto* newFolder = new QPushButton(tr("New folder"), this);
    auto* newNote = new QPushButton(tr("New note"), this);
    newNote->setObjectName(QStringLiteral("primary"));

    m_cards = new QListWidget(this);
    m_cards->setObjectName(QStringLiteral("cards"));
    m_cards->setViewMode(QListView::IconMode);
    m_cards->setResizeMode(QListView::Adjust);
    m_cards->setMovement(QListView::Static);
    m_cards->setSpacing(7);
    m_cards->setUniformItemSizes(true);
    m_cards->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_cards->setItemDelegate(new CardDelegate(m_cards));
    m_cards->setMouseTracking(true);
    m_cards->viewport()->setCursor(Qt::PointingHandCursor);
    m_cards->setContextMenuPolicy(Qt::CustomContextMenu);

    m_empty = new QLabel(tr("This folder is empty.\nCreate a note or a folder to get started."), this);
    m_empty->setObjectName(QStringLiteral("muted"));
    m_empty->setAlignment(Qt::AlignCenter);

    auto* titleRow = new QHBoxLayout;
    titleRow->addWidget(m_title, 1);
    titleRow->addWidget(newFolder);
    titleRow->addWidget(newNote);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(36, 28, 28, 16);
    layout->setSpacing(6);
    layout->addWidget(m_breadcrumb);
    layout->addLayout(titleRow);
    layout->addWidget(m_summary);
    layout->addSpacing(12);
    layout->addWidget(m_cards, 1);
    layout->addWidget(m_empty, 1);

    auto open = [this](QListWidgetItem* item) { emit openRequested(item->data(PathRole).toString()); };
    connect(m_cards, &QListWidget::itemClicked, this, open);
    connect(m_cards, &QListWidget::itemActivated, this, open);
    connect(m_cards, &QListWidget::customContextMenuRequested, this, &FolderView::showContextMenu);
    connect(newNote, &QPushButton::clicked, this, [this] { emit newNoteRequested(m_directory); });
    connect(newFolder, &QPushButton::clicked, this, [this] { emit newFolderRequested(m_directory); });
    connect(m_breadcrumb, &QLabel::linkActivated, this, [this](const QString& link) {
        emit openRequested(m_crumbs.value(link.toInt()));
    });
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &FolderView::refresh);

    auto* remove = new QAction(m_cards);
    remove->setShortcut(QKeySequence(QKeySequence::Delete));
    remove->setShortcutContext(Qt::WidgetShortcut);
    connect(remove, &QAction::triggered, this, [this] {
        const QString path = selectedItem();
        if (!path.isEmpty())
            emit deleteRequested(path);
    });
    m_cards->addAction(remove);
}

void FolderView::showFolder(const QString& directory)
{
    if (!m_watcher.directories().isEmpty())
        m_watcher.removePaths(m_watcher.directories());
    m_directory = directory;
    m_watcher.addPath(directory);
    refresh();
}

void FolderView::refreshTheme()
{
    if (!m_directory.isEmpty())
        updateBreadcrumb();
    m_cards->viewport()->update();
}

QString FolderView::selectedItem() const
{
    const auto selected = m_cards->selectedItems();
    return selected.isEmpty() ? QString() : selected.first()->data(PathRole).toString();
}

QString FolderView::focusedItem() const
{
    return m_cards->hasFocus() ? selectedItem() : QString();
}

void FolderView::showContextMenu(const QPoint& pos)
{
    // Copy the path now: the list may refresh while the menu is open.
    const QListWidgetItem* item = m_cards->itemAt(pos);
    const QString path = item ? item->data(PathRole).toString() : QString();

    QMenu menu(this);
    QAction* open = nullptr;
    QAction* rename = nullptr;
    QAction* remove = nullptr;
    QAction* newNote = nullptr;
    QAction* newFolder = nullptr;
    if (!path.isEmpty()) {
        open = menu.addAction(tr("Open"));
        rename = menu.addAction(makeIcon(Glyph::Rename), tr("Rename…"));
        remove = menu.addAction(makeIcon(Glyph::Trash), tr("Delete…"));
    } else {
        newNote = menu.addAction(makeIcon(Glyph::NewNote), tr("New note…"));
        newFolder = menu.addAction(makeIcon(Glyph::NewFolder), tr("New folder…"));
    }

    QAction* chosen = menu.exec(m_cards->viewport()->mapToGlobal(pos));
    if (!chosen)
        return;
    if (chosen == open)
        emit openRequested(path);
    else if (chosen == rename)
        emit renameRequested(path);
    else if (chosen == remove)
        emit deleteRequested(path);
    else if (chosen == newNote)
        emit newNoteRequested(m_directory);
    else if (chosen == newFolder)
        emit newFolderRequested(m_directory);
}

void FolderView::refresh()
{
    const QFileInfoList entries = listEntries(m_directory);
    m_title->setText(QFileInfo(m_directory).fileName());
    m_summary->setText(countText(entries));
    updateBreadcrumb();

    m_cards->clear();
    for (const QFileInfo& entry : entries) {
        const bool folder = entry.isDir();
        const QString path = entry.absoluteFilePath();
        auto* item = new QListWidgetItem(folder ? entry.fileName() : entry.completeBaseName(), m_cards);
        item->setData(PathRole, path);
        item->setData(FolderRole, folder);
        item->setData(PreviewRole, folder ? folderPreview(path) : notePreview(path));
        item->setToolTip(entry.fileName());
    }
    m_cards->setVisible(!entries.isEmpty());
    m_empty->setVisible(entries.isEmpty());
}

void FolderView::updateBreadcrumb()
{
    const QString relative = QDir(m_root).relativeFilePath(m_directory);
    QStringList names { QFileInfo(m_root).fileName() };
    m_crumbs = QStringList { m_root };
    if (relative != QLatin1String(".")) {
        QString path = m_root;
        for (const QString& part : relative.split(QLatin1Char('/'), Qt::SkipEmptyParts)) {
            path = QDir(path).filePath(part);
            names << part;
            m_crumbs << path;
        }
    }
    const QString color = md::theme().accent.name();
    QStringList links;
    for (int i = 0; i < names.size(); ++i)
        links << QStringLiteral("<a href=\"%1\" style=\"color:%2; text-decoration:none\">%3</a>")
                     .arg(QString::number(i), color, names[i].toHtmlEscaped());
    m_breadcrumb->setText(links.join(QStringLiteral("&nbsp;&nbsp;›&nbsp;&nbsp;")));
}

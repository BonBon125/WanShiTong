#include "FolderView.h"
#include "theme/Icons.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QShortcut>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QVBoxLayout>

namespace {
enum Role {
    PathRole = Qt::UserRole,
    IsDirRole,
    PreviewRole
};

constexpr QSize kCardSize(220, 156);

QFileInfoList entriesOf(const QString& dir)
{
    return QDir(dir).entryInfoList({ QStringLiteral("*.md") },
        QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot,
        QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
}

// The first few lines of a note with most Markdown syntax removed.
QString notePreview(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    QString text = QString::fromUtf8(f.read(800));

    static const QRegularExpression bullet(QStringLiteral(R"re(^[ \t]*[-+*][ \t]+(\[[ xX]\][ \t]+)?)re"),
        QRegularExpression::MultilineOption);
    static const QRegularExpression prefix(QStringLiteral(R"re(^[ \t]*(#{1,6}[ \t]+|>+[ \t]?|```.*$))re"),
        QRegularExpression::MultilineOption);
    static const QRegularExpression marks(QStringLiteral(R"re(\*\*|__|~~|==|`|\\)re"));
    static const QRegularExpression blankLines(QStringLiteral(R"re(\n\s*\n)re"));

    text.replace(bullet, QStringLiteral("• "));
    text.remove(prefix);
    text.remove(marks);
    text.replace(blankLines, QStringLiteral("\n"));
    text = text.trimmed();
    return text.isEmpty() ? FolderView::tr("Empty note") : text;
}

QString folderPreview(const QString& path)
{
    const QFileInfoList entries = entriesOf(path);
    if (entries.isEmpty())
        return FolderView::tr("Empty folder");
    QStringList lines;
    for (int i = 0; i < entries.size() && i < 4; ++i)
        lines << (entries[i].isDir() ? entries[i].fileName() + QLatin1Char('/') : entries[i].completeBaseName());
    if (entries.size() > 4)
        lines << FolderView::tr("+ %1 more").arg(entries.size() - 4);
    return lines.join(QLatin1Char('\n'));
}

class CardDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override { return kCardSize; }

    void paint(QPainter* p, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        const bool active = option.state & (QStyle::State_MouseOver | QStyle::State_Selected);
        const bool isDir = index.data(IsDirRole).toBool();
        const QRect r = option.rect.adjusted(1, 1, -1, -1);

        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(QPen(active ? QColor(0x7a, 0xa2, 0xf7) : QColor(0x35, 0x3b, 0x45), 1));
        p->setBrush(active ? QColor(0x2a, 0x2e, 0x35) : QColor(0x21, 0x25, 0x2b));
        p->drawRoundedRect(QRectF(r).adjusted(0.5, 0.5, -0.5, -0.5), 10, 10);

        const QIcon icon = isDir ? icons::get(icons::Glyph::Folder, QColor(0x7a, 0xa2, 0xf7))
                                 : icons::get(icons::Glyph::Note, QColor(0x8b, 0x95, 0xa1));
        icon.paint(p, QRect(r.left() + 14, r.top() + 14, 20, 20));

        QFont titleFont = option.font;
        titleFont.setBold(true);
        p->setFont(titleFont);
        p->setPen(QColor(0xdd, 0xe1, 0xe6));
        const QRect titleRect(r.left() + 42, r.top() + 14, r.width() - 54, 20);
        p->drawText(titleRect, Qt::AlignVCenter,
            QFontMetrics(titleFont).elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideRight, titleRect.width()));

        QFont small = option.font;
        small.setPointSizeF(qMax(8.0, option.font.pointSizeF() - 1));
        p->setFont(small);
        p->setPen(QColor(0x8b, 0x95, 0xa1));
        const int lineHeight = QFontMetrics(small).lineSpacing();
        const int height = ((r.height() - 58) / lineHeight) * lineHeight; // whole lines only
        p->drawText(QRect(r.left() + 14, r.top() + 46, r.width() - 28, height),
            Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap, index.data(PreviewRole).toString());
        p->restore();
    }
};
} // namespace

FolderView::FolderView(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("folderView");
    setAttribute(Qt::WA_StyledBackground);

    m_title = new QLabel(this);
    m_title->setObjectName("heroTitle");
    m_title->setTextFormat(Qt::PlainText);
    m_summary = new QLabel(this);
    m_summary->setObjectName("muted");

    auto* addNote = new QPushButton(tr("New note"), this);
    addNote->setProperty("accent", true);
    auto* addFolder = new QPushButton(tr("New folder"), this);
    connect(addNote, &QPushButton::clicked, this, [this] { emit newNoteRequested(m_dir); });
    connect(addFolder, &QPushButton::clicked, this, [this] { emit newFolderRequested(m_dir); });

    m_cards = new QListWidget(this);
    m_cards->setObjectName("cards");
    m_cards->setViewMode(QListView::IconMode);
    m_cards->setResizeMode(QListView::Adjust);
    m_cards->setMovement(QListView::Static);
    m_cards->setSpacing(7);
    m_cards->setUniformItemSizes(true);
    m_cards->setMouseTracking(true);
    m_cards->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_cards->setItemDelegate(new CardDelegate(m_cards));
    connect(m_cards, &QListWidget::itemClicked, this, &FolderView::activate);
    for (const Qt::Key key : { Qt::Key_Return, Qt::Key_Enter }) {
        auto* shortcut = new QShortcut(QKeySequence(key), m_cards);
        shortcut->setContext(Qt::WidgetShortcut);
        connect(shortcut, &QShortcut::activated, this, [this] { activate(m_cards->currentItem()); });
    }

    m_empty = new QLabel(tr("Nothing here yet. Create a note to get started."), this);
    m_empty->setObjectName("muted");
    m_empty->setAlignment(Qt::AlignCenter);

    auto* titles = new QVBoxLayout;
    titles->setSpacing(2);
    titles->addWidget(m_title);
    titles->addWidget(m_summary);

    auto* header = new QHBoxLayout;
    header->setContentsMargins(8, 0, 8, 0);
    header->addLayout(titles, 1);
    header->addWidget(addNote, 0, Qt::AlignVCenter);
    header->addWidget(addFolder, 0, Qt::AlignVCenter);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 16, 24, 16);
    layout->setSpacing(14);
    layout->addLayout(header);
    layout->addWidget(m_cards, 1);
    layout->addWidget(m_empty, 1);

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &FolderView::reload);
}

void FolderView::showFolder(const QString& dir)
{
    if (!m_watcher.directories().isEmpty())
        m_watcher.removePaths(m_watcher.directories());
    m_dir = dir;
    m_watcher.addPath(dir);
    reload();
}

void FolderView::reload()
{
    m_cards->clear();
    m_title->setText(QFileInfo(m_dir).fileName());

    int folders = 0, notes = 0;
    const QFileInfoList entries = entriesOf(m_dir);
    for (const QFileInfo& info : entries) {
        const bool isDir = info.isDir();
        auto* item = new QListWidgetItem(isDir ? info.fileName() : info.completeBaseName(), m_cards);
        item->setData(PathRole, info.absoluteFilePath());
        item->setData(IsDirRole, isDir);
        item->setData(PreviewRole, isDir ? folderPreview(info.absoluteFilePath()) : notePreview(info.absoluteFilePath()));
        item->setToolTip(info.fileName());
        ++(isDir ? folders : notes);
    }

    m_summary->setText(tr("Folders: %1  ·  Notes: %2").arg(folders).arg(notes));
    m_cards->setVisible(!entries.isEmpty());
    m_empty->setVisible(entries.isEmpty());
}

void FolderView::activate(const QListWidgetItem* item)
{
    if (!item)
        return;
    const QString path = item->data(PathRole).toString();
    const bool isDir = item->data(IsDirRole).toBool();
    // Deferred: opening a folder rebuilds the list that is emitting this signal.
    QTimer::singleShot(0, this, [this, path, isDir] {
        if (isDir)
            emit folderActivated(path);
        else
            emit noteActivated(path);
    });
}

#include "WorkspaceDialog.h"
#include "theme/Icons.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

namespace {
const char* kRecentKey = "workspaces/recent";
constexpr int kMaxRecent = 8;
} // namespace

WorkspaceDialog::WorkspaceDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("mdnote — Choose notes directory"));
    resize(520, 420);

    auto* title = new QLabel(tr("Welcome to mdnote"), this);
    title->setObjectName("heroTitle");
    auto* subtitle = new QLabel(tr("Choose where your notes live."), this);
    subtitle->setObjectName("muted");

    auto* recent = new QListWidget(this);
    recent->setObjectName("recentList");
    recent->setIconSize(QSize(20, 20));
    const QStringList paths = QSettings().value(kRecentKey).toStringList();
    for (const QString& p : paths) {
        if (!QFileInfo(p).isDir())
            continue;
        auto* item = new QListWidgetItem(icons::get(icons::Glyph::Folder, QColor(0x7a, 0xa2, 0xf7)),
            QFileInfo(p).fileName() + QLatin1Char('\n') + QDir::toNativeSeparators(p), recent);
        item->setData(Qt::UserRole, p);
    }
    auto openItem = [this](QListWidgetItem* item) { choose(item->data(Qt::UserRole).toString()); };
    connect(recent, &QListWidget::itemClicked, this, openItem);
    connect(recent, &QListWidget::itemActivated, this, openItem);

    auto* create = new QPushButton(tr("Create new directory…"), this);
    create->setProperty("accent", true);
    create->setAutoDefault(false);
    auto* open = new QPushButton(tr("Open existing directory…"), this);
    open->setAutoDefault(false);
    connect(create, &QPushButton::clicked, this, &WorkspaceDialog::createNew);
    connect(open, &QPushButton::clicked, this, &WorkspaceDialog::openExisting);

    auto* buttons = new QHBoxLayout;
    buttons->addWidget(create);
    buttons->addWidget(open);
    buttons->addStretch();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(10);
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(8);
    if (recent->count() > 0) {
        layout->addWidget(new QLabel(tr("Recent"), this));
        layout->addWidget(recent, 1);
    } else {
        recent->hide();
        layout->addStretch(1);
    }
    layout->addLayout(buttons);
}

void WorkspaceDialog::remember(const QString& path)
{
    QSettings settings;
    QStringList recent = settings.value(kRecentKey).toStringList();
    recent.removeAll(path);
    recent.prepend(path);
    while (recent.size() > kMaxRecent)
        recent.removeLast();
    settings.setValue(kRecentKey, recent);
}

void WorkspaceDialog::choose(const QString& path)
{
    m_path = QDir(path).absolutePath();
    remember(m_path);
    accept();
}

void WorkspaceDialog::openExisting()
{
    const QString p = QFileDialog::getExistingDirectory(this, tr("Open notes directory"), QDir::homePath());
    if (!p.isEmpty())
        choose(p);
}

void WorkspaceDialog::createNew()
{
    const QString parent = QFileDialog::getExistingDirectory(this, tr("Where should the new directory go?"), QDir::homePath());
    if (parent.isEmpty())
        return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("New notes directory"), tr("Directory name:"),
        QLineEdit::Normal, QStringLiteral("Notes"), &ok)
                             .trimmed();
    if (!ok || name.isEmpty())
        return;
    if (name.contains(QLatin1Char('/')) || name.contains(QLatin1Char('\\')) || !QDir(parent).mkdir(name)) {
        QMessageBox::warning(this, tr("New notes directory"),
            tr("Cannot create \"%1\". The name may be invalid or already in use.").arg(name));
        return;
    }
    choose(QDir(parent).filePath(name));
}

#include "WelcomeDialog.h"
#include "ui/Icons.h"
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
const char* const kRecentKey = "recentDirectories";
constexpr int kMaxRecent = 6;
} // namespace

WelcomeDialog::WelcomeDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("WanShiTong — Choose notes directory"));
    setMinimumSize(520, 420);

    auto* title = new QLabel(tr("Welcome to WanShiTong"), this);
    title->setObjectName(QStringLiteral("heading"));
    auto* subtitle = new QLabel(tr("Choose the directory where your notes live."), this);
    subtitle->setObjectName(QStringLiteral("muted"));
    auto* recentLabel = new QLabel(tr("Recent"), this);
    recentLabel->setObjectName(QStringLiteral("muted"));

    m_recent = new QListWidget(this);
    m_recent->setObjectName(QStringLiteral("recentList"));
    m_recent->setIconSize(QSize(22, 22));
    const QStringList recent = QSettings().value(QLatin1String(kRecentKey)).toStringList();
    for (const QString& path : recent) {
        if (!QFileInfo(path).isDir())
            continue;
        auto* item = new QListWidgetItem(makeIcon(Glyph::Folder),
            QStringLiteral("%1\n%2").arg(QFileInfo(path).fileName(), QDir::toNativeSeparators(path)), m_recent);
        item->setData(Qt::UserRole, path);
    }

    auto* create = new QPushButton(tr("Create new…"), this);
    auto* open = new QPushButton(tr("Open directory…"), this);
    auto* openRecent = new QPushButton(tr("Open selected"), this);
    openRecent->setObjectName(QStringLiteral("primary"));

    auto* buttons = new QHBoxLayout;
    buttons->addWidget(create);
    buttons->addWidget(open);
    buttons->addStretch();
    buttons->addWidget(openRecent);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 24, 28, 22);
    layout->setSpacing(10);
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(10);
    layout->addWidget(recentLabel);
    layout->addWidget(m_recent, 1);
    layout->addSpacing(6);
    layout->addLayout(buttons);

    auto openItem = [this](QListWidgetItem* item) {
        if (item)
            choose(item->data(Qt::UserRole).toString());
    };
    connect(create, &QPushButton::clicked, this, &WelcomeDialog::createNew);
    connect(open, &QPushButton::clicked, this, &WelcomeDialog::openExisting);
    connect(openRecent, &QPushButton::clicked, this, [this, openItem] { openItem(m_recent->currentItem()); });
    connect(m_recent, &QListWidget::itemActivated, this, openItem);

    if (m_recent->count() > 0) {
        m_recent->setCurrentRow(0);
        openRecent->setDefault(true);
    } else {
        recentLabel->setText(tr("No recent directories yet."));
        openRecent->setEnabled(false);
        open->setDefault(true);
    }
}

void WelcomeDialog::openExisting()
{
    const QString path = QFileDialog::getExistingDirectory(this, tr("Open notes directory"), QDir::homePath());
    if (!path.isEmpty())
        choose(path);
}

void WelcomeDialog::createNew()
{
    const QString parent = QFileDialog::getExistingDirectory(this,
        tr("Choose where to create the new directory"), QDir::homePath());
    if (parent.isEmpty())
        return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("New notes directory"), tr("Directory name:"),
        QLineEdit::Normal, tr("Notes"), &ok)
                             .trimmed();
    if (!ok)
        return;
    if (name.isEmpty() || name.startsWith(QLatin1Char('.')) || name.contains(QLatin1Char('/'))
        || name.contains(QLatin1Char('\\')) || !QDir(parent).mkdir(name)) {
        QMessageBox::warning(this, tr("New notes directory"),
            tr("Could not create that directory. Check that the name is valid and not already in use."));
        return;
    }
    choose(QDir(parent).filePath(name));
}

void WelcomeDialog::choose(const QString& path)
{
    m_path = QDir::cleanPath(path);
    QSettings settings;
    QStringList recent = settings.value(QLatin1String(kRecentKey)).toStringList();
    recent.removeAll(m_path);
    recent.prepend(m_path);
    while (recent.size() > kMaxRecent)
        recent.removeLast();
    settings.setValue(QLatin1String(kRecentKey), recent);
    accept();
}

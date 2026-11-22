#include "AppTheme.h"
#include "Theme.h"
#include <QApplication>
#include <QPair>
#include <QPalette>
#include <QSettings>

namespace {
// Note: no QTextEdit rule here, as it would override the editor's page colours.
const char* const kStyleSheet = R"qss(
QToolTip { color:@text@; background:@surface@; border:1px solid @border@; border-radius:4px; padding:4px 6px; }

QMenuBar { background:@chrome@; color:@text@; padding:3px 6px; }
QMenuBar::item { background:transparent; padding:5px 10px; border-radius:5px; }
QMenuBar::item:selected { background:@hover@; }
QMenu { background:@surface@; color:@text@; border:1px solid @border@; padding:5px; }
QMenu::item { padding:6px 28px 6px 28px; border-radius:5px; }
QMenu::item:selected { background:@selection@; }
QMenu::item:disabled { color:@muted@; }
QMenu::separator { height:1px; background:@border@; margin:5px 8px; }

QToolBar { background:@chrome@; border:0; border-bottom:1px solid @border@; spacing:2px; padding:5px 8px; }
QToolBar::separator { width:1px; background:@border@; margin:5px 7px; }
QToolButton { background:transparent; border:0; border-radius:6px; padding:5px; }
QToolButton:hover { background:@hover@; }
QToolButton:pressed, QToolButton:checked { background:@selection@; }
QToolButton::menu-indicator { image:none; }

QComboBox { background:@surface@; color:@text@; border:1px solid @border@; border-radius:6px; padding:4px 10px; }
QComboBox:hover { border-color:@accent@; }
QComboBox:disabled { color:@muted@; }
QComboBox::drop-down { border:0; width:22px; }
QComboBox QAbstractItemView { background:@surface@; color:@text@; border:1px solid @border@;
    selection-background-color:@selection@; selection-color:@text@; outline:0; }

QPushButton { background:@surface@; color:@text@; border:1px solid @border@; border-radius:6px; padding:7px 16px; }
QPushButton:hover { background:@hover@; }
QPushButton:pressed { background:@selection@; }
QPushButton:default { border-color:@accent@; }
QPushButton:disabled { color:@muted@; }
QPushButton#primary { background:@accent@; border-color:@accent@; color:@accentText@; }
QPushButton#primary:hover { background:@accentHover@; }
QPushButton#primary:disabled { background:@surface@; border-color:@border@; color:@muted@; }

QLineEdit, QSpinBox { background:@page@; color:@text@; border:1px solid @border@; border-radius:6px; padding:5px 8px; }
QLineEdit:focus, QSpinBox:focus { border-color:@accent@; }

QLabel#heading { font-size:22pt; font-weight:600; color:@text@; }
QLabel#muted { color:@muted@; }

QFrame#sidebar { background:@sidebar@; }
QPushButton#rootButton { background:transparent; border:0; text-align:left; font-weight:600; padding:5px 4px; }
QPushButton#rootButton:hover { color:@accent@; }
QTreeView#explorerTree { background:transparent; color:@text@; border:0; outline:0; padding:0 6px;
    selection-background-color:@selection@; show-decoration-selected:0; }
QTreeView#explorerTree::item { padding:5px 2px; border-radius:5px; }
QTreeView#explorerTree::item:hover { background:@hover@; }
QTreeView#explorerTree::item:selected { background:@selection@; color:@text@; }

QFrame#folderView { background:@canvas@; }
QListWidget#cards { background:transparent; border:0; outline:0; }

QListWidget#recentList { background:@page@; color:@text@; border:1px solid @border@; border-radius:8px; padding:5px; outline:0; }
QListWidget#recentList::item { padding:7px; border-radius:6px; }
QListWidget#recentList::item:hover { background:@hover@; }
QListWidget#recentList::item:selected { background:@selection@; color:@text@; }

QSplitter::handle { background:@border@; }
QDockWidget::title { background:@chrome@; padding:8px 10px; border-bottom:1px solid @border@; }
QPlainTextEdit { background:@page@; color:@text@; border:0; padding:6px; }
QStatusBar { background:@chrome@; border-top:1px solid @border@; color:@muted@; }

QScrollBar:vertical { background:transparent; width:12px; margin:0; }
QScrollBar:horizontal { background:transparent; height:12px; margin:0; }
QScrollBar::handle { background:@border@; border-radius:4px; margin:2px; }
QScrollBar::handle:vertical { min-height:32px; }
QScrollBar::handle:horizontal { min-width:32px; }
QScrollBar::handle:hover { background:@muted@; }
QScrollBar::add-line, QScrollBar::sub-line { width:0; height:0; }
QScrollBar::add-page, QScrollBar::sub-page { background:transparent; }
)qss";
} // namespace

QString savedThemeId()
{
    return QSettings().value(QStringLiteral("theme"), QStringLiteral("midnight")).toString();
}

void applyTheme(const QString& id)
{
    md::setTheme(id);
    const md::Theme& t = md::theme();
    QSettings().setValue(QStringLiteral("theme"), t.id);

    QPalette p;
    p.setColor(QPalette::Window, t.chrome);
    p.setColor(QPalette::WindowText, t.text);
    p.setColor(QPalette::Base, t.page);
    p.setColor(QPalette::AlternateBase, t.surface);
    p.setColor(QPalette::ToolTipBase, t.surface);
    p.setColor(QPalette::ToolTipText, t.text);
    p.setColor(QPalette::Text, t.text);
    p.setColor(QPalette::Button, t.surface);
    p.setColor(QPalette::ButtonText, t.text);
    p.setColor(QPalette::BrightText, Qt::red);
    p.setColor(QPalette::Link, t.link);
    p.setColor(QPalette::Highlight, t.selection);
    p.setColor(QPalette::HighlightedText, t.text);
    p.setColor(QPalette::PlaceholderText, t.muted);
    p.setColor(QPalette::Dark, t.border);
    for (auto role : { QPalette::Text, QPalette::WindowText, QPalette::ButtonText })
        p.setColor(QPalette::Disabled, role, t.muted);
    qApp->setPalette(p);

    const QVector<QPair<QString, QColor>> colors {
        { QStringLiteral("@chrome@"), t.chrome },
        { QStringLiteral("@sidebar@"), t.sidebar },
        { QStringLiteral("@canvas@"), t.canvas },
        { QStringLiteral("@page@"), t.page },
        { QStringLiteral("@surface@"), t.surface },
        { QStringLiteral("@hover@"), t.hover },
        { QStringLiteral("@selection@"), t.selection },
        { QStringLiteral("@border@"), t.border },
        { QStringLiteral("@text@"), t.text },
        { QStringLiteral("@muted@"), t.muted },
        { QStringLiteral("@accent@"), t.accent },
        { QStringLiteral("@accentText@"), t.accentText },
        { QStringLiteral("@accentHover@"), t.accent.darker(112) },
    };
    QString css = QString::fromLatin1(kStyleSheet);
    for (const auto& color : colors)
        css.replace(color.first, color.second.name());
    qApp->setStyleSheet(css);
}

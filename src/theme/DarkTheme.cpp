#include "DarkTheme.h"
#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

void applyDarkTheme(QApplication& app)
{
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    const QColor window(0x1e, 0x21, 0x26), base(0x16, 0x18, 0x1c), button(0x2b, 0x2f, 0x36);
    const QColor text(0xdd, 0xe1, 0xe6), disabled(0x6b, 0x72, 0x7c), hl(0x3a, 0x6e, 0xc8);
    QPalette p;
    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, base);
    p.setColor(QPalette::AlternateBase, button);
    p.setColor(QPalette::ToolTipBase, button);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, button);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::BrightText, Qt::red);
    p.setColor(QPalette::Link, QColor(0x6c, 0xb6, 0xff));
    p.setColor(QPalette::Highlight, hl);
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::PlaceholderText, disabled);
    p.setColor(QPalette::Dark, QColor(0x5a, 0x60, 0x68));
    for (auto role : { QPalette::Text, QPalette::WindowText, QPalette::ButtonText })
        p.setColor(QPalette::Disabled, role, disabled);
    app.setPalette(p);

    // Note: no QTextEdit rule here, as it would override the editor's page colours.
    app.setStyleSheet(QString::fromLatin1(R"qss(
QToolTip { color:#e6eaf0; background:#2b2f36; border:1px solid #444c56; border-radius:4px; padding:4px 6px; }

QMenuBar { background:#1e2126; padding:3px 6px; }
QMenuBar::item { background:transparent; padding:5px 10px; border-radius:5px; }
QMenuBar::item:selected { background:#2d323b; }
QMenu { background:#252930; border:1px solid #3a404a; padding:5px; }
QMenu::item { padding:6px 28px 6px 28px; border-radius:5px; }
QMenu::item:selected { background:#34425e; }
QMenu::item:disabled { color:#6b727c; }
QMenu::separator { height:1px; background:#3a404a; margin:5px 8px; }

QToolBar { background:#1e2126; border:0; border-bottom:1px solid #30353e; spacing:2px; padding:5px 8px; }
QToolBar::separator { width:1px; background:#353b45; margin:5px 7px; }
QToolButton { background:transparent; border:0; border-radius:6px; padding:5px; }
QToolButton:hover { background:#2d323b; }
QToolButton:pressed { background:#383f4a; }
QToolButton:checked { background:#2f3d5c; }

QComboBox { background:#262a31; border:1px solid #383e48; border-radius:6px; padding:4px 10px; }
QComboBox:hover { border-color:#4b5361; }
QComboBox::drop-down { border:0; width:22px; }
QComboBox QAbstractItemView { background:#252930; border:1px solid #3a404a; selection-background-color:#34425e; outline:0; }

QPushButton { background:#2b2f36; border:1px solid #3d444f; border-radius:6px; padding:7px 16px; }
QPushButton:hover { background:#343a43; }
QPushButton:pressed { background:#3d444f; }
QPushButton:default { border-color:#4f8cff; }
QPushButton:disabled { color:#6b727c; border-color:#30353e; }
QPushButton#primary { background:#3a6ec8; border-color:#3a6ec8; color:white; }
QPushButton#primary:hover { background:#477dda; }
QPushButton#primary:disabled { background:#2b2f36; border-color:#30353e; color:#6b727c; }

QLineEdit, QSpinBox { background:#16181c; border:1px solid #3a404a; border-radius:6px; padding:5px 8px; }
QLineEdit:focus, QSpinBox:focus { border-color:#4f8cff; }

QLabel#heading { font-size:22pt; font-weight:600; color:#f0f3f7; }
QLabel#muted { color:#8b95a1; }

QFrame#sidebar { background:#1b1e23; }
QPushButton#rootButton { background:transparent; border:0; text-align:left; font-weight:600; padding:5px 4px; }
QPushButton#rootButton:hover { color:#8ab4ff; }
QTreeView#explorerTree { background:transparent; border:0; outline:0; padding:0 6px;
    selection-background-color:#2f3d5c; show-decoration-selected:0; }
QTreeView#explorerTree::item { padding:5px 2px; border-radius:5px; }
QTreeView#explorerTree::item:hover { background:#262b33; }
QTreeView#explorerTree::item:selected { background:#2f3d5c; color:white; }

QFrame#folderView { background:#17191d; }
QListWidget#cards { background:transparent; border:0; outline:0; }

QListWidget#recentList { background:#16181c; border:1px solid #30353e; border-radius:8px; padding:5px; outline:0; }
QListWidget#recentList::item { padding:7px; border-radius:6px; }
QListWidget#recentList::item:hover { background:#252a32; }
QListWidget#recentList::item:selected { background:#2f3d5c; color:white; }

QSplitter::handle { background:#30353e; }
QDockWidget::title { background:#1e2126; padding:8px 10px; border-bottom:1px solid #30353e; }
QPlainTextEdit { background:#16181c; border:0; padding:6px; }
QStatusBar { background:#1e2126; border-top:1px solid #30353e; color:#8b95a1; }

QScrollBar:vertical { background:transparent; width:12px; margin:0; }
QScrollBar:horizontal { background:transparent; height:12px; margin:0; }
QScrollBar::handle { background:#3b424d; border-radius:4px; margin:2px; }
QScrollBar::handle:vertical { min-height:32px; }
QScrollBar::handle:horizontal { min-width:32px; }
QScrollBar::handle:hover { background:#525b69; }
QScrollBar::add-line, QScrollBar::sub-line { width:0; height:0; }
QScrollBar::add-page, QScrollBar::sub-page { background:transparent; }
)qss"));
}

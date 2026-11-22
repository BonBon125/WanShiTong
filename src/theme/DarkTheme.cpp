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

    // Note: deliberately no QTextEdit rule, NoteEditor colours itself via its palette.
    app.setStyleSheet(QStringLiteral(R"qss(
        QToolTip { color:#dde1e6; background:#2b2f36; border:1px solid #444c56; padding:4px 6px; }
        QPlainTextEdit { border:0; }

        QMenuBar { background:#1e2126; padding:2px 6px; }
        QMenuBar::item { background:transparent; padding:4px 10px; border-radius:5px; }
        QMenuBar::item:selected { background:#2b3038; }
        QMenu { background:#23272e; border:1px solid #353b45; padding:5px; }
        QMenu::item { padding:5px 28px 5px 24px; border-radius:5px; }
        QMenu::item:selected { background:#2f4468; }
        QMenu::item:disabled { color:#6b727c; }
        QMenu::separator { height:1px; background:#353b45; margin:5px 8px; }

        QToolBar { background:#1e2126; border:0; border-bottom:1px solid #2c313a; padding:5px 8px; spacing:2px; }
        QToolBar::separator { width:1px; background:#353b45; margin:5px 6px; }
        QToolButton { background:transparent; border:0; border-radius:6px; padding:5px; }
        QToolButton:hover { background:#2b3038; }
        QToolButton:pressed { background:#353b45; }
        QToolButton:checked { background:#2f4468; }

        QComboBox { background:#262a31; border:1px solid #353b45; border-radius:6px; padding:4px 10px; min-width:130px; }
        QComboBox:hover { border-color:#565f6b; }
        QComboBox QAbstractItemView { background:#23272e; border:1px solid #353b45; selection-background-color:#2f4468; }

        QPushButton { background:#2b2f36; border:1px solid #3a414b; border-radius:6px; padding:7px 14px; }
        QPushButton:hover { background:#343a43; }
        QPushButton[accent="true"] { background:#3a6ec8; border-color:#3a6ec8; color:white; }
        QPushButton[accent="true"]:hover { background:#4a7ed8; }

        QLineEdit { background:#16181c; border:1px solid #353b45; border-radius:6px; padding:5px 8px; }
        QLineEdit:focus { border-color:#3a6ec8; }

        #explorer { background:#1e2126; }
        #workspaceTitle { font-weight:bold; padding:4px 6px; }
        QTreeView { background:#1e2126; border:0; outline:0; show-decoration-selected:0; padding:0 4px; }
        QTreeView::item { padding:4px 2px; border-radius:5px; }
        QTreeView::item:hover { background:#272b32; }
        QTreeView::item:selected { background:#2f4468; color:white; }

        #contentArea, #folderView { background:#17191d; }
        #pathBar { padding:9px 18px 3px 18px; }
        #heroTitle { font-size:20pt; font-weight:bold; }
        #muted { color:#8b95a1; }
        #cards { background:transparent; border:0; outline:0; }
        #recentList { background:#16181c; border:1px solid #353b45; border-radius:8px; outline:0; padding:4px; }
        #recentList::item { padding:8px; border-radius:6px; }
        #recentList::item:hover { background:#272b32; }
        #recentList::item:selected { background:#2f4468; }

        QSplitter::handle { background:#2c313a; }
        QStatusBar { background:#1e2126; color:#8b95a1; }
        QDockWidget::title { background:#1e2126; padding:7px 10px; }

        QScrollBar:vertical { background:transparent; width:10px; margin:0; }
        QScrollBar:horizontal { background:transparent; height:10px; margin:0; }
        QScrollBar::handle { background:#3a414b; border-radius:4px; margin:1px; }
        QScrollBar::handle:vertical { min-height:30px; }
        QScrollBar::handle:horizontal { min-width:30px; }
        QScrollBar::handle:hover { background:#565f6b; }
        QScrollBar::add-line, QScrollBar::sub-line { width:0; height:0; }
        QScrollBar::add-page, QScrollBar::sub-page { background:transparent; }
    )qss"));
}

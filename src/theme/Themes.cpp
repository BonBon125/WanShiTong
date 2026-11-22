#include "Themes.h"
#include <QApplication>
#include <QPalette>
#include <QSettings>

const QVector<Theme>& availableThemes()
{
    // Colour order matches the fields of struct Theme.
    static const QVector<Theme> themes {
        midnightTheme(),
        { "tokyonight", "Tokyo Night",
            0x16161e, 0x16161e, 0x1a1b26, 0x24283b, 0x1f2335, 0x292e42, 0x33467c, 0x3b4261, 0xc0caf5, 0x7982a9, 0x7aa2f7,
            0x7dcfff, 0x3b4261, 0x1a1b26, 0x2c3047, 0x6b5a2a,
            0xbb9af7, 0x9ece6a, 0xff9e64, 0x565f89 },
        { "gruvbox-dark", "Gruvbox Dark",
            0x1d2021, 0x1d2021, 0x1d2021, 0x282828, 0x32302f, 0x3c3836, 0x504945, 0x504945, 0xebdbb2, 0xa89984, 0xfabd2f,
            0x83a598, 0x3c3836, 0x1d2021, 0x32302f, 0x7c5b12,
            0xfb4934, 0xb8bb26, 0xd3869b, 0x928374 },
        { "gruvbox-light", "Gruvbox Light",
            0xebdbb2, 0xebdbb2, 0xf2e5bc, 0xfbf1c7, 0xfbf1c7, 0xe0cfa6, 0xd5c4a1, 0xd5c4a1, 0x3c3836, 0x7c6f64, 0xaf3a03,
            0x076678, 0xebdbb2, 0xf2e5bc, 0xf2e5bc, 0xf8d878,
            0x9d0006, 0x79740e, 0x8f3f71, 0x928374 },
        { "catppuccin-mocha", "Catppuccin Mocha",
            0x181825, 0x181825, 0x11111b, 0x1e1e2e, 0x252536, 0x313244, 0x45475a, 0x45475a, 0xcdd6f4, 0x9399b2, 0xcba6f7,
            0x89b4fa, 0x313244, 0x181825, 0x28283a, 0x6e5f32,
            0xcba6f7, 0xa6e3a1, 0xfab387, 0x7f849c },
        { "nord", "Nord",
            0x2e3440, 0x2e3440, 0x292e39, 0x3b4252, 0x353c4a, 0x434c5e, 0x4c566a, 0x4c566a, 0xeceff4, 0xa7b1c2, 0x88c0d0,
            0x8fbcbb, 0x4c566a, 0x2e3440, 0x434c5e, 0x7d6a3d,
            0x81a1c1, 0xa3be8c, 0xb48ead, 0x7b88a1 },
        { "dracula", "Dracula",
            0x21222c, 0x21222c, 0x191a21, 0x282a36, 0x2f313f, 0x383a4a, 0x44475a, 0x44475a, 0xf8f8f2, 0x9ea4c4, 0xbd93f9,
            0x8be9fd, 0x44475a, 0x21222c, 0x343746, 0x6b6a2a,
            0xff79c6, 0xf1fa8c, 0xbd93f9, 0x6272a4 },
        { "solarized-light", "Solarized Light",
            0xeee8d5, 0xeee8d5, 0xe8e1cc, 0xfdf6e3, 0xfdf6e3, 0xe4ddc8, 0xd9d2bd, 0xd3cbb7, 0x073642, 0x657b83, 0x268bd2,
            0x268bd2, 0xeee8d5, 0xf3ecd9, 0xf5eedb, 0xf5df8f,
            0x859900, 0x2aa198, 0xd33682, 0x93a1a1 },
    };
    return themes;
}

QString savedThemeId()
{
    return QSettings().value(QStringLiteral("theme"), QStringLiteral("midnight")).toString();
}

void useTheme(const QString& id)
{
    Theme theme = midnightTheme();
    for (const Theme& t : availableThemes())
        if (id == QLatin1String(t.id))
            theme = t;
    activeTheme() = theme;
    QSettings().setValue(QStringLiteral("theme"), QString::fromLatin1(theme.id));

    const QColor window(theme.window), surface(theme.surface), text(theme.text), muted(theme.muted);
    const QColor accent(theme.accent), selection(theme.selection);
    const QColor disabled = mixColors(muted, window, 0.45);
    const QColor onAccent = qGray(theme.accent) > 140 ? QColor(0x1a, 0x1a, 0x1a) : QColor(Qt::white);

    QPalette p;
    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, surface);
    p.setColor(QPalette::AlternateBase, window);
    p.setColor(QPalette::ToolTipBase, surface);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, surface);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::BrightText, Qt::red);
    p.setColor(QPalette::Link, QColor(theme.link));
    p.setColor(QPalette::Highlight, selection);
    p.setColor(QPalette::HighlightedText, text);
    p.setColor(QPalette::PlaceholderText, disabled);
    p.setColor(QPalette::Dark, QColor(theme.border));
    for (auto role : { QPalette::Text, QPalette::WindowText, QPalette::ButtonText })
        p.setColor(QPalette::Disabled, role, disabled);
    qApp->setPalette(p);

    // Note: no QTextEdit rule here, as it would override the editor's page colours.
    QString css = QString::fromLatin1(R"qss(
QToolTip { color:@text; background:@surface; border:1px solid @border; border-radius:4px; padding:4px 6px; }

QMenuBar { background:@window; color:@text; padding:3px 6px; }
QMenuBar::item { background:transparent; padding:5px 10px; border-radius:5px; }
QMenuBar::item:selected { background:@hover; }
QMenu { background:@surface; color:@text; border:1px solid @border; padding:5px; }
QMenu::item { padding:6px 28px 6px 28px; border-radius:5px; }
QMenu::item:selected { background:@selection; }
QMenu::item:disabled { color:@disabled; }
QMenu::separator { height:1px; background:@border; margin:5px 8px; }

QToolBar { background:@window; border:0; border-bottom:1px solid @border; spacing:2px; padding:5px 8px; }
QToolBar::separator { width:1px; background:@border; margin:5px 6px; }
QToolButton { background:transparent; border:0; border-radius:6px; padding:5px; }
QToolButton:hover { background:@hover; }
QToolButton:pressed, QToolButton:checked { background:@selection; }
QToolButton::menu-indicator { image:none; }

QComboBox { background:@surface; color:@text; border:1px solid @border; border-radius:6px; padding:4px 10px; }
QComboBox:hover { border-color:@accent; }
QComboBox:disabled { color:@disabled; }
QComboBox::drop-down { border:0; width:22px; }
QComboBox QAbstractItemView { background:@surface; border:1px solid @border; selection-background-color:@selection; outline:0; }

QPushButton { background:@surface; color:@text; border:1px solid @border; border-radius:6px; padding:7px 16px; }
QPushButton:hover { background:@hover; }
QPushButton:pressed { background:@selection; }
QPushButton:default { border-color:@accent; }
QPushButton:disabled { color:@disabled; }
QPushButton#primary { background:@accent; border-color:@accent; color:@onAccent; }
QPushButton#primary:hover { background:@hiAccent; }
QPushButton#primary:disabled { background:@surface; border-color:@border; color:@disabled; }

QLineEdit, QSpinBox { background:@surface; color:@text; border:1px solid @border; border-radius:6px; padding:5px 8px; }
QLineEdit:focus, QSpinBox:focus { border-color:@accent; }

QLabel#heading { font-size:22pt; font-weight:600; color:@text; }
QLabel#muted { color:@muted; }

QFrame#sidebar { background:@sidebar; }
QPushButton#rootButton { background:transparent; border:0; text-align:left; font-weight:600; padding:5px 4px; }
QPushButton#rootButton:hover { color:@accent; }
QTreeView#explorerTree { background:transparent; color:@text; border:0; outline:0; padding:0 6px;
    selection-background-color:@selection; show-decoration-selected:0; }
QTreeView#explorerTree::item { padding:5px 2px; border-radius:5px; }
QTreeView#explorerTree::item:hover { background:@hover; }
QTreeView#explorerTree::item:selected { background:@selection; color:@text; }

QFrame#folderView { background:@canvas; }
QListWidget#cards { background:transparent; border:0; outline:0; }

QListWidget#recentList { background:@surface; border:1px solid @border; border-radius:8px; padding:5px; outline:0; }
QListWidget#recentList::item { padding:7px; border-radius:6px; color:@text; }
QListWidget#recentList::item:hover { background:@hover; }
QListWidget#recentList::item:selected { background:@selection; }

QSplitter::handle { background:@border; }
QDockWidget { color:@text; }
QDockWidget::title { background:@window; padding:8px 10px; border-bottom:1px solid @border; }
QPlainTextEdit { background:@surface; color:@text; border:0; padding:6px; }
QStatusBar { background:@window; border-top:1px solid @border; color:@muted; }

QScrollBar:vertical { background:transparent; width:12px; margin:0; }
QScrollBar:horizontal { background:transparent; height:12px; margin:0; }
QScrollBar::handle { background:@border; border-radius:4px; margin:2px; }
QScrollBar::handle:vertical { min-height:32px; }
QScrollBar::handle:horizontal { min-width:32px; }
QScrollBar::handle:hover { background:@muted; }
QScrollBar::add-line, QScrollBar::sub-line { width:0; height:0; }
QScrollBar::add-page, QScrollBar::sub-page { background:transparent; }
)qss");

    const auto set = [&css](const char* key, const QColor& color) {
        css.replace(QLatin1String(key), color.name());
    };
    set("@window", window);
    set("@sidebar", QColor(theme.sidebar));
    set("@canvas", QColor(theme.canvas));
    set("@surface", surface);
    set("@hover", QColor(theme.hover));
    set("@selection", selection);
    set("@border", QColor(theme.border));
    set("@text", text);
    set("@muted", muted);
    set("@disabled", disabled);
    set("@hiAccent", mixColors(accent, text, 0.15));
    set("@accent", accent);
    set("@onAccent", onAccent);
    qApp->setStyleSheet(css);
}

#pragma once
#include <QColor>
#include <QString>
#include <QVector>

namespace md {

// Every colour used by the application. The field order matters for Theme.cpp.
struct Theme {
    QString id;
    QString name;
    // interface
    QColor chrome; // menu bar, toolbar, status bar
    QColor sidebar;
    QColor canvas; // area around the page
    QColor page; // writing surface
    QColor surface; // menus, buttons, cards, table headers
    QColor hover;
    QColor selection;
    QColor border;
    QColor text;
    QColor muted;
    QColor accent;
    QColor accentText; // text drawn on top of the accent colour
    // note content
    QColor link;
    QColor highlight;
    QColor inlineCode;
    QColor codeBlock;
    QColor quote;
    // syntax highlighting
    QColor keyword;
    QColor string;
    QColor number;
    QColor comment;
};

const QVector<Theme>& themes();
const Theme& theme(); // the active theme
void setTheme(const QString& id); // unknown ids select the first theme

} // namespace md

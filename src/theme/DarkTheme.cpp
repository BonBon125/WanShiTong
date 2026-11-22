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

    app.setStyleSheet(QStringLiteral(
        "QToolTip { color:#dde1e6; background:#2b2f36; border:1px solid #444c56; }"
        "QToolBar { border:0; spacing:3px; padding:2px; }"
        "QPlainTextEdit { border:0; }"));
}

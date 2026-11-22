#pragma once
#include <QColor>

// A colour theme. Header-only so the Markdown library can read the active colours.
struct Theme {
    const char* id;
    const char* name;
    // interface
    QRgb window, sidebar, canvas, page, surface, hover, selection, border, text, muted, accent;
    // document
    QRgb link, inlineCode, codeBlock, quote, highlight;
    // syntax highlighting
    QRgb keyword, string, number, comment;
};

inline Theme midnightTheme()
{
    return { "midnight", "Midnight",
        0x1e2126, 0x1b1e23, 0x17191d, 0x2a2e35, 0x252930, 0x2d323b, 0x2f3d5c, 0x383f4a, 0xdde1e6, 0x8b95a1, 0x4f8cff,
        0x6cb6ff, 0x3d444e, 0x1f2329, 0x323844, 0x7a6200,
        0xff7b72, 0xa5d6ff, 0x79c0ff, 0x8b949e };
}

inline Theme& activeTheme()
{
    static Theme theme = midnightTheme();
    return theme;
}

// t = 0 gives a, t = 1 gives b.
inline QColor mixColors(const QColor& a, const QColor& b, qreal t)
{
    return QColor(qRound(a.red() + (b.red() - a.red()) * t),
        qRound(a.green() + (b.green() - a.green()) * t),
        qRound(a.blue() + (b.blue() - a.blue()) * t));
}

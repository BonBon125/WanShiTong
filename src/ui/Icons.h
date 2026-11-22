#pragma once
#include <QIcon>

// Vector icons drawn in code. They follow the active theme's colours.
enum class Glyph {
    Sidebar,
    NewNote,
    NewFolder,
    Save,
    Rename,
    Trash,
    Bold,
    Italic,
    Strike,
    Highlight,
    Code,
    Subscript,
    Superscript,
    BulletList,
    NumberList,
    TaskList,
    Quote,
    CodeBlock,
    Link,
    Image,
    Table,
    Rule,
    Source,
    Theme,
    Folder,
    Note
};

QIcon makeIcon(Glyph glyph);

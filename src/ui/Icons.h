#pragma once
#include <QIcon>

// Vector icons drawn in code, so no image files or SVG module are needed.
enum class Glyph {
    Sidebar,
    NewNote,
    NewFolder,
    Save,
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
    Folder,
    Note
};

QIcon makeIcon(Glyph glyph);

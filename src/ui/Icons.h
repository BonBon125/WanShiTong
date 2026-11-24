#pragma once
#include <QIcon>

// Vector icons drawn in code, so no image files or SVG module are needed.
// Their colours follow the active theme.
enum class Glyph {
    Sidebar,
    NewNote,
    NewFolder,
    Save,
    Rename,
    Trash,
    Theme,
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

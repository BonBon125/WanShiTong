#pragma once
#include <QColor>
#include <QIcon>

// Small vector icons painted in code, so no image files are needed.
namespace icons {

enum class Glyph {
    Sidebar,
    NewNote,
    NewFolder,
    Folder,
    Note,
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
    Source
};

QIcon get(Glyph glyph, const QColor& color = QColor(0xc9, 0xce, 0xd6));

} // namespace icons

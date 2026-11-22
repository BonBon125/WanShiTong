#include "Icons.h"

#include <QGuiApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

namespace icons {

QIcon get(Glyph glyph, const QColor& color)
{
    QPixmap pm(60, 60);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    p.scale(3, 3); // draw on a 20x20 grid
    const QPen pen(color, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);

    auto line = [&p](qreal x1, qreal y1, qreal x2, qreal y2) { p.drawLine(QPointF(x1, y1), QPointF(x2, y2)); };
    auto text = [&p](const QString& s, int px, const QRectF& r = QRectF(0, 0, 20, 20),
                    bool bold = false, bool italic = false) {
        QFont f = QGuiApplication::font();
        f.setPixelSize(px);
        f.setBold(bold);
        f.setItalic(italic);
        p.setFont(f);
        p.drawText(r, Qt::AlignCenter, s);
    };
    auto folderShape = [] {
        QPainterPath path;
        path.moveTo(2.5, 4.5);
        path.lineTo(7.5, 4.5);
        path.lineTo(9.5, 6.5);
        path.lineTo(17.5, 6.5);
        path.lineTo(17.5, 16);
        path.lineTo(2.5, 16);
        path.closeSubpath();
        return path;
    };
    auto pageShape = [] {
        QPainterPath path;
        path.moveTo(4.5, 2.5);
        path.lineTo(11.5, 2.5);
        path.lineTo(15.5, 6.5);
        path.lineTo(15.5, 17.5);
        path.lineTo(4.5, 17.5);
        path.closeSubpath();
        return path;
    };

    switch (glyph) {
    case Glyph::Sidebar:
        p.drawRoundedRect(QRectF(2.5, 3.5, 15, 13), 2, 2);
        line(7.5, 3.5, 7.5, 16.5);
        break;
    case Glyph::NewNote:
        p.drawPath(pageShape());
        line(10, 8, 10, 14);
        line(7, 11, 13, 11);
        break;
    case Glyph::NewFolder:
        p.drawPath(folderShape());
        line(10, 8.5, 10, 13.5);
        line(7.5, 11, 12.5, 11);
        break;
    case Glyph::Folder:
        p.drawPath(folderShape());
        break;
    case Glyph::Note:
        p.drawPath(pageShape());
        line(7, 9, 13, 9);
        line(7, 12, 13, 12);
        line(7, 15, 11, 15);
        break;
    case Glyph::Bold:
        text("B", 15, QRectF(0, 0, 20, 20), true);
        break;
    case Glyph::Italic:
        text("I", 15, QRectF(0, 0, 20, 20), false, true);
        break;
    case Glyph::Strike:
        text("S", 15);
        line(4, 10.5, 16, 10.5);
        break;
    case Glyph::Highlight:
        p.fillRect(QRectF(3, 14.5, 14, 3.5), QColor(0xe5, 0xc0, 0x3b));
        text("A", 13, QRectF(0, -2, 20, 19), true);
        break;
    case Glyph::Code:
        text("</>", 10, QRectF(0, 0, 20, 20), true);
        break;
    case Glyph::Subscript:
        text("x", 14, QRectF(1, 0, 11, 18));
        text("2", 9, QRectF(11, 9, 8, 10));
        break;
    case Glyph::Superscript:
        text("x", 14, QRectF(1, 2, 11, 18));
        text("2", 9, QRectF(11, 1, 8, 10));
        break;
    case Glyph::BulletList:
        for (int y : { 5, 10, 15 }) {
            p.setBrush(color);
            p.drawEllipse(QPointF(4, y), 1, 1);
            line(8, y, 17, y);
        }
        break;
    case Glyph::NumberList:
        for (int i = 0; i < 3; ++i) {
            const int y = 5 + i * 5;
            text(QString::number(i + 1), 6, QRectF(1, y - 4, 6, 8), true);
            line(9, y, 17, y);
        }
        break;
    case Glyph::TaskList:
        p.drawRoundedRect(QRectF(2.5, 3.5, 5, 5), 1, 1);
        p.drawRoundedRect(QRectF(2.5, 11.5, 5, 5), 1, 1);
        line(3.5, 6, 4.8, 7.2);
        line(4.8, 7.2, 6.8, 4.8);
        line(10.5, 6, 17.5, 6);
        line(10.5, 14, 17.5, 14);
        break;
    case Glyph::Quote:
        p.setPen(QPen(color, 2.5, Qt::SolidLine, Qt::RoundCap));
        line(4, 4, 4, 16);
        p.setPen(pen);
        line(8, 6, 17, 6);
        line(8, 10, 17, 10);
        line(8, 14, 14, 14);
        break;
    case Glyph::CodeBlock:
        text("{ }", 12, QRectF(0, 0, 20, 19), true);
        break;
    case Glyph::Link:
        p.translate(10, 10);
        p.rotate(-45);
        p.drawRoundedRect(QRectF(-8, -3, 9.5, 6), 3, 3);
        p.drawRoundedRect(QRectF(-1.5, -3, 9.5, 6), 3, 3);
        break;
    case Glyph::Image:
        p.drawRoundedRect(QRectF(2.5, 3.5, 15, 13), 2, 2);
        p.drawEllipse(QPointF(7, 8), 1.4, 1.4);
        line(4, 15, 9, 11);
        line(9, 11, 11.5, 13);
        line(11.5, 13, 14, 10);
        line(14, 10, 17, 13.5);
        break;
    case Glyph::Table:
        p.drawRoundedRect(QRectF(2.5, 3.5, 15, 13), 1.5, 1.5);
        line(2.5, 8, 17.5, 8);
        line(2.5, 12.2, 17.5, 12.2);
        line(10, 8, 10, 16.5);
        break;
    case Glyph::Source:
        text("M↓", 11, QRectF(0, 0, 20, 20), true);
        break;
    }
    p.end();
    return QIcon(pm);
}

} // namespace icons

#include "Icons.h"
#include <QGuiApplication>
#include <QIconEngine>
#include <QPainter>
#include <QPainterPath>

namespace {
void drawText(QPainter& p, const QRectF& r, const QString& text, int size, bool bold = false)
{
    QFont f = QGuiApplication::font();
    f.setPixelSize(size);
    f.setBold(bold);
    p.setFont(f);
    p.drawText(r, Qt::AlignCenter, text);
}

void lines(QPainter& p, qreal from, qreal to, std::initializer_list<qreal> ys)
{
    for (const qreal y : ys)
        p.drawLine(QPointF(from, y), QPointF(to, y));
}

QPainterPath folderPath()
{
    QPainterPath path(QPointF(3, 5.5));
    path.lineTo(9, 5.5);
    path.lineTo(11.5, 8);
    path.lineTo(21, 8);
    path.lineTo(21, 19);
    path.lineTo(3, 19);
    path.closeSubpath();
    return path;
}

// All icons are drawn on a 24 x 24 grid.
void drawGlyph(QPainter& p, Glyph glyph, const QColor& color)
{
    switch (glyph) {
    case Glyph::Sidebar:
        p.drawRoundedRect(QRectF(3, 4, 18, 16), 3, 3);
        p.drawLine(QPointF(9, 4), QPointF(9, 20));
        break;
    case Glyph::NewNote:
        p.drawPolyline(QPolygonF({ { 11, 4 }, { 5, 4 }, { 5, 20 }, { 18, 20 }, { 18, 12 } }));
        p.drawLine(QPointF(17, 3), QPointF(17, 9));
        p.drawLine(QPointF(14, 6), QPointF(20, 6));
        break;
    case Glyph::NewFolder:
        p.drawPath(folderPath());
        p.drawLine(QPointF(12, 10.5), QPointF(12, 16.5));
        p.drawLine(QPointF(9, 13.5), QPointF(15, 13.5));
        break;
    case Glyph::Folder:
        p.drawPath(folderPath());
        break;
    case Glyph::Note:
        p.drawPolygon(QPolygonF({ { 6, 3 }, { 14, 3 }, { 19, 8 }, { 19, 21 }, { 6, 21 } }));
        p.drawPolyline(QPolygonF({ { 14, 3 }, { 14, 8 }, { 19, 8 } }));
        lines(p, 9, 16, { 12.5, 16.5 });
        break;
    case Glyph::Save:
        p.drawPolygon(QPolygonF({ { 4, 4 }, { 16, 4 }, { 20, 8 }, { 20, 20 }, { 4, 20 } }));
        p.drawRect(QRectF(8, 13, 8, 7));
        lines(p, 8, 14, { 8 });
        break;
    case Glyph::Bold:
        drawText(p, QRectF(0, 0, 24, 24), QStringLiteral("B"), 18, true);
        break;
    case Glyph::Italic:
        lines(p, 10, 17, { 5 });
        lines(p, 7, 14, { 19 });
        p.drawLine(QPointF(13.5, 5), QPointF(10.5, 19));
        break;
    case Glyph::Strike:
        drawText(p, QRectF(0, 0, 24, 24), QStringLiteral("S"), 18);
        lines(p, 4, 20, { 12.5 });
        break;
    case Glyph::Highlight:
        drawText(p, QRectF(0, 0, 24, 18), QStringLiteral("A"), 16, true);
        p.fillRect(QRectF(4, 18, 16, 3.5), QColor(0xe0, 0xb8, 0x20));
        break;
    case Glyph::Code:
        p.drawPolyline(QPolygonF({ { 7.5, 7 }, { 2.5, 12 }, { 7.5, 17 } }));
        p.drawPolyline(QPolygonF({ { 16.5, 7 }, { 21.5, 12 }, { 16.5, 17 } }));
        p.drawLine(QPointF(14, 5), QPointF(10, 19));
        break;
    case Glyph::Subscript:
        drawText(p, QRectF(1, 1, 14, 20), QStringLiteral("x"), 18);
        drawText(p, QRectF(13, 11, 10, 12), QStringLiteral("2"), 11);
        break;
    case Glyph::Superscript:
        drawText(p, QRectF(1, 3, 14, 20), QStringLiteral("x"), 18);
        drawText(p, QRectF(13, 1, 10, 12), QStringLiteral("2"), 11);
        break;
    case Glyph::BulletList:
        lines(p, 10, 21, { 6, 12, 18 });
        p.setBrush(color);
        for (const qreal y : { 6.0, 12.0, 18.0 })
            p.drawEllipse(QPointF(4.5, y), 1.2, 1.2);
        break;
    case Glyph::NumberList:
        lines(p, 10, 21, { 6, 12, 18 });
        drawText(p, QRectF(1, 2, 7, 8), QStringLiteral("1"), 8, true);
        drawText(p, QRectF(1, 8, 7, 8), QStringLiteral("2"), 8, true);
        drawText(p, QRectF(1, 14, 7, 8), QStringLiteral("3"), 8, true);
        break;
    case Glyph::TaskList:
        p.drawRoundedRect(QRectF(3, 4, 6.5, 6.5), 1.5, 1.5);
        p.drawRoundedRect(QRectF(3, 14, 6.5, 6.5), 1.5, 1.5);
        p.drawPolyline(QPolygonF({ { 4.5, 7.2 }, { 6, 8.8 }, { 8.2, 5.8 } }));
        lines(p, 13, 21, { 7.25, 17.25 });
        break;
    case Glyph::Quote:
        p.drawLine(QPointF(4.5, 5), QPointF(4.5, 19));
        lines(p, 9, 21, { 7, 12 });
        lines(p, 9, 16, { 17 });
        break;
    case Glyph::CodeBlock:
        p.drawRoundedRect(QRectF(3, 4, 18, 16), 3, 3);
        p.drawPolyline(QPolygonF({ { 9.5, 9 }, { 6.5, 12 }, { 9.5, 15 } }));
        p.drawPolyline(QPolygonF({ { 14.5, 9 }, { 17.5, 12 }, { 14.5, 15 } }));
        break;
    case Glyph::Link:
        p.translate(12, 12);
        p.rotate(-45);
        p.drawRoundedRect(QRectF(-10.5, -3.5, 12, 7), 3.5, 3.5);
        p.drawRoundedRect(QRectF(-1.5, -3.5, 12, 7), 3.5, 3.5);
        break;
    case Glyph::Image:
        p.drawRoundedRect(QRectF(3, 5, 18, 14), 2, 2);
        p.drawEllipse(QPointF(8.5, 9.5), 1.4, 1.4);
        p.drawPolyline(QPolygonF({ { 4, 17.5 }, { 10, 12.5 }, { 13, 15 }, { 16, 11.5 }, { 21, 16 } }));
        break;
    case Glyph::Table:
        p.drawRoundedRect(QRectF(3, 4, 18, 16), 2, 2);
        lines(p, 3, 21, { 9.5, 14.75 });
        p.drawLine(QPointF(12, 9.5), QPointF(12, 20));
        break;
    case Glyph::Rule:
        lines(p, 3, 21, { 12 });
        lines(p, 7, 17, { 6, 18 });
        break;
    case Glyph::Source:
        p.drawPolyline(QPolygonF({ { 3, 17 }, { 3, 7 }, { 7, 12 }, { 11, 7 }, { 11, 17 } }));
        p.drawLine(QPointF(17, 7), QPointF(17, 17));
        p.drawPolyline(QPolygonF({ { 14, 14 }, { 17, 17 }, { 20, 14 } }));
        break;
    }
}

class GlyphEngine : public QIconEngine {
public:
    explicit GlyphEngine(Glyph glyph)
        : m_glyph(glyph)
    {
    }

    void paint(QPainter* p, const QRect& rect, QIcon::Mode mode, QIcon::State state) override
    {
        QColor color(0xc5, 0xcc, 0xd6);
        if (mode == QIcon::Disabled)
            color = QColor(0x55, 0x5c, 0x67);
        else if (state == QIcon::On)
            color = QColor(0x8a, 0xb4, 0xff);

        const qreal side = qMin(rect.width(), rect.height());
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setRenderHint(QPainter::TextAntialiasing);
        p->translate(rect.x() + (rect.width() - side) / 2, rect.y() + (rect.height() - side) / 2);
        p->scale(side / 24.0, side / 24.0);
        p->setPen(QPen(color, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p->setBrush(Qt::NoBrush);
        drawGlyph(*p, m_glyph, color);
        p->restore();
    }

    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override
    {
        QPixmap pm(size);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        paint(&p, QRect(QPoint(0, 0), size), mode, state);
        return pm;
    }

    QIconEngine* clone() const override { return new GlyphEngine(m_glyph); }

private:
    Glyph m_glyph;
};
} // namespace

QIcon makeIcon(Glyph glyph) { return QIcon(new GlyphEngine(glyph)); }

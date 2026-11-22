#pragma once
#include "markdown/MarkdownWriter.h"
#include <QTextEdit>
#include <QTextListFormat>

class NoteEditor : public QTextEdit {
    Q_OBJECT
public:
    explicit NoteEditor(QWidget* parent = nullptr);

    QString exportMarkdown() const;
    void importMarkdown(const QString& markdown);
    void setBaseDirectory(const QString& dir); // for relative image paths

    md::WriterOptions& writerOptions() { return m_opts; }

    int headingLevel() const;
    bool isSetextHeading() const;

public slots:
    // inline
    void toggleBold(bool on);
    void toggleItalic(bool on);
    void toggleStrike(bool on);
    void toggleHighlight(bool on);
    void toggleInlineCode(bool on);
    void toggleSubscript(bool on);
    void toggleSuperscript(bool on);
    void clearFormatting();
    // blocks
    void setHeading(int level, bool setext = false);
    void insertHardBreak();
    void insertRule(const QString& style);
    void toggleList(QTextListFormat::Style style, bool task);
    void shiftListItem(int delta);
    void changeQuote(int delta);
    void toggleCodeBlock();
    void setCodeLanguage();
    // links & references
    void insertLink();
    void insertReferenceLink();
    void insertAutolink();
    void removeLink();
    void insertImage();
    void insertFootnote();
    // tables
    void insertTable();
    void setColumnAlignment(const QString& a); // "l", "c", "r", ""

signals:
    void stateChanged();

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void paintEvent(QPaintEvent* e) override;
    QMimeData* createMimeDataFromSelection() const override;
    void insertFromMimeData(const QMimeData* source) override;
    bool event(QEvent* e) override;

private:
    QVector<QTextBlock> selectedBlocks() const;
    void insertStyledLink(int kind, const QString& url, const QString& ref, const QString& label);
    void nextCell(bool forward);

    md::WriterOptions m_opts;
    QString m_lastLang;

    bool tryAutoBullet();
    void updatePageMargins();
    void paintOutlinedSquare(QPainter& p, const QTextBlock& block, const QRectF& blockRect) const;
};

#include "codeeditor.h"
#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>

LineNumberArea::LineNumberArea(CodeEditor *editor)
    : QWidget(editor), codeEditor(editor)
{
}

QSize LineNumberArea::sizeHint() const
{
    return QSize(codeEditor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    codeEditor->lineNumberAreaPaintEvent(event);
}

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent), lineNumbersShown(true)
{
    setFont(QFont(QStringLiteral("Consolas"), 11));
    setTabStopDistance(4 * QFontMetricsF(font()).horizontalAdvance(QLatin1Char(' ')));
    setLineWrapMode(QPlainTextEdit::WidgetWidth);

    lineNumberArea = new LineNumberArea(this);

    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth();
    highlightCurrentLine();
}

/* 行号区宽度 = 能容纳最大行号所需的像素宽度 + 左右留白 */
int CodeEditor::lineNumberAreaWidth()
{
    if (!lineNumbersShown)
        return 0;
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) { max /= 10; ++digits; }
    digits = qMax(digits, 2);                       // 至少显示两位，避免宽度跳动
    return 14 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void CodeEditor::updateLineNumberAreaWidth()
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth();
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    lineNumberArea->setGeometry(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height());
}

void CodeEditor::setLineNumbersVisible(bool on)
{
    lineNumbersShown = on;
    lineNumberArea->setVisible(on);
    updateLineNumberAreaWidth();
    updateLineNumberAreaVisible();
}

void CodeEditor::updateLineNumberAreaVisible()
{
    if (lineNumbersShown)
        lineNumberArea->update();
}

/* 行号绘制：浅灰底、灰蓝色行号，当前行加深 */
void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(0xF2, 0xF4, 0xF7));

    QTextBlock block = firstVisibleBlock();
    int blockNumber  = block.blockNumber();
    int top    = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    int currentLine = textCursor().blockNumber();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.setPen(blockNumber == currentLine ? QColor(0x1A, 0x66, 0xC2)
                                                      : QColor(0x8A, 0x97, 0xA8));
            painter.setFont(font());
            painter.drawText(0, top, lineNumberArea->width() - 8,
                             fontMetrics().height(),
                             Qt::AlignRight, QString::number(blockNumber + 1));
        }
        block  = block.next();
        top    = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

/* 当前行高亮：没有选中内容时给当前行铺一层浅蓝底色 */
void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> selections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(QColor(0xE8, 0xF1, 0xFC));
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        sel.cursor = textCursor();
        sel.cursor.clearSelection();
        selections.append(sel);
    }
    setExtraSelections(selections);
    updateLineNumberAreaVisible();
}

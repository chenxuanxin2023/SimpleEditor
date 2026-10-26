#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>

class CodeEditor;

/* 行号区域：跟随编辑器滚动的 QWidget */
class LineNumberArea : public QWidget
{
    Q_OBJECT
public:
    explicit LineNumberArea(CodeEditor *editor);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    CodeEditor *codeEditor;
};

/* 从 QPlainTextEdit 派生出的代码编辑器，支持行号显示与当前行高亮。
   在 Qt Designer 中通过"提升为"(Promote) 操作替换普通 QPlainTextEdit。 */
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit CodeEditor(QWidget *parent = nullptr);

    void setLineNumbersVisible(bool on);
    bool lineNumbersVisible() const { return lineNumbersShown; }

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int  lineNumberAreaWidth();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();
    void updateLineNumberAreaVisible();

private:
    QWidget *lineNumberArea;
    bool lineNumbersShown;
};

#endif // CODEEDITOR_H

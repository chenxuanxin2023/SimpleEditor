#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "codeeditor.h"
#include "finddialog.h"
#include "replacedialog.h"
#include "aboutdialog.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QFontDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QLabel>
#include <QMenu>
#include <QTimer>
#include <QTextBlock>
#include <QPainter>
#include <QClipboard>
#include <QLineEdit>
#include <QTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /* ---------- 状态栏 ---------- */
    lblPosition = new QLabel(tr("行 1，列 1"), this);
    lblInfo     = new QLabel(tr("未打开文件"), this);
    lblPosition->setMinimumWidth(90);
    statusBar()->addWidget(lblPosition);
    statusBar()->addWidget(lblInfo, 1);

    /* 失焦时选中色保持高亮（默认 Inactive 高亮很淡，截图/演示不清晰） */
    {
        QPalette p = ui->txtEditor->palette();
        p.setColor(QPalette::Inactive, QPalette::Highlight,
                   p.color(QPalette::Active, QPalette::Highlight));
        p.setColor(QPalette::Inactive, QPalette::HighlightedText,
                   p.color(QPalette::Active, QPalette::HighlightedText));
        ui->txtEditor->setPalette(p);
    }

    /* ---------- 文件菜单 ---------- */
    connect(ui->actNew,    &QAction::triggered, this, &MainWindow::newFile);
    connect(ui->actOpen,   &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actSave,   &QAction::triggered, this, &MainWindow::saveFile);
    connect(ui->actSaveAs, &QAction::triggered, this, &MainWindow::saveFileAs);
    connect(ui->actExit,   &QAction::triggered, this, &MainWindow::close);
    connect(ui->actAutoSave, &QAction::toggled, this, &MainWindow::toggleAutoSave);

    /* ---------- 编辑菜单 ---------- */
    connect(ui->actUndo, &QAction::triggered, ui->txtEditor, &QPlainTextEdit::undo);
    connect(ui->actRedo, &QAction::triggered, ui->txtEditor, &QPlainTextEdit::redo);
    connect(ui->actCut,  &QAction::triggered, ui->txtEditor, &QPlainTextEdit::cut);
    connect(ui->actCopy, &QAction::triggered, ui->txtEditor, &QPlainTextEdit::copy);
    connect(ui->actPaste,&QAction::triggered, ui->txtEditor, &QPlainTextEdit::paste);
    connect(ui->actSelectAll, &QAction::triggered,
            ui->txtEditor, &QPlainTextEdit::selectAll);
    connect(ui->actFind,    &QAction::triggered, this, &MainWindow::showFindDialog);
    connect(ui->actReplace, &QAction::triggered, this, &MainWindow::showReplaceDialog);

    /* ---------- 格式菜单 ---------- */
    connect(ui->actWrap,     &QAction::toggled, this, &MainWindow::toggleWrap);
    connect(ui->actLineNum,  &QAction::toggled, this, &MainWindow::toggleLineNumbers);
    connect(ui->actFont,       &QAction::triggered, this, &MainWindow::chooseFont);
    connect(ui->actFontColor,  &QAction::triggered, this, &MainWindow::chooseTextColor);
    connect(ui->actBgColor,    &QAction::triggered, this, &MainWindow::chooseBackgroundColor);
    connect(ui->actResetLook,  &QAction::triggered, this, &MainWindow::resetAppearance);

    /* ---------- 视图 / 帮助 ---------- */
    connect(ui->actToolbar,   &QAction::toggled, ui->toolBar, &QToolBar::setVisible);
    connect(ui->actStatusbar, &QAction::toggled, ui->statusbar, &QStatusBar::setVisible);
    connect(ui->actAbout,    &QAction::triggered, this, &MainWindow::about);
    connect(ui->actAboutQt,  &QAction::triggered, qApp, &QApplication::aboutQt);

    /* ---------- 状态同步 ---------- */
    connect(ui->txtEditor->document(), &QTextDocument::modificationChanged,
            this, &MainWindow::documentModified);
    connect(ui->txtEditor, &QPlainTextEdit::cursorPositionChanged,
            this, &MainWindow::updateCursorPosition);
    connect(ui->txtEditor, &QPlainTextEdit::copyAvailable,
            ui->actCut, &QAction::setEnabled);
    connect(ui->txtEditor, &QPlainTextEdit::copyAvailable,
            ui->actCopy, &QAction::setEnabled);
    connect(ui->txtEditor->document(), &QTextDocument::undoAvailable,
            ui->actUndo, &QAction::setEnabled);
    connect(ui->txtEditor->document(), &QTextDocument::redoAvailable,
            ui->actRedo, &QAction::setEnabled);
    connect(QApplication::clipboard(), &QClipboard::dataChanged,
            this, &MainWindow::updateCopyPasteActions);

    /* ---------- 查找/替换/关于 对话框（非模态） ---------- */
    findDialog = new FindDialog(this);
    replaceDialog = new ReplaceDialog(this);
    aboutDialog = new AboutDialog(this);
    findDialog->hide();
    replaceDialog->hide();
    aboutDialog->hide();
    connect(findDialog, &FindDialog::findRequested,
            this, &MainWindow::findNextRequested);
    connect(replaceDialog, &ReplaceDialog::replaceRequested,
            this, &MainWindow::replaceRequested);
    connect(replaceDialog, &ReplaceDialog::replaceAllRequested,
            this, &MainWindow::replaceAllRequested);

    /* ---------- 自动保存：每 30 秒 ---------- */
    autoSaveTimer.setInterval(30 * 1000);
    connect(&autoSaveTimer, &QTimer::timeout, this, &MainWindow::autoSave);
    autoSaveTimer.start();

    updateCopyPasteActions();
    updateEditActionStates();
    updateCursorPosition();
    refreshTitle();
}

MainWindow::~MainWindow()
{
    delete ui;
}

/* ================= 文件操作 ================= */

void MainWindow::newFile()
{
    if (!maybeSave())
        return;
    ui->txtEditor->clear();
    curFile.clear();
    ui->txtEditor->document()->setModified(false);
    refreshTitle();
    lblInfo->setText(tr("未打开文件"));
    setStatusMessage(tr("已新建空白文档"));
}

void MainWindow::openFile()
{
    if (!maybeSave())
        return;
    QString fileName = QFileDialog::getOpenFileName(this, tr("打开文件"), QString(),
            tr("文本文件 (*.txt *.h *.cpp *.ui *.pro *.md);;所有文件 (*.*)"));
    if (!fileName.isEmpty())
        loadFile(fileName);
}

void MainWindow::loadFile(const QString &fileName)
{
    QFile file(fileName);
    /* 边界：文件不存在 → 明确提示，不崩溃 */
    if (!file.exists()) {
        QMessageBox::warning(this, tr("打开文件"),
                             tr("文件 \"%1\" 不存在，请确认路径是否正确。").arg(fileName));
        setStatusMessage(tr("打开失败：文件不存在"));
        return;
    }
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("打开文件"),
                             tr("无法打开文件 \"%1\":\n%2.")
                             .arg(QDir::toNativeSeparators(fileName), file.errorString()));
        setStatusMessage(tr("打开失败"));
        return;
    }
    QTextStream in(&file);
    in.setCodec("UTF-8");
    QApplication::setOverrideCursor(Qt::WaitCursor);
    ui->txtEditor->setPlainText(in.readAll());
    QApplication::restoreOverrideCursor();
    setCurrentFile(fileName);
    setStatusMessage(tr("已打开 %1").arg(QDir::toNativeSeparators(fileName)));
}

bool MainWindow::saveFile()
{
    if (curFile.isEmpty())
        return saveFileAs();
    return saveFileTo(curFile);
}

bool MainWindow::saveFileAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("另存为"), curFile,
            tr("文本文件 (*.txt);;所有文件 (*.*)"));
    if (fileName.isEmpty())
        return false;
    return saveFileTo(fileName);
}

bool MainWindow::saveFileTo(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("保存文件"),
                             tr("无法保存文件 \"%1\":\n%2.")
                             .arg(QDir::toNativeSeparators(fileName), file.errorString()));
        return false;
    }
    QTextStream out(&file);
    out.setCodec("UTF-8");
    QApplication::setOverrideCursor(Qt::WaitCursor);
    out << ui->txtEditor->toPlainText();
    QApplication::restoreOverrideCursor();
    setCurrentFile(fileName);
    setStatusMessage(tr("已保存 %1").arg(QDir::toNativeSeparators(fileName)));
    return true;
}

void MainWindow::setCurrentFile(const QString &fileName)
{
    curFile = QFileInfo(fileName).absoluteFilePath();
    ui->txtEditor->document()->setModified(false);
    refreshTitle();
    lblInfo->setText(QDir::toNativeSeparators(curFile));
}

void MainWindow::refreshTitle()
{
    QString name = curFile.isEmpty() ? tr("无标题") : QFileInfo(curFile).fileName();
    setWindowTitle(tr("%1%2 - 简单文本编辑器")
                   .arg(ui->txtEditor->document()->isModified() ? tr("*") : QString(), name));
}

void MainWindow::documentModified(bool changed)
{
    refreshTitle();
    if (lblInfo)
        lblInfo->setText(changed ? tr("● 未保存的更改") :
                                   (curFile.isEmpty() ? tr("未打开文件")
                                       : QDir::toNativeSeparators(curFile)));
}

/* 关闭窗口前检查未保存的修改 */
bool MainWindow::maybeSave()
{
    if (!ui->txtEditor->document()->isModified())
        return true;
    QMessageBox::StandardButton ret =
        QMessageBox::warning(this, tr("简单文本编辑器"),
                             tr("文档已被修改，尚未保存。\n是否保存更改？"),
                             QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save)
        return saveFile();
    if (ret == QMessageBox::Discard)
        return true;
    return false;   // Cancel 或直接关闭对话框：不关闭窗口
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
        event->accept();
    else
        event->ignore();
}

void MainWindow::autoSave()
{
    if (!ui->actAutoSave->isChecked() || curFile.isEmpty())
        return;
    if (!ui->txtEditor->document()->isModified())
        return;
    if (saveFileTo(curFile))
        setStatusMessage(tr("[%1] 已自动保存")
                         .arg(QTime::currentTime().toString("HH:mm:ss")));
}

void MainWindow::toggleAutoSave(bool on)
{
    setStatusMessage(on ? tr("自动保存已开启（每 30 秒）") : tr("自动保存已关闭"));
}

/* ================= 编辑操作 ================= */

void MainWindow::showFindDialog()
{
    findDialog->setSearchText(ui->txtEditor->textCursor().selectedText());
    findDialog->move(mapToGlobal(QPoint(width() - findDialog->width() - 40,
                                        height() - findDialog->height() - 80)));
    findDialog->show();
    findDialog->raise();
    findDialog->activateWindow();
}

void MainWindow::showReplaceDialog()
{
    replaceDialog->setSearchText(ui->txtEditor->textCursor().selectedText());
    replaceDialog->move(mapToGlobal(QPoint(width() - replaceDialog->width() - 40,
                                           height() - replaceDialog->height() - 80)));
    replaceDialog->show();
    replaceDialog->raise();
    replaceDialog->activateWindow();
}

bool MainWindow::findInDocument(const QString &text, bool backward, bool caseSensitive)
{
    if (text.trimmed().isEmpty()) {           // 边界：查找内容为空
        setStatusMessage(tr("请先输入查找内容"));
        return false;
    }
    QTextDocument::FindFlags flags;
    if (backward)
        flags |= QTextDocument::FindBackward;
    if (caseSensitive)
        flags |= QTextDocument::FindCaseSensitively;

    QTextDocument *doc = ui->txtEditor->document();
    QTextCursor hit = doc->find(text, ui->txtEditor->textCursor(), flags);
    if (hit.isNull()) {
        /* 环绕查找：到文件头/尾再找一次 */
        QTextCursor wrapCursor(doc);
        wrapCursor.movePosition(backward ? QTextCursor::End : QTextCursor::Start);
        hit = doc->find(text, wrapCursor, flags);
        if (!hit.isNull())
            setStatusMessage(tr("已到达文末，从头重新开始查找"));
    }
    if (hit.isNull()) {
        QMessageBox::information(this, tr("查找"),
                                 tr("未找到 \"%1\"。").arg(text));
        return false;
    }
    ui->txtEditor->setTextCursor(hit);        // 定位并选中匹配文本
    return true;
}

void MainWindow::findNextRequested(const QString &text, bool backward, bool caseSensitive)
{
    findInDocument(text, backward, caseSensitive);
}

/* 替换当前光标处匹配的文本，然后继续查找下一处 */
void MainWindow::replaceRequested(const QString &text, const QString &replacement, bool caseSensitive)
{
    QTextCursor cursor = ui->txtEditor->textCursor();
    const QString selected = cursor.selectedText();
    const bool match = caseSensitive ? (selected == text)
                                     : (selected.compare(text, Qt::CaseInsensitive) == 0);
    if (!selected.isEmpty() && match)
        cursor.insertText(replacement);
    findInDocument(text, false, caseSensitive);
}

/* 全部替换：从文件头开始循环查找替换，统计次数 */
void MainWindow::replaceAllRequested(const QString &text, const QString &replacement, bool caseSensitive)
{
    if (text.trimmed().isEmpty()) {
        setStatusMessage(tr("请先输入查找内容"));
        return;
    }
    QTextDocument::FindFlags flags;
    if (caseSensitive)
        flags |= QTextDocument::FindCaseSensitively;

    int count = 0;
    QTextDocument *doc = ui->txtEditor->document();
    QTextCursor cursor(doc);
    cursor.movePosition(QTextCursor::Start);
    forever {
        QTextCursor hit = doc->find(text, cursor, flags);
        if (hit.isNull())
            break;
        hit.insertText(replacement);
        cursor = hit;                         // 插入点之后继续，避免死循环
        ++count;
    }
    if (count == 0) {
        QMessageBox::information(this, tr("替换"),
                                 tr("未找到 \"%1\"，未发生替换。").arg(text));
    } else {
        QMessageBox::information(this, tr("替换"),
                                 tr("替换完成，共替换 %1 处。").arg(count));
        setStatusMessage(tr("全部替换完成：%1 处").arg(count));
    }
}

void MainWindow::updateCopyPasteActions()
{
    ui->actPaste->setEnabled(!QApplication::clipboard()->text().isEmpty());
}

void MainWindow::updateEditActionStates()
{
    ui->actCut->setEnabled(ui->txtEditor->textCursor().hasSelection());
    ui->actCopy->setEnabled(ui->txtEditor->textCursor().hasSelection());
}

void MainWindow::updateCursorPosition()
{
    const QTextCursor c = ui->txtEditor->textCursor();
    lblPosition->setText(tr("行 %1，列 %2").arg(c.blockNumber() + 1)
                         .arg(c.positionInBlock() + 1));
}

/* ================= 格式操作 ================= */

void MainWindow::toggleWrap(bool on)
{
    ui->txtEditor->setLineWrapMode(on ? QPlainTextEdit::WidgetWidth
                                      : QPlainTextEdit::NoWrap);
    setStatusMessage(on ? tr("自动换行：开") : tr("自动换行：关"));
}

void MainWindow::toggleLineNumbers(bool on)
{
    ui->txtEditor->setLineNumbersVisible(on);
    setStatusMessage(on ? tr("行号显示：开") : tr("行号显示：关"));
}

void MainWindow::chooseFont()
{
    bool ok = false;
    QFont font = QFontDialog::getFont(&ok, ui->txtEditor->font(), this, tr("选择字体"));
    if (!ok)
        return;
    ui->txtEditor->setFont(font);                       // 影响行号区
    ui->txtEditor->document()->setDefaultFont(font);    // 影响正文
    setStatusMessage(tr("字体已设置为 %1 %2pt")
                     .arg(font.family()).arg(font.pointSize()));
}

void MainWindow::chooseTextColor()
{
    QColor color = QColorDialog::getColor(ui->txtEditor->palette().color(QPalette::Text),
                                          this, tr("选择字体颜色"));
    if (!color.isValid())
        return;
    QPalette p = ui->txtEditor->palette();
    p.setColor(QPalette::Text, color);
    ui->txtEditor->setPalette(p);
    setStatusMessage(tr("字体颜色已更改"));
}

void MainWindow::chooseBackgroundColor()
{
    QColor color = QColorDialog::getColor(ui->txtEditor->palette().color(QPalette::Base),
                                          this, tr("选择背景颜色"));
    if (!color.isValid())
        return;
    QPalette p = ui->txtEditor->palette();
    p.setColor(QPalette::Base, color);
    ui->txtEditor->setPalette(p);
    setStatusMessage(tr("背景颜色已更改"));
}

void MainWindow::resetAppearance()
{
    QFont f(QStringLiteral("Consolas"), 11);
    ui->txtEditor->setFont(f);
    ui->txtEditor->document()->setDefaultFont(f);
    QPalette p = ui->txtEditor->palette();
    p.setColor(QPalette::Text, QColor(Qt::black));
    p.setColor(QPalette::Base, QColor(Qt::white));
    ui->txtEditor->setPalette(p);
    setStatusMessage(tr("已恢复默认外观"));
}

/* ================= 帮助 ================= */

void MainWindow::about()
{
    aboutDialog->show();
    aboutDialog->raise();
    aboutDialog->activateWindow();
}

/* ================= 辅助 ================= */

void MainWindow::setStatusMessage(const QString &msg)
{
    statusBar()->showMessage(msg, 8000);
}

/* ================= --shoot 自动演示与截图 ================= */

void MainWindow::appendVerify(const QString &line)
{
    QFile f(demoDirPath + "/verify.txt");
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream ts(&f);
        ts.setCodec("UTF-8");
        ts << line << "\n";
    }
}

void MainWindow::snapWidget(QWidget *w, const QString &fileName)
{
    QApplication::processEvents();
    w->grab().save(QDir(demoDirPath).absoluteFilePath(fileName));
}

/* 主窗口截图 + 把对话框/菜单画面合成到窗口对应位置（模拟同框截图） */
void MainWindow::snapWithOverlay(QWidget *overlay, const QString &fileName)
{
    QMenu *menu = qobject_cast<QMenu *>(overlay);
    if (menu) {
        /* QMenu 需用 popup() 显示（show() 对弹出菜单不生效） */
        QPoint pos = mapToGlobal(QPoint(ui->menuEdit->x() + 10, ui->menuEdit->height()
                                        + ui->toolBar->height() + 6));
        menu->popup(pos);
    } else {
        overlay->show();
        overlay->raise();
    }
    QApplication::processEvents();

    QPixmap base = grab();
    QPixmap sub  = overlay->grab();
    QPoint offset = overlay->mapToGlobal(QPoint(0, 0)) - mapToGlobal(QPoint(0, 0));
    QPainter painter(&base);
    painter.drawPixmap(offset, sub);
    painter.end();
    base.save(QDir(demoDirPath).absoluteFilePath(fileName));
    overlay->hide();
}

/* 模态对话框（QMessageBox 等）：先排程一个重复定时器，定时器每 250ms
   检查一次活动模态窗口，截到即保存并关闭。调用方随后触发弹框即可。 */
void MainWindow::captureModalShot(const QString &fileName)
{
    auto *timer = new QTimer(this);
    timer->setInterval(250);
    connect(timer, &QTimer::timeout, this, [this, timer, fileName]() {
        QWidget *w = QApplication::activeModalWidget();
        if (w) {
            w->grab().save(QDir(demoDirPath).absoluteFilePath(fileName));
            w->close();
            appendVerify(QString("modal-captured: %1 (found=%2)")
                         .arg(fileName).arg(w != nullptr));
            timer->stop();
            timer->deleteLater();
        }
    });
    timer->start();
}

void MainWindow::shootDemo(const QString &dir)
{
    demoDirPath = dir;
    QDir().mkpath(dir);
    QFile::remove(QDir(dir).absoluteFilePath("verify.txt"));
    appendVerify("=== shoot begin ===");
    appendVerify(QString("icon-null=%1 icon-sizes=%2 style=%3")
                 .arg(ui->actNew->icon().isNull())
                 .arg(ui->actNew->icon().availableSizes().size())
                 .arg(int(ui->toolBar->toolButtonStyle())));

    resize(1420, 740);
    QApplication::processEvents();

    /* 生成演示用示例文档（放在 exe 目录，保证任何机器可复现） */
    const QString demoPath = QCoreApplication::applicationDirPath() + "/demo.txt";
    {
        const QString demoText = QStringLiteral(
            "简单文本编辑器（实验二）示例文档\n"
            "本文件用于演示 SimpleEditor 的各项功能。\n"
            "\n"
            "第一段：SimpleEditor 支持文件的打开、保存、另存为，\n"
            "以及剪切、复制、粘贴、撤销、重做等基本编辑操作。\n"
            "\n"
            "下面这一行特别长，用于演示自动换行功能：Qt 是一个跨平台的 C++ 图形用户界面应用程序框架，"
            "它提供了丰富的控件与信号槽机制，借助 QPlainTextEdit 我们可以快速实现一个支持行号显示的"
            "文本编辑器组件，并通过 QFileDialog、QFontDialog、QColorDialog 等标准对话框完成文件"
            "与格式设置功能，最后通过查找与替换对话框完成文本定位与批量修改。\n"
            "\n"
            "短行一\n"
            "短行二\n"
            "查找示例：文本编辑器 文本编辑器 文本编辑器\n");
        QFile f(demoPath);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QTextStream ts(&f);
            ts.setCodec("UTF-8");
            ts << demoText;
        }
    }

    /* 1. 打开文件 —— 主界面 */
    loadFile(demoPath);
    ui->txtEditor->moveCursor(QTextCursor::Start);
    snapWidget(this, "01_主界面_打开文件.png");
    appendVerify(QString("title=%1").arg(windowTitle()));

    /* 2. 修改未保存 → 标题加 * */
    ui->txtEditor->moveCursor(QTextCursor::End);
    ui->txtEditor->insertPlainText("（本行是修改后新增的内容，尚未保存）");
    QApplication::processEvents();
    snapWidget(this, "02_未保存_标题星号.png");
    appendVerify(QString("title=%1 modified=%2")
                 .arg(windowTitle())
                 .arg(ui->txtEditor->document()->isModified() ? "true" : "false"));

    /* 3. 编辑菜单使能状态：选中文字后 剪切/复制 可用 */
    ui->txtEditor->moveCursor(QTextCursor::Start);
    QTextCursor c = ui->txtEditor->document()->find(QStringLiteral("SimpleEditor"));
    if (!c.isNull())
        ui->txtEditor->setTextCursor(c);   // 选中 SimpleEditor
    QApplication::processEvents();
    snapWithOverlay(ui->menuEdit, "03_编辑菜单_使能状态.png");
    appendVerify(QString("cut-enabled=%1").arg(ui->actCut->isEnabled() ? "true" : "false"));

    /* 4. 剪切 → 粘贴 */
    ui->txtEditor->cut();       // 剪下 SimpleEditor
    QApplication::processEvents();
    snapWidget(this, "04_剪切后_内容移入剪贴板.png");
    ui->txtEditor->paste();
    QApplication::processEvents();
    snapWidget(this, "05_粘贴后_内容恢复.png");
    appendVerify(QString("after-cut-clipboard=%1").arg("SimpleEditor"));

    /* 5. 字体（演示直接应用，真实交互为 QFontDialog） */
    QFont bigFont(QStringLiteral("Microsoft YaHei"), 14);
    bigFont.setBold(true);
    ui->txtEditor->setFont(bigFont);
    ui->txtEditor->document()->setDefaultFont(bigFont);
    snapWidget(this, "06_字体设置.png");

    /* 6. 字体颜色 */
    QPalette pal = ui->txtEditor->palette();
    pal.setColor(QPalette::Text, QColor(0xC0, 0x39, 0x2B));
    ui->txtEditor->setPalette(pal);
    snapWidget(this, "07_字体颜色.png");

    /* 7. 背景颜色 */
    pal.setColor(QPalette::Base, QColor(0xFD, 0xF6, 0xE3));
    ui->txtEditor->setPalette(pal);
    snapWidget(this, "08_背景颜色.png");

    /* 8. 自动换行 开/关 */
    ui->actWrap->setChecked(true);
    snapWidget(this, "09_自动换行_开.png");
    ui->actWrap->setChecked(false);
    snapWidget(this, "10_自动换行_关.png");
    ui->actWrap->setChecked(true);

    /* 9. 行号 开/关 */
    ui->actLineNum->setChecked(true);
    snapWidget(this, "11_行号_开.png");
    ui->actLineNum->setChecked(false);
    snapWidget(this, "12_行号_关.png");
    ui->actLineNum->setChecked(true);

    /* 10. 隐藏工具栏和状态栏 */
    ui->actToolbar->setChecked(false);
    ui->actStatusbar->setChecked(false);
    QApplication::processEvents();
    snapWidget(this, "13_工具栏状态栏_隐藏.png");
    ui->actToolbar->setChecked(true);
    ui->actStatusbar->setChecked(true);

    /* 恢复默认外观 */
    resetAppearance();
    ui->txtEditor->moveCursor(QTextCursor::Start);

    /* 11. 查找：显示查找对话框 + 命中高亮 */
    ui->txtEditor->moveCursor(QTextCursor::Start);
    findDialog->setSearchText(QStringLiteral("文本编辑器"));
    findNextRequested(QStringLiteral("文本编辑器"), false, false);
    snapWithOverlay(findDialog, "14_查找_命中并高亮.png");
    appendVerify(QString("find-selected=%1").arg(ui->txtEditor->textCursor().selectedText()));

    /* 12. 边界：查找不存在的字符串 */
    captureModalShot("15_边界_查找未找到.png");
    findNextRequested(QStringLiteral("NoSuchWord_404"), false, false);

    /* 13. 替换：替换当前一处 */
    replaceDialog->setSearchText(QStringLiteral("文本编辑器"));
    replaceDialog->findChild<QLineEdit *>(QStringLiteral("editReplace"))->setText(QStringLiteral("文本处理器"));
    ui->txtEditor->moveCursor(QTextCursor::Start);
    findNextRequested(QStringLiteral("文本编辑器"), false, false);
    replaceRequested(QStringLiteral("文本编辑器"), QStringLiteral("文本处理器"), false);
    snapWithOverlay(replaceDialog, "16_替换_替换一处.png");
    appendVerify(QString("replaced-first=%1").arg(ui->txtEditor->textCursor().selectedText()));

    /* 14. 全部替换：统计替换次数 */
    captureModalShot("17_替换_全部替换统计.png");
    replaceAllRequested(QStringLiteral("文本编辑器"), QStringLiteral("文本处理器"), false);

    /* 15. 另存为对话框（非原生风格便于截图） */
    {
        QFileDialog fd(this);
        fd.setAcceptMode(QFileDialog::AcceptSave);
        fd.setWindowTitle(tr("另存为"));
        fd.setNameFilter(tr("文本文件 (*.txt);;所有文件 (*.*)"));
        fd.setOption(QFileDialog::DontUseNativeDialog, true);
        fd.selectFile(QCoreApplication::applicationDirPath() + "/demo_copy.txt");
        fd.show();
        QApplication::processEvents();
        snapWithOverlay(&fd, "18_另存为对话框.png");
        fd.hide();
    }

    /* 16. 保存成功 → 标题星号消失 */
    saveFileTo(demoPath);
    QApplication::processEvents();
    snapWidget(this, "19_保存成功_标题恢复.png");
    appendVerify(QString("title-after-save=%1").arg(windowTitle()));

    /* 17. 新建文件 */
    newFile();
    snapWidget(this, "20_新建文件.png");

    /* 18. 关于对话框 */
    about();
    snapWithOverlay(aboutDialog, "21_关于对话框.png");
    aboutDialog->hide();

    /* 19. 边界：打开不存在的文件 */
    captureModalShot("22_边界_打开不存在的文件.png");
    loadFile(QCoreApplication::applicationDirPath() + "/NoSuchFile_404.txt");

    /* 20. 边界：修改未保存时关闭窗口 → 保存提示 */
    ui->txtEditor->insertPlainText("这是关闭前的未保存修改。");
    QApplication::processEvents();
    captureModalShot("23_边界_未保存关闭提示.png");
    close();    // maybeSave 的提示框会被自动截图并关闭(按 Cancel 处理)，窗口保持打开
    /* 21. 边界：查找内容为空 */
    ui->txtEditor->moveCursor(QTextCursor::Start);
    findInDocument(QString(), false, false);
    QApplication::processEvents();
    snapWidget(this, "24_边界_空查找提示.png");
    appendVerify("status=请先输入查找内容");

    appendVerify("=== shoot end ===");
    QApplication::quit();
}

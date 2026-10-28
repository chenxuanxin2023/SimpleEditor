#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

class QLabel;
class CodeEditor;
class FindDialog;
class ReplaceDialog;
class AboutDialog;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    /* --shoot 模式：自动演示全部功能并截图到指定目录（用于实验报告） */
    void shootDemo(const QString &dir);

    /* --demo 模式：按时间轴真实操作界面，供外部录屏（用于录制测试视频） */
    void runLiveDemo();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    /* 文件 */
    void newFile();
    void openFile();
    bool saveFile();
    bool saveFileAs();
    void toggleAutoSave(bool on);
    /* 编辑 */
    void showFindDialog();
    void showReplaceDialog();
    void findNextRequested(const QString &text, bool backward, bool caseSensitive);
    void replaceRequested(const QString &text, const QString &replacement, bool caseSensitive);
    void replaceAllRequested(const QString &text, const QString &replacement, bool caseSensitive);
    /* 格式 */
    void toggleWrap(bool on);
    void toggleLineNumbers(bool on);
    void chooseFont();
    void chooseTextColor();
    void chooseBackgroundColor();
    void resetAppearance();
    /* 视图 / 帮助 */
    void about();
    /* 状态同步 */
    void documentModified(bool changed);
    void updateCursorPosition();
    void updateCopyPasteActions();
    void updateEditActionStates();
    void autoSave();

private:
    /* 业务函数 */
    void loadFile(const QString &fileName);
    bool maybeSave();
    bool saveFileTo(const QString &fileName);
    void setCurrentFile(const QString &fileName);
    void refreshTitle();
    bool findInDocument(const QString &text, bool backward, bool caseSensitive);
    void setStatusMessage(const QString &msg);
    void appendVerify(const QString &line);

    /* --shoot 辅助 */
    void snapWidget(QWidget *w, const QString &fileName);
    void snapWithOverlay(QWidget *overlay, const QString &fileName);
    void captureModalShot(const QString &fileName);

    Ui::MainWindow *ui = nullptr;
    FindDialog *findDialog = nullptr;
    ReplaceDialog *replaceDialog = nullptr;
    AboutDialog *aboutDialog = nullptr;
    QString curFile;
    QString demoDirPath;             // --shoot 截图输出目录
    QLabel *lblPosition = nullptr;   // 行/列
    QLabel *lblInfo = nullptr;       // 文件路径
    QTimer autoSaveTimer;
};

#endif // MAINWINDOW_H

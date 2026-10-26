#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>

namespace Ui { class FindDialog; }

class FindDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FindDialog(QWidget *parent = nullptr);
    ~FindDialog() override;

    void setSearchText(const QString &text);
    QString searchText() const;

signals:
    /* 点击"查找下一个"发出：text 查找串，backward 是否向上，caseSensitive 区分大小写 */
    void findRequested(const QString &text, bool backward, bool caseSensitive);

private slots:
    void onFindClicked();
    void updateFindButton(const QString &text);

private:
    Ui::FindDialog *ui;
};

#endif // FINDDIALOG_H

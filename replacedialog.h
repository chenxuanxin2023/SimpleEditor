#ifndef REPLACEDIALOG_H
#define REPLACEDIALOG_H

#include <QDialog>

namespace Ui { class ReplaceDialog; }

class ReplaceDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ReplaceDialog(QWidget *parent = nullptr);
    ~ReplaceDialog() override;

    void setSearchText(const QString &text);
    QString searchText() const;

signals:
    void replaceRequested(const QString &text, const QString &replacement, bool caseSensitive);
    void replaceAllRequested(const QString &text, const QString &replacement, bool caseSensitive);

private slots:
    void onReplaceClicked();
    void onReplaceAllClicked();
    void updateButtons();

private:
    Ui::ReplaceDialog *ui;
};

#endif // REPLACEDIALOG_H

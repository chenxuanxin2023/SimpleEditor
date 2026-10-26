#include "finddialog.h"
#include "ui_finddialog.h"

FindDialog::FindDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::FindDialog)
{
    ui->setupUi(this);
    connect(ui->btnFind, &QPushButton::clicked, this, &FindDialog::onFindClicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &FindDialog::hide);
    connect(ui->editFind, &QLineEdit::textChanged,
            this, &FindDialog::updateFindButton);
    updateFindButton(QString());
}

FindDialog::~FindDialog()
{
    delete ui;
}

void FindDialog::updateFindButton(const QString &text)
{
    ui->btnFind->setEnabled(!text.trimmed().isEmpty());
}

void FindDialog::onFindClicked()
{
    emit findRequested(ui->editFind->text(),
                       ui->radioUp->isChecked(),
                       ui->chkCase->isChecked());
}

void FindDialog::setSearchText(const QString &text)
{
    ui->editFind->setText(text);
    ui->editFind->setFocus();
    ui->editFind->selectAll();
}

QString FindDialog::searchText() const
{
    return ui->editFind->text();
}

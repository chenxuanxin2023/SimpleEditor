#include "replacedialog.h"
#include "ui_replacedialog.h"

ReplaceDialog::ReplaceDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::ReplaceDialog)
{
    ui->setupUi(this);
    connect(ui->btnReplace, &QPushButton::clicked, this, &ReplaceDialog::onReplaceClicked);
    connect(ui->btnReplaceAll, &QPushButton::clicked, this, &ReplaceDialog::onReplaceAllClicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &ReplaceDialog::hide);
    connect(ui->editFind, &QLineEdit::textChanged, this, &ReplaceDialog::updateButtons);
    updateButtons();
}

ReplaceDialog::~ReplaceDialog()
{
    delete ui;
}

void ReplaceDialog::updateButtons()
{
    bool ok = !ui->editFind->text().trimmed().isEmpty();
    ui->btnReplace->setEnabled(ok);
    ui->btnReplaceAll->setEnabled(ok);
}

void ReplaceDialog::onReplaceClicked()
{
    emit replaceRequested(ui->editFind->text(),
                          ui->editReplace->text(),
                          ui->chkCase->isChecked());
}

void ReplaceDialog::onReplaceAllClicked()
{
    emit replaceAllRequested(ui->editFind->text(),
                             ui->editReplace->text(),
                             ui->chkCase->isChecked());
}

void ReplaceDialog::setSearchText(const QString &text)
{
    ui->editFind->setText(text);
    ui->editFind->setFocus();
    ui->editFind->selectAll();
}

QString ReplaceDialog::searchText() const
{
    return ui->editFind->text();
}

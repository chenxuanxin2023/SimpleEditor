#include "aboutdialog.h"
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    ui->lblQt->setText(tr("基于 Qt %1 构建").arg(QT_VERSION_STR));
    connect(ui->btnClose, &QPushButton::clicked, this, &QDialog::close);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

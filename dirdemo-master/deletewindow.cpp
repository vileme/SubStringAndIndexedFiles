#include "deletewindow.h"
#include "ui_deletewindow.h"

DeleteWindow::DeleteWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DeleteWindow)
{
    ui->setupUi(this);
    ui->label->setText("Some directories were changed.Do you want to reindex?");
    QPixmap p = QPixmap::fromImage(QImage(":/img/icons8-warning-shield-48.png"));
    ui->label_2->setPixmap(p);
}

DeleteWindow::~DeleteWindow()
{
    delete ui;
}

void DeleteWindow::on_buttonBox_accepted()
{
    emit(ok());
}

void DeleteWindow::on_buttonBox_rejected()
{
    emit(no());
}

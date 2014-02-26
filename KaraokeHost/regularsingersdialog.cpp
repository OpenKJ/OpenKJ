#include "regularsingersdialog.h"
#include "ui_regularsingersdialog.h"

RegularSingersDialog::RegularSingersDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegularSingersDialog)
{
    ui->setupUi(this);
    regularSingerModel = new RegularSingerModel(this);
    ui->treeViewRegulars->setModel(regularSingerModel);
    ui->treeViewRegulars->hideColumn(0);
    ui->treeViewRegulars->setColumnWidth(3,20);
    ui->treeViewRegulars->setColumnWidth(4,20);
    ui->treeViewRegulars->setColumnWidth(5,20);
    ui->treeViewRegulars->header()->setSectionResizeMode(2,QHeaderView::ResizeToContents);
    ui->treeViewRegulars->header()->setSectionResizeMode(1,QHeaderView::Stretch);
}

RegularSingersDialog::~RegularSingersDialog()
{
    delete ui;
}

void RegularSingersDialog::on_btnClose_clicked()
{
    close();
}

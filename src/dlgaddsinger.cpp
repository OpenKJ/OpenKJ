#include "dlgaddsinger.h"
#include "ui_dlgaddsinger.h"
#include "settings.h"
#include <QDebug>

#include <QMessageBox>

extern Settings settings;

DlgAddSinger::DlgAddSinger(TableModelRotation *rotModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgAddSinger)
{
    ui->setupUi(this);
    this->rotModel = rotModel;
    ui->cbxPosition->addItem(tr("Fair"));
    ui->cbxPosition->addItem(tr("Bottom"));
    ui->cbxPosition->addItem(tr("Next"));
    ui->cbxPosition->setCurrentIndex(settings.lastSingerAddPositionType());
    connect(&settings, &Settings::lastSingerAddPositionTypeChanged, ui->cbxPosition, &QComboBox::setCurrentIndex);
    connect(ui->cbxPosition, SIGNAL(currentIndexChanged(int)), &settings, SLOT(setLastSingerAddPositionType(int)));

}

DlgAddSinger::~DlgAddSinger()
{
    delete ui;
}

void DlgAddSinger::on_buttonBox_accepted()
{
    if (rotModel->singerExists(ui->lineEditName->text()))
    {
        QMessageBox msgBox;
        msgBox.setText(tr("A singer by that name already exists."));
        msgBox.exec();
    }
    else
    {
        if (ui->lineEditName->text() == "")
            return;
        int newSingerPos{0};
        int newSingerId = rotModel->singerAdd(ui->lineEditName->text(), ui->cbxPosition->currentIndex());
        newSingerPos = rotModel->getSingerPosition(newSingerId);
        emit newSingerAdded(newSingerPos);
        ui->lineEditName->clear();
        close();
    }
}

void DlgAddSinger::showEvent(QShowEvent *event)
{
    ui->lineEditName->setFocus();
    QDialog::showEvent(event);
}

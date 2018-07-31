#include "dlgaddsinger.h"
#include "ui_dlgaddsinger.h"
#include "settings.h"

#include <QMessageBox>

extern Settings *settings;

DlgAddSinger::DlgAddSinger(RotationModel *rotModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgAddSinger)
{
    ui->setupUi(this);
    this->rotModel = rotModel;
    ui->cbxPosition->addItem(tr("Fair"));
    ui->cbxPosition->addItem(tr("Bottom"));
    ui->cbxPosition->addItem(tr("Next"));
    ui->cbxPosition->setCurrentIndex(0);
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
        rotModel->singerAdd(ui->lineEditName->text());
        if (rotModel->currentSinger() != -1)
        {
            int curSingerPos = rotModel->getSingerPosition(rotModel->currentSinger());
            if (ui->cbxPosition->currentIndex() == 2)
                rotModel->singerMove(rotModel->rowCount() -1, curSingerPos + 1);
            else if ((ui->cbxPosition->currentIndex() == 0) && (curSingerPos != 0))
                rotModel->singerMove(rotModel->rowCount() -1, curSingerPos);
        }
        ui->lineEditName->clear();
        close();
    }
}

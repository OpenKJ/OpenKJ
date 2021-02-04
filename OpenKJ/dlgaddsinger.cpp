#include "dlgaddsinger.h"
#include "ui_dlgaddsinger.h"
#include "settings.h"
#include <QDebug>

#include <QMessageBox>

extern Settings settings;

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
        int newSingerPos{0};
        rotModel->singerAdd(ui->lineEditName->text());
        if (rotModel->currentSinger() != -1)
        {
            int curSingerPos = rotModel->getSingerPosition(rotModel->currentSinger());
            if (curSingerPos < 0)
                curSingerPos = 0;
            if (rotModel->singers().size() == 1) {
                qInfo() << "Skipping singer move, the new singer is the only singer";
                newSingerPos = 0;
            }
            else if (ui->cbxPosition->currentIndex() == 2)
            {
                newSingerPos = curSingerPos + 1;
                rotModel->singerMove(rotModel->rowCount() -1, newSingerPos);
            }
            else if ((ui->cbxPosition->currentIndex() == 0) && (curSingerPos != 0))
            {
                newSingerPos = curSingerPos;
                rotModel->singerMove(rotModel->rowCount() -1, newSingerPos);
            }
            else
            {
                newSingerPos = rotModel->rowCount() - 1;
            }
        }
        else
        {
            newSingerPos = rotModel->rowCount() - 1;
        }
        qInfo() << "New singer added, selecting postion: " << newSingerPos;
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

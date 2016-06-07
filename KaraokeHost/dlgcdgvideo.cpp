#include "dlgcdgvideo.h"
#include "ui_dlgcdgvideo.h"

DlgCdgVideo::DlgCdgVideo(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgCdgVideo)
{
    ui->setupUi(this);
  //  ui->videoWidget->videoSurface()->present(QImage(QString("/home/ipa/lightburnisaac/Pictures/juiceguyslogo.jpg")));
    ui->videoWidget->setEnabled(true);
    ui->videoWidget->setUpdatesEnabled(true);
    ui->videoWidget->setVisible(true);
}

DlgCdgVideo::~DlgCdgVideo()
{
    delete ui;
}

void DlgCdgVideo::present(QVideoFrame frame)
{
    ui->videoWidget->videoSurface()->present(frame);
}

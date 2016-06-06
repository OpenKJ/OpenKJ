#include "dlgcdgvideo.h"
#include "ui_dlgcdgvideo.h"

DlgCdgVideo::DlgCdgVideo(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgCdgVideo)
{
    ui->setupUi(this);
    ui->videoWidget->videoSurface()->present(QImage(QString("/home/idm/lightburnisa/Pictures/diabeetus.jpg")));
}

DlgCdgVideo::~DlgCdgVideo()
{
    delete ui;
}

#include "dlgcdgvideo.h"
#include "ui_dlgcdgvideo.h"

DlgCdgVideo::DlgCdgVideo(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgCdgVideo)
{
    ui->setupUi(this);
}

DlgCdgVideo::~DlgCdgVideo()
{
    delete ui;
}

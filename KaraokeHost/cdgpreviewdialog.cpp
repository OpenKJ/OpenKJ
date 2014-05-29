#include "cdgpreviewdialog.h"
#include "ui_cdgpreviewdialog.h"
#include <QMessageBox>
#include <QDebug>
#include "khzip.h"

CdgPreviewDialog::CdgPreviewDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CdgPreviewDialog)
{
    ui->setupUi(this);
    cdg = new CDG();
    timer = new QTimer(this);
    timer->stop();
    connect(timer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
    cdgPosition = 0;
    cdgTempDir = NULL;

}

CdgPreviewDialog::~CdgPreviewDialog()
{
    delete ui;
}

void CdgPreviewDialog::setZipFile(QString zipFile)
{
    m_zipFile = zipFile;
}

void CdgPreviewDialog::preview()
{
    timer->stop();
    cdgPosition = 0;
    if (cdg->IsOpen()) cdg->VideoClose();
    setVisible(true);
    cdgTempDir = new QTemporaryDir();
    KhZip zip(m_zipFile);
    qDebug() << "Extracting " << m_zipFile;
    if (!zip.extractCdg(QDir(cdgTempDir->path())))
    {
        QMessageBox::warning(this, tr("Bad karaoke file"),tr("Zip file does not contain a valid karaoke track.  CDG file missing."),QMessageBox::Ok);
        close();
        return;
    }
    QFile cdgFile(cdgTempDir->path() + QDir::separator() + "tmp.cdg");
    if (cdgFile.size() == 0)
    {
        QMessageBox::warning(this, tr("Bad karaoke file"), tr("CDG file contains no data"),QMessageBox::Ok);
        close();
        return;
    }
    qDebug() << "Opening cdg file " << cdgTempDir->path() << "/tmp.cdg";
    cdg->FileOpen(cdgTempDir->path().toStdString() + QDir::separator().toLatin1() + "tmp.cdg");
    cdg->Process();
    timer->start(40);
    delete cdgTempDir;
}


void CdgPreviewDialog::timerTimeout()
{
    if (cdg->IsOpen())
    {
        if (cdg->GetLastCDGUpdate() >= cdgPosition)
        {
            if (!cdg->SkipFrame(cdgPosition))
            {
                unsigned char* rgbdata;
                rgbdata = cdg->GetImageByTime(cdgPosition);
                QImage img(rgbdata, 300, 216, QImage::Format_RGB888);
                ui->cdgOutput->setPixmap(QPixmap::fromImage(img));
                free(rgbdata);
            }
            cdgPosition = cdgPosition + timer->interval();
        }
        else
        {
            timer->stop();
            cdg->VideoClose();
            cdgPosition = 0;
            close();
        }
    }
}



void CdgPreviewDialog::on_pushButtonClose_clicked()
{
    timer->stop();
    cdg->VideoClose();
    cdgPosition = 0;
    close();
}

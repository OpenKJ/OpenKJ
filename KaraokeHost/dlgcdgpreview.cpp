#include "dlgcdgpreview.h"
#include "ui_dlgcdgpreview.h"
#include <QMessageBox>
#include <QDebug>
#include "khzip.h"

DlgCdgPreview::DlgCdgPreview(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgCdgPreview)
{
    ui->setupUi(this);
    cdg = new CDG();
    timer = new QTimer(this);
    timer->stop();
    connect(timer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
    cdgPosition = 0;
    cdgTempDir = NULL;

}

DlgCdgPreview::~DlgCdgPreview()
{
    delete ui;
    delete cdg;
}

void DlgCdgPreview::setSourceFile(QString srcFile)
{
    m_srcFile = srcFile;
}

void DlgCdgPreview::preview()
{
    timer->stop();
    cdgPosition = 0;
    if (cdg->IsOpen()) cdg->VideoClose();
    setVisible(true);
    if (m_srcFile.endsWith(".zip", Qt::CaseInsensitive))
    {
    cdgTempDir = new QTemporaryDir();
    KhZip zip(m_srcFile);
    qDebug() << "Extracting " << m_srcFile;
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
    }
    else if (m_srcFile.endsWith(".cdg", Qt::CaseInsensitive))
    {
        QFile cdgFile(m_srcFile);
        if (cdgFile.size() == 0)
        {
            QMessageBox::warning(this, tr("Bad karaoke file"), tr("CDG file contains no data"),QMessageBox::Ok);
            close();
            return;
        }
        cdg->FileOpen(m_srcFile.toStdString());
    }
    cdg->Process();
    timer->start(40);
    delete cdgTempDir;
}


void DlgCdgPreview::timerTimeout()
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



void DlgCdgPreview::on_pushButtonClose_clicked()
{
    timer->stop();
    cdg->VideoClose();
    cdgPosition = 0;
    close();
}

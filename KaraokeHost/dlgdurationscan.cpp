#include "dlgdurationscan.h"
#include "ui_dlgdurationscan.h"
#include <QDebug>
#include <QSqlQuery>
#include <QApplication>


DlgDurationScan::DlgDurationScan(KhSongs *songs, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgDurationScan)
{
    m_songs = songs;
    m_needUpdate = new KhSongs;
    ui->setupUi(this);
    stopProcessing = false;
}

DlgDurationScan::~DlgDurationScan()
{
    delete m_needUpdate;
    delete ui;
}

void DlgDurationScan::on_buttonClose_clicked()
{
    stopProcessing = true;
    close();
}

void DlgDurationScan::on_buttonStop_clicked()
{
    stopProcessing = true;
}

void DlgDurationScan::on_buttonStart_clicked()
{
    stopProcessing = false;
    findNeedUpdateSongs();
    qDebug() << "Found " << m_needUpdate->count() << " songs that need duration";
    ui->lblTotal->setText(QString::number(m_needUpdate->count()));
    db.beginTransaction();
    for (int i=0; i < m_needUpdate->count(); i++)
    {
        QApplication::processEvents();
        zip.setZipFile(m_needUpdate->at(i)->path);
        int duration = zip.getSongDuration();
        QApplication::processEvents();
        db.songSetDuration(m_needUpdate->at(i)->ID, duration);
        m_needUpdate->at(i)->Duration = duration;
        ui->progressBar->setValue(((float)(i + 1) / (float)m_needUpdate->count()) * 100);
        ui->lblProcessed->setText(QString::number(i));
        QApplication::processEvents();
        if (stopProcessing)
        {
            stopProcessing = false;
            break;
        }
    }
    db.endTransaction();

}

void DlgDurationScan::findNeedUpdateSongs()
{
    m_needUpdate->clear();
    for (int i=0; i < m_songs->count(); i++)
    {
        if (m_songs->at(i)->Duration == 0)
        {
            m_needUpdate->push_back(m_songs->at(i));
        }
    }
}

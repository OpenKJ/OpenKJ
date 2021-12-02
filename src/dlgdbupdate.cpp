#include "dlgdbupdate.h"
#include "ui_dlgdbupdate.h"
#include <qscrollbar.h>

DlgDbUpdate::DlgDbUpdate(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgDbUpdate)
{
    ui->setupUi(this);
    reset();
    m_logFlushTimer.start(200, this);
}

DlgDbUpdate::~DlgDbUpdate()
{
    delete ui;
}

void DlgDbUpdate::addLogMsg(const QString& msg)
{
    m_log += msg;
    m_log += "\n";
}

void DlgDbUpdate::changeStatusTxt(QString txt)
{
    ui->lblCurrentActivity->setText(txt);
}

void DlgDbUpdate::changeProgress(int progress, int max)
{
    ui->progressBar->setMaximum(std::max(max, progress));
    ui->progressBar->setValue(progress);
}

void DlgDbUpdate::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_logFlushTimer.timerId()) {
        if (!m_log.isEmpty()) {
            ui->txtLog->appendPlainText(m_log);
            m_log.clear();
        }
    } else {
        QWidget::timerEvent(event);
    }
}

void DlgDbUpdate::reset()
{
    m_log.clear();
    ui->progressBar->setValue(0);
    ui->txtLog->clear();
    ui->lblCurrentActivity->setText("");

}

#include "dlgdebugoutput.h"
#include "ui_dlgdebugoutput.h"
#include "settings.h"

extern Settings *settings;

DlgDebugOutput::DlgDebugOutput(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgDebugOutput)
{
    ui->setupUi(this);
    timer = new QTimer(this);
    timer->start(250);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
    connect(ui->btnClear, SIGNAL(clicked()), ui->textEditLog, SLOT(clear()));
}

DlgDebugOutput::~DlgDebugOutput()
{
    delete ui;
}

void DlgDebugOutput::timerTimeout()
{
    if (!isVisible())
        return;
    while (!logContents->empty())
    {
        ui->textEditLog->append(logContents->takeFirst());
    }

}


void DlgDebugOutput::closeEvent(QCloseEvent *event)
{
    settings->setLogVisible(false);
    QDialog::closeEvent(event);
}

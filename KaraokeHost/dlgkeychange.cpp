#include "dlgkeychange.h"
#include "ui_dlgkeychange.h"

DlgKeyChange::DlgKeyChange(RotationTableModel *rotationModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgKeyChange)
{
    m_rotationModel = rotationModel;
    m_activeSong = NULL;
    ui->setupUi(this);
}

void DlgKeyChange::setActiveSong(KhQueueSong *song)
{
    m_activeSong = song;
    ui->spinBoxKey->setValue(song->getKeyChange());
}

DlgKeyChange::~DlgKeyChange()
{
    delete ui;
}

void DlgKeyChange::on_buttonBox_accepted()
{
    m_activeSong->setKeyChange(ui->spinBoxKey->value());
    //m_queueSongs->queueUpdated();
    close();
}

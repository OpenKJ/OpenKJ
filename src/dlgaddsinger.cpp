#include "dlgaddsinger.h"
#include "ui_dlgaddsinger.h"
#include <QDebug>
#include <QMessageBox>


DlgAddSinger::DlgAddSinger(TableModelRotation &rotationModel, QWidget *parent) :
        QDialog(parent),
        m_rotModel(rotationModel),
        ui(new Ui::DlgAddSinger) {
    ui->setupUi(this);
    ui->cbxPosition->addItems({"Fair", "Bottom", "Next"});
    ui->cbxPosition->setCurrentIndex(m_settings.lastSingerAddPositionType());
    connect(ui->cbxPosition, qOverload<int>(&QComboBox::currentIndexChanged), &m_settings, &Settings::setLastSingerAddPositionType);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DlgAddSinger::addSinger);

}

DlgAddSinger::~DlgAddSinger() = default;

void DlgAddSinger::addSinger() {
    if (m_rotModel.singerExists(ui->lineEditName->text())) {
        QMessageBox::warning(this, "Unable to add singer", "A singer with the same name already exists.");
        return;
    }
    if (ui->lineEditName->text() == "") {
        QMessageBox::warning(this, "Missing required field", "You must enter a singer name.");
        return;
    }
    int newSingerId = m_rotModel.singerAdd(ui->lineEditName->text(), ui->cbxPosition->currentIndex());
    emit newSingerAdded(m_rotModel.getSinger(newSingerId).position);
    ui->lineEditName->clear();
    close();
}

void DlgAddSinger::showEvent(QShowEvent *event) {
    ui->lineEditName->setFocus();
    QDialog::showEvent(event);
}

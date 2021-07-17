#include "dlgaddsinger.h"
#include "ui_dlgaddsinger.h"
#include "settings.h"
#include <QDebug>
#include <QMessageBox>

extern Settings settings;

DlgAddSinger::DlgAddSinger(TableModelRotation &rotationModel, QWidget *parent) :
        QDialog(parent),
        m_rotModel(rotationModel),
        ui(new Ui::DlgAddSinger) {
    ui->setupUi(this);
    ui->cbxPosition->addItem(tr("Fair"));
    ui->cbxPosition->addItem(tr("Bottom"));
    ui->cbxPosition->addItem(tr("Next"));
    ui->cbxPosition->setCurrentIndex(settings.lastSingerAddPositionType());
    connect(&settings, &Settings::lastSingerAddPositionTypeChanged, ui->cbxPosition, &QComboBox::setCurrentIndex);
    connect(ui->cbxPosition, SIGNAL(currentIndexChanged(int)), &settings, SLOT(setLastSingerAddPositionType(int)));
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
    emit newSingerAdded(m_rotModel.getSingerPosition(newSingerId));
    ui->lineEditName->clear();
    close();
}

void DlgAddSinger::showEvent(QShowEvent *event) {
    ui->lineEditName->setFocus();
    QDialog::showEvent(event);
}

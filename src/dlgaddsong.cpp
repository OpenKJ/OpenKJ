#include "dlgaddsong.h"
#include "ui_dlgaddsong.h"
#include <QDebug>
#include <QMessageBox>


DlgAddSong::DlgAddSong(TableModelRotation &rotationModel, TableModelQueueSongs &queueModel, int songId, QWidget *parent) :
    QDialog(parent), ui(new Ui::DlgAddSong), m_rotModel(rotationModel), m_queueModel(queueModel), m_songId(songId)
{
    ui->setupUi(this);
    m_rButtons.addButton(ui->radioButtonAddSinger);
    m_rButtons.setId(ui->radioButtonAddSinger, 0);
    m_rButtons.addButton(ui->radioButtonAddToExistingSinger);
    m_rButtons.setId(ui->radioButtonAddToExistingSinger, 1);
    m_rButtons.addButton(ui->radioButtonLoadRegAndAdd);
    m_rButtons.setId(ui->radioButtonLoadRegAndAdd, 2);
    m_rButtons.setExclusive(true);
    ui->radioButtonAddSinger->setChecked(true);
    ui->lineEditNewSingerName->setFocus();

    ui->comboBoxPosition->addItem(tr("Fair"));
    ui->comboBoxPosition->addItem(tr("Bottom"));
    ui->comboBoxPosition->addItem(tr("Next"));
    ui->comboBoxPosition->setCurrentIndex(m_settings.lastSingerAddPositionType());

    auto rotSingers = m_rotModel.singers();
    rotSingers.sort(Qt::CaseInsensitive);
    ui->comboBoxRotSingers->addItems(rotSingers);

    auto regSingers = m_rotModel.historySingers();
    regSingers.sort(Qt::CaseInsensitive);
    ui->comboBoxRegSingers->addItems(regSingers);

    connect(ui->pushButtonKeyDown, &QPushButton::clicked, ui->spinBoxKeyChange, &QSpinBox::stepDown);
    connect(ui->pushButtonKeyUp, &QPushButton::clicked, ui->spinBoxKeyChange, &QSpinBox::stepUp);
    connect(ui->comboBoxPosition, qOverload<int>(&QComboBox::currentIndexChanged), &m_settings, &Settings::setLastSingerAddPositionType);
    connect(ui->pushButtonAdd, &QPushButton::clicked, this, &DlgAddSong::pushButtonAddClicked);
    connect(ui->pushButtonCancel, &QPushButton::clicked, this, &DlgAddSong::pushButtonCancelClicked);
    connect(ui->spinBoxKeyChange, qOverload<int>(&QSpinBox::valueChanged), this, &DlgAddSong::spinBoxKeyChangeChanged);
    connect(ui->comboBoxRotSingers, qOverload<int>(&QComboBox::activated), this, &DlgAddSong::comboBoxRotSingersActivated);
    connect(ui->lineEditNewSingerName, &QLineEdit::returnPressed, this, &DlgAddSong::pushButtonAddClicked);
    connect(ui->lineEditNewSingerName, &QLineEdit::textChanged, this, &DlgAddSong::lineEditNewSingerNameTextChanged);
    connect(ui->comboBoxRegSingers, qOverload<int>(&QComboBox::activated), this, &DlgAddSong::comboBoxRegSingersActivated);

}

DlgAddSong::~DlgAddSong() = default;


void DlgAddSong::closeEvent([[maybe_unused]] QCloseEvent *event)
{
    this->deleteLater();
}

void DlgAddSong::pushButtonCancelClicked()
{
    close();
}

void DlgAddSong::spinBoxKeyChangeChanged(int arg1)
{
    if (arg1 > 0)
        ui->spinBoxKeyChange->setPrefix("+");
    else
        ui->spinBoxKeyChange->setPrefix("");
}

void DlgAddSong::pushButtonAddClicked()
{
    switch (m_rButtons.checkedId()) {
    case 0:
        if (ui->lineEditNewSingerName->text() == QString())
        {
            QMessageBox::warning(this, "No singer name specified", "You must enter a new singer name to add the song to.");
        }
        else if (m_rotModel.singerExists(ui->lineEditNewSingerName->text()))
        {
            QMessageBox msgBox;
            msgBox.setText("A singer with that name already exists");
            msgBox.setInformativeText("Would you like to add the song to the existing singer instead?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            if (msgBox.exec() == QMessageBox::Yes)
            {
                int singerId = m_rotModel.getSingerId(ui->lineEditNewSingerName->text());
                m_queueModel.songAddSlot(m_songId, singerId, ui->spinBoxKeyChange->value());
                emit newSingerAdded(m_rotModel.getSingerPosition(singerId));
                close();
            }
        }
        else
        {
            int singerId = m_rotModel.singerAdd(ui->lineEditNewSingerName->text(), ui->comboBoxPosition->currentIndex());
            m_queueModel.songAddSlot(m_songId, singerId, ui->spinBoxKeyChange->value());
            emit newSingerAdded(m_rotModel.getSingerPosition(singerId));
            close();
        }
        break;
    case 1:
    {
        int singerId = m_rotModel.getSingerId(ui->comboBoxRotSingers->currentText());
        m_queueModel.songAddSlot(m_songId, singerId, ui->spinBoxKeyChange->value());
        emit newSingerAdded(m_rotModel.getSingerPosition(singerId));
        close();
        break;
    }
    case 2:
        if (m_rotModel.singerExists(ui->comboBoxRegSingers->currentText()))
        {
            int existingId = m_rotModel.getSingerId(ui->comboBoxRegSingers->currentText());
            if (m_rotModel.singerIsRegular(existingId))
            {
                QMessageBox msgBox;
                msgBox.setText("Regular singer already loaded");
                msgBox.setInformativeText("That regular singer is already loaded in the rotation.\n"
                                          "Would you like to add the song to their queue instead?");
                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                msgBox.setDefaultButton(QMessageBox::Yes);
                if (msgBox.exec() == QMessageBox::Yes)
                {
                    m_queueModel.songAddSlot(m_songId, existingId, ui->spinBoxKeyChange->value());
                    emit newSingerAdded(m_rotModel.getSingerPosition(existingId));
                    close();
                }
            }
            else
            {
                auto answer = QMessageBox::question(this,
                                                    "A singer by this name already exists in the rotation",
                                                    "A singer with this name is currently in the rotation, but they aren't marked as a regular.\n"
                                                    "Would you like to add the song to their queue and load this regular's song history for "
                                                    "the matching rotation singer?",
                                                    QMessageBox::StandardButtons(QMessageBox::Yes|QMessageBox::Cancel),
                                                    QMessageBox::Yes
                                                    );
                if (answer == QMessageBox::Yes)
                {
                    int singerId = m_rotModel.getSingerId(ui->comboBoxRegSingers->currentText());
                    m_rotModel.singerMakeRegular(singerId);
                    m_queueModel.songAddSlot(m_songId, singerId, ui->spinBoxKeyChange->value());
                    emit newSingerAdded(m_rotModel.getSingerPosition(singerId));
                }
            }
        }
        else
        {
            int singerId = m_rotModel.singerAdd(ui->comboBoxRegSingers->currentText(), ui->comboBoxPosition->currentIndex());
            m_rotModel.singerMakeRegular(singerId);
            m_queueModel.songAddSlot(m_songId, singerId, ui->spinBoxKeyChange->value());
            emit newSingerAdded(m_rotModel.getSingerPosition(singerId));
            close();
        }
        break;
    }
}

void DlgAddSong::comboBoxRotSingersActivated([[maybe_unused]] int index)
{
    ui->radioButtonAddToExistingSinger->setChecked(true);
}

void DlgAddSong::lineEditNewSingerNameTextChanged([[maybe_unused]] const QString &arg1)
{
    ui->radioButtonAddSinger->setChecked(true);
}

void DlgAddSong::comboBoxRegSingersActivated([[maybe_unused]]int index)
{
    ui->radioButtonLoadRegAndAdd->setChecked(true);
}

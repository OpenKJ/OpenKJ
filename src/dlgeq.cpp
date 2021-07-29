#include "dlgeq.h"
#include "ui_dlgeq.h"


DlgEq::DlgEq(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgEq)
{
    ui->setupUi(this);
    connect(ui->checkBoxEqBypassK, &QCheckBox::toggled, [&] (auto enabled) {
        m_settings.setEqKBypass(enabled);
        emit karEqBypassChanged(enabled);
    });
    connect(ui->checkBoxEqBypassB, &QCheckBox::toggled, [&](auto enabled) {
        m_settings.setEqBBypass(enabled);
        emit bmEqBypassChanged(enabled);
    });
    ui->checkBoxEqBypassK->setChecked(m_settings.eqKBypass());
    ui->checkBoxEqBypassB->setChecked(m_settings.eqBBypass());
    auto eqSliderControlsK = {
        ui->verticalSliderEqK1,
        ui->verticalSliderEqK2,
        ui->verticalSliderEqK3,
        ui->verticalSliderEqK4,
        ui->verticalSliderEqK5,
        ui->verticalSliderEqK6,
        ui->verticalSliderEqK7,
        ui->verticalSliderEqK8,
        ui->verticalSliderEqK9,
        ui->verticalSliderEqK10
    };
    auto eqSliderControlsB = {
        ui->verticalSliderEqB1,
        ui->verticalSliderEqB2,
        ui->verticalSliderEqB3,
        ui->verticalSliderEqB4,
        ui->verticalSliderEqB5,
        ui->verticalSliderEqB6,
        ui->verticalSliderEqB7,
        ui->verticalSliderEqB8,
        ui->verticalSliderEqB9,
        ui->verticalSliderEqB10
    };
    int band{0};
    for (auto &slider : eqSliderControlsK)
    {
        slider->setValue(m_settings.getEqKLevel(band));
        connect(slider, &QSlider::valueChanged, [=]( int newValue ) {
            m_settings.setEqKLevel(band, newValue);
            emit karEqLevelChanged(band, newValue);
        });
        band++;
    }
    band = 0;
    for (auto &slider : eqSliderControlsB)
    {
        slider->setValue(m_settings.getEqBLevel(band));
        connect(slider, &QSlider::valueChanged, [=]( int newValue ) {
            m_settings.setEqBLevel(band, newValue);
            emit bmEqLevelChanged(band, newValue);
        });
        band++;
    }
}

DlgEq::~DlgEq()
{
    delete ui;
}

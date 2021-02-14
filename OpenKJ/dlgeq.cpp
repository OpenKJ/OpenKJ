#include "dlgeq.h"
#include "ui_dlgeq.h"
#include "settings.h"

extern Settings settings;

DlgEq::DlgEq(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgEq)
{
    ui->setupUi(this);

    connect(ui->checkBoxEqBypassK, SIGNAL(toggled(bool)), &settings, SLOT(setEqKBypass(bool)));
    connect(ui->checkBoxEqBypassB, SIGNAL(toggled(bool)), &settings, SLOT(setEqBBypass(bool)));

    ui->checkBoxEqBypassK->setChecked(settings.eqKBypass());
    ui->checkBoxEqBypassB->setChecked(settings.eqBBypass());

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


    int band = 0;

    for (auto &slider : eqSliderControlsK)
    {
        connect(slider, &QSlider::valueChanged, [=]( int newValue ) { settings.setEqKLevel(band, newValue); });
        slider->setValue(settings.getEqKLevel(band));
        band++;
    }

    band = 0;
    for (auto &slider : eqSliderControlsB)
    {
        connect(slider, &QSlider::valueChanged, [=]( int newValue ) { settings.setEqBLevel(band, newValue); });
        slider->setValue(settings.getEqBLevel(band));
        band++;
    }
}

DlgEq::~DlgEq()
{
    delete ui;
}

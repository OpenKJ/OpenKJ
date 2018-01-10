#include "dlgeq.h"
#include "ui_dlgeq.h"
#include "settings.h"

extern Settings *settings;

DlgEq::DlgEq(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgEq)
{
    ui->setupUi(this);

    connect(ui->checkBoxEqBypassB, SIGNAL(toggled(bool)), settings, SLOT(setEqBBypass(bool)));
    connect(ui->checkBoxEqBypassK, SIGNAL(toggled(bool)), settings, SLOT(setEqKBypass(bool)));
    connect(ui->verticalSliderEqB1, SIGNAL(valueChanged(int)), settings, SLOT(setEqBLevel1(int)));
    connect(ui->verticalSliderEqB2, SIGNAL(valueChanged(int)), settings, SLOT(setEqBLevel2(int)));
    connect(ui->verticalSliderEqB3, SIGNAL(valueChanged(int)), settings, SLOT(setEqBLevel3(int)));
    connect(ui->verticalSliderEqB4, SIGNAL(valueChanged(int)), settings, SLOT(setEqBLevel4(int)));
    connect(ui->verticalSliderEqB5, SIGNAL(valueChanged(int)), settings, SLOT(setEqBLevel5(int)));
    connect(ui->verticalSliderEqB6, SIGNAL(valueChanged(int)), settings, SLOT(setEqBLevel6(int)));
    connect(ui->verticalSliderEqB7, SIGNAL(valueChanged(int)), settings, SLOT(setEqBLevel7(int)));
    connect(ui->verticalSliderEqB8, SIGNAL(valueChanged(int)), settings, SLOT(setEqBLevel8(int)));
    connect(ui->verticalSliderEqB9, SIGNAL(valueChanged(int)), settings, SLOT(setEqBLevel9(int)));
    connect(ui->verticalSliderEqB10, SIGNAL(valueChanged(int)), settings, SLOT(setEqBLevel10(int)));
    connect(ui->verticalSliderEqK1, SIGNAL(valueChanged(int)), settings, SLOT(setEqKLevel1(int)));
    connect(ui->verticalSliderEqK2, SIGNAL(valueChanged(int)), settings, SLOT(setEqKLevel2(int)));
    connect(ui->verticalSliderEqK3, SIGNAL(valueChanged(int)), settings, SLOT(setEqKLevel3(int)));
    connect(ui->verticalSliderEqK4, SIGNAL(valueChanged(int)), settings, SLOT(setEqKLevel4(int)));
    connect(ui->verticalSliderEqK5, SIGNAL(valueChanged(int)), settings, SLOT(setEqKLevel5(int)));
    connect(ui->verticalSliderEqK6, SIGNAL(valueChanged(int)), settings, SLOT(setEqKLevel6(int)));
    connect(ui->verticalSliderEqK7, SIGNAL(valueChanged(int)), settings, SLOT(setEqKLevel7(int)));
    connect(ui->verticalSliderEqK8, SIGNAL(valueChanged(int)), settings, SLOT(setEqKLevel8(int)));
    connect(ui->verticalSliderEqK9, SIGNAL(valueChanged(int)), settings, SLOT(setEqKLevel9(int)));
    connect(ui->verticalSliderEqK10, SIGNAL(valueChanged(int)), settings, SLOT(setEqKLevel10(int)));
    ui->checkBoxEqBypassB->setChecked(settings->eqBBypass());
    ui->checkBoxEqBypassK->setChecked(settings->eqKBypass());
    ui->verticalSliderEqB1->setValue(settings->eqBLevel1());
    ui->verticalSliderEqB2->setValue(settings->eqBLevel2());
    ui->verticalSliderEqB3->setValue(settings->eqBLevel3());
    ui->verticalSliderEqB4->setValue(settings->eqBLevel4());
    ui->verticalSliderEqB5->setValue(settings->eqBLevel5());
    ui->verticalSliderEqB6->setValue(settings->eqBLevel6());
    ui->verticalSliderEqB7->setValue(settings->eqBLevel7());
    ui->verticalSliderEqB8->setValue(settings->eqBLevel8());
    ui->verticalSliderEqB9->setValue(settings->eqBLevel9());
    ui->verticalSliderEqB10->setValue(settings->eqBLevel10());
    ui->verticalSliderEqK1->setValue(settings->eqKLevel1());
    ui->verticalSliderEqK2->setValue(settings->eqKLevel2());
    ui->verticalSliderEqK3->setValue(settings->eqKLevel3());
    ui->verticalSliderEqK4->setValue(settings->eqKLevel4());
    ui->verticalSliderEqK5->setValue(settings->eqKLevel5());
    ui->verticalSliderEqK6->setValue(settings->eqKLevel6());
    ui->verticalSliderEqK7->setValue(settings->eqKLevel7());
    ui->verticalSliderEqK8->setValue(settings->eqKLevel8());
    ui->verticalSliderEqK9->setValue(settings->eqKLevel9());
    ui->verticalSliderEqK10->setValue(settings->eqKLevel10());
}

DlgEq::~DlgEq()
{
    delete ui;
}

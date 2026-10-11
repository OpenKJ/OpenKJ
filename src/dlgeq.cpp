#include "dlgeq.h"
#include "ui_dlgeq.h"
#include <QTabWidget>
#include <QGridLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>

namespace {
constexpr int DefaultEqGain = 0;
}


DlgEq::DlgEq(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DlgEq)
{
    ui->setupUi(this);
    auto *tabs = new QTabWidget(this);
    tabs->setObjectName("equalizerTabs");
    ui->horizontalLayout_24->removeWidget(ui->groupBox_4);
    ui->horizontalLayout_24->removeWidget(ui->groupBox_5);
    ui->groupBox_4->setTitle(QString());
    ui->groupBox_5->setTitle(QString());
    tabs->addTab(ui->groupBox_4, tr("Karaoke"));
    tabs->addTab(ui->groupBox_5, tr("Break Music"));
    ui->verticalLayout->removeItem(ui->horizontalLayout_24);
    while (auto *item = ui->horizontalLayout_24->takeAt(0))
        delete item;
    delete ui->horizontalLayout_24;
    ui->verticalLayout->insertWidget(0, tabs);
    ui->verticalLayout->setContentsMargins(0, 0, 0, 0);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
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
    const QList<QSlider *> eqSliderControlsK = {
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
    const QList<QSlider *> eqSliderControlsB = {
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
    // Labels match the fixed center frequencies of GStreamer's equalizer-10bands.
    const QStringList frequencies = {"29", "59", "119", "237", "474", "947",
                                     "1.9k", "3.8k", "7.5k", "15k"};
    auto labelBands = [this, &frequencies](auto sliders, QHBoxLayout *row, QVBoxLayout *container) {
        auto *grid = new QGridLayout;
        grid->setHorizontalSpacing(2);
        grid->setVerticalSpacing(2);
        int column = 0;
        for (auto *slider : sliders) {
            row->removeWidget(slider);
            slider->installEventFilter(this);
            slider->setMinimumHeight(80);
            slider->setMaximumHeight(110);
            grid->addWidget(slider, 0, column, Qt::AlignHCenter);
            auto *label = new QLabel(frequencies.at(column));
            m_frequencyLabels.append(label);
            label->setAlignment(Qt::AlignCenter);
            label->setToolTip(QObject::tr("Center frequency in Hz"));
            grid->addWidget(label, 1, column);
            auto updateTooltip = [slider, frequency = frequencies.at(column)](int gain) {
                slider->setToolTip(QObject::tr("%1 Hz: %2 dB\nDouble-click to reset to 0 dB").arg(frequency).arg(gain));
            };
            updateTooltip(slider->value());
            QObject::connect(slider, &QSlider::valueChanged, slider, updateTooltip);
            ++column;
        }
        container->removeItem(row);
        delete row;
        container->insertLayout(0, grid);
    };
    labelBands(eqSliderControlsK, ui->horizontalLayout_19, ui->verticalLayout_11);
    labelBands(eqSliderControlsB, ui->horizontalLayout_23, ui->verticalLayout_23);
    auto addResetButton = [this](const QList<QSlider *> &sliders, QCheckBox *bypass,
                                QVBoxLayout *container, const QString &name) {
        container->removeWidget(bypass);
        auto *controls = new QHBoxLayout;
        controls->addWidget(bypass);
        controls->addStretch();
        auto *reset = new QPushButton(tr("Reset"));
        reset->setObjectName(name);
        reset->setToolTip(tr("Reset all ten bands in this section to 0 dB"));
        controls->addWidget(reset);
        container->addLayout(controls);
        connect(reset, &QPushButton::clicked, this, [sliders]() {
            for (auto *slider : sliders)
                slider->setValue(DefaultEqGain);
        });
    };
    addResetButton(eqSliderControlsK, ui->checkBoxEqBypassK, ui->verticalLayout_11,
                   QStringLiteral("buttonResetKaraokeEq"));
    addResetButton(eqSliderControlsB, ui->checkBoxEqBypassB, ui->verticalLayout_23,
                   QStringLiteral("buttonResetBreakMusicEq"));
    int band{0};
    for (auto &slider : eqSliderControlsK)
    {
        slider->setValue(m_settings.getEqKLevel(band));
        connect(slider, &QSlider::valueChanged,
            this, [this, band](int newValue) {
            m_settings.setEqKLevel(band, newValue);
            emit karEqLevelChanged(band, newValue);
        });
        band++;
    }
    band = 0;
    for (auto &slider : eqSliderControlsB)
    {
        slider->setValue(m_settings.getEqBLevel(band));
        connect(slider, &QSlider::valueChanged,
            this, [this, band](int newValue) {
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

bool DlgEq::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonDblClick) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            if (auto *slider = qobject_cast<QSlider *>(watched)) {
                // setValue follows the same settings and audio updates as dragging.
                slider->setValue(DefaultEqGain);
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void DlgEq::setFrequencyLabelsVisible(bool visible)
{
    for (auto *label : m_frequencyLabels)
        label->setVisible(visible);
}

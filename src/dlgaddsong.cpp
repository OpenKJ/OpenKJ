#include "dlgaddsong.h"
#include "ui_dlgaddsong.h"
#include <QMessageBox>
#include <QScreen>
#include <QScrollBar>
#include <QTimer>
#include <algorithm>

namespace {
constexpr int singersPerColumn = 10;
constexpr int dialogScreenWidthPercent = 45;
}

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
    ui->listWidgetRotSingers->addItems(rotSingers);
    for (int row = 0; row < ui->listWidgetRotSingers->count(); ++row) {
        auto *item = ui->listWidgetRotSingers->item(row);
        item->setToolTip(item->text());
    }
    ui->listWidgetRotSingers->setCurrentRow(rotSingers.isEmpty() ? -1 : 0);
    ui->radioButtonAddToExistingSinger->setEnabled(!rotSingers.isEmpty());
    ui->listWidgetRotSingers->setEnabled(!rotSingers.isEmpty());
    m_singerListReady = true;
    installEventFilter(this);
    ui->listWidgetRotSingers->installEventFilter(this);
    ui->listWidgetRotSingers->viewport()->installEventFilter(this);

    auto regSingers = m_rotModel.historySingers();
    regSingers.sort(Qt::CaseInsensitive);
    ui->comboBoxRegSingers->addItems(regSingers);

    connect(ui->pushButtonKeyDown, &QPushButton::clicked, ui->spinBoxKeyChange, &QSpinBox::stepDown);
    connect(ui->pushButtonKeyUp, &QPushButton::clicked, ui->spinBoxKeyChange, &QSpinBox::stepUp);
    connect(ui->comboBoxPosition, qOverload<int>(&QComboBox::currentIndexChanged), &m_settings, &Settings::setLastSingerAddPositionType);
    connect(ui->pushButtonAdd, &QPushButton::clicked, this, &DlgAddSong::pushButtonAddClicked);
    connect(ui->pushButtonCancel, &QPushButton::clicked, this, &DlgAddSong::pushButtonCancelClicked);
    connect(ui->spinBoxKeyChange, qOverload<int>(&QSpinBox::valueChanged), this, &DlgAddSong::spinBoxKeyChangeChanged);
    connect(ui->listWidgetRotSingers, &QListWidget::itemClicked, this, &DlgAddSong::rotationSingerSelected);
    connect(ui->listWidgetRotSingers, &QListWidget::currentRowChanged, this, &DlgAddSong::rotationSingerSelected);
    connect(ui->listWidgetRotSingers, &QListWidget::itemActivated, this, [this] {
        rotationSingerSelected();
        pushButtonAddClicked();
    });
    connect(ui->lineEditNewSingerName, &QLineEdit::returnPressed, this, &DlgAddSong::pushButtonAddClicked);
    connect(ui->lineEditNewSingerName, &QLineEdit::textChanged, this, &DlgAddSong::lineEditNewSingerNameTextChanged);
    connect(ui->comboBoxRegSingers, qOverload<int>(&QComboBox::activated), this, &DlgAddSong::comboBoxRegSingersActivated);

    // Size once on opening; subsequent resizes belong to the host.
    ensurePolished();
    const int columns = std::max(1, (ui->listWidgetRotSingers->count() + singersPerColumn - 1) / singersPerColumn);
    // Measure the longest name once as the minimum width for every column.
    // Extra space is shared evenly when laying out the visible columns.
    m_singerColumnWidth = 24;
    for (const auto &name : rotSingers)
        m_singerColumnWidth = std::max(m_singerColumnWidth,
            ui->listWidgetRotSingers->fontMetrics().horizontalAdvance(name) + 24);
    ui->listWidgetRotSingers->setMinimumWidth(0);
    updateSingerListLayout();
    ui->positionAndKeyLayout->setDirection(QBoxLayout::TopToBottom);
    ui->positionAndKeyLayout->setSpacing(24);
    ui->positionAndKeyLayout->setStretch(0, 1);
    ui->positionAndKeyLayout->setStretch(1, 1);
    layout()->activate();
    layout()->setGeometry(rect());
    // Count layout padding rather than unused space around the compact list.
    int margins = 2 * ui->listWidgetRotSingers->frameWidth();
    for (const auto *containerLayout : {static_cast<QLayout *>(ui->verticalLayout_2),
                                       static_cast<QLayout *>(ui->verticalLayout_8),
                                       static_cast<QLayout *>(ui->horizontalLayout_2)}) {
        const auto padding = containerLayout->contentsMargins();
        margins += padding.left() + padding.right();
    }
    const auto framePadding = ui->frame_2->contentsMargins();
    margins += framePadding.left() + framePadding.right();
    // Limit the dialog to just under half the usable screen, except when
    // its controls need more room (for example with accessibility fonts).
    const int minimumWidth = minimumSizeHint().width();
    const int widthBudget = std::max(minimumWidth,
        screen()->availableGeometry().width() * dialogScreenWidthPercent / 100);
    const int visibleColumns = std::clamp((widthBudget - margins) / m_singerColumnWidth, 1, columns);
    const int initialWidth = std::max(minimumWidth,
        std::min(widthBudget, visibleColumns * m_singerColumnWidth + margins));
    // Keep the stacked minimum width even when the wider layout is active,
    // so dragging the window narrower can trigger stacking again.
    setMinimumWidth(minimumWidth);
    setMaximumWidth(widthBudget);
    layout()->setSizeConstraint(QLayout::SetNoConstraint);
    resize(initialWidth, sizeHint().height());
    updateControlLayout();
    layout()->activate();
    resize(width(), sizeHint().height());
    updateSingerListLayout();
}

DlgAddSong::~DlgAddSong() = default;

void DlgAddSong::updateControlLayout()
{
    constexpr int horizontalGap = 16;
    const int sectionWidth = std::max(ui->horizontalLayout_5->minimumSize().width(),
                                      ui->horizontalLayoutKeyChange->minimumSize().width());
    const auto outerPadding = layout()->contentsMargins();
    const auto innerPadding = ui->positionAndKeyLayout->contentsMargins();
    const int availableWidth = width() - outerPadding.left() - outerPadding.right()
                               - innerPadding.left() - innerPadding.right();
    const bool sideBySide = availableWidth >= 2 * sectionWidth + horizontalGap;
    const auto direction = sideBySide ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom;
    if (ui->positionAndKeyLayout->direction() != direction) {
        ui->positionAndKeyLayout->setDirection(direction);
        ui->positionAndKeyLayout->setSpacing(sideBySide ? horizontalGap : 24);
        layout()->activate();
    }
    // Height follows the content, including responsive stacking and the
    // singer list's scrollbar. Only the width can be resized by the host.
    const int contentHeight = layout()->sizeHint().height();
    if (minimumHeight() != contentHeight || maximumHeight() != contentHeight)
        setFixedHeight(contentHeight);
}

void DlgAddSong::updateSingerListLayout()
{
    if (!m_singerListReady || m_singerColumnWidth == 0)
        return;
    auto *list = ui->listWidgetRotSingers;
    const int rowHeight = std::max(28, list->fontMetrics().height() + 10);
    const int visibleRows = std::clamp(list->count(), 1, singersPerColumn);
    const int columns = std::max(1, (list->count() + singersPerColumn - 1) / singersPerColumn);
    const int visibleColumns = std::clamp(list->viewport()->width() / m_singerColumnWidth, 1, columns);
    const int columnWidth = std::max(m_singerColumnWidth, list->viewport()->width() / visibleColumns);
    const bool needsScroll = columns * columnWidth > list->viewport()->width();
    list->setHorizontalScrollBarPolicy(needsScroll ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAlwaysOff);
    // Some native styles overlay the scrollbar without shrinking viewport
    // geometry, but QListView still reserves its height when wrapping.
    const int scrollHeight = needsScroll ? list->horizontalScrollBar()->sizeHint().height() : 0;
    const int chromeHeight = 2 * list->frameWidth() + scrollHeight;
    // QListView wraps at its inclusive viewport boundary; leave one pixel so
    // the tenth complete grid cell remains in the first column.
    list->setFixedHeight(visibleRows * rowHeight + chromeHeight + 1);
    // Give the items the full cell dimensions, not just the text's size hint,
    // so selection and hit targets fill each row consistently.
    for (int row = 0; row < list->count(); ++row) {
        const QSize cellSize(columnWidth, rowHeight);
        auto *item = list->item(row);
        if (item->sizeHint() != cellSize)
            item->setSizeHint(cellSize);
    }
}

bool DlgAddSong::eventFilter(QObject *watched, QEvent *event)
{
    if (m_singerListReady && !m_singerLayoutPending &&
        ((watched == this && event->type() == QEvent::Resize) ||
         (watched == ui->listWidgetRotSingers->viewport() && event->type() == QEvent::Resize) ||
         (watched == ui->listWidgetRotSingers &&
          (event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange)))) {
        m_singerLayoutPending = true;
        // The dialog's outer resize can precede its child layout. Use the
        // settled viewport dimensions without nested event processing.
        QTimer::singleShot(0, this, [this] {
            m_singerLayoutPending = false;
            updateSingerListLayout();
            updateControlLayout();
        });
    }
    return QDialog::eventFilter(watched, event);
}

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
                int singerId = m_rotModel.getSingerByName(ui->lineEditNewSingerName->text()).id;
                m_queueModel.songAddSlot(m_songId, singerId, ui->spinBoxKeyChange->value());
                emit newSingerAdded(m_rotModel.getSinger(singerId).position);
                close();
            }
        }
        else
        {
            m_rotModel.singerAddWithSong(ui->lineEditNewSingerName->text(), ui->comboBoxPosition->currentIndex(), m_songId, ui->spinBoxKeyChange->value(), false);
            close();
        }
        break;
    case 1:
    {
        const auto *item = ui->listWidgetRotSingers->currentItem();
        if (!item || !m_rotModel.singerExists(item->text())) {
            QMessageBox::warning(this, tr("Select a singer"), tr("Choose a singer who is still in the rotation."));
            return;
        }
        int singerId = m_rotModel.getSingerByName(item->text()).id;
        m_queueModel.songAddSlot(m_songId, singerId, ui->spinBoxKeyChange->value());
        emit newSingerAdded(m_rotModel.getSinger(singerId).position);
        close();
        break;
    }
    case 2:
        if (m_rotModel.singerExists(ui->comboBoxRegSingers->currentText()))
        {
            int existingId = m_rotModel.getSingerByName(ui->comboBoxRegSingers->currentText()).id;
            if (m_rotModel.getSinger(existingId).regular)
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
                    emit newSingerAdded(m_rotModel.getSinger(existingId).position);
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
                    int singerId = m_rotModel.getSingerByName(ui->comboBoxRegSingers->currentText()).id;
                    m_rotModel.singerMakeRegular(singerId);
                    m_queueModel.songAddSlot(m_songId, singerId, ui->spinBoxKeyChange->value());
                    emit newSingerAdded(m_rotModel.getSinger(singerId).position);
                }
            }
        }
        else
        {
            m_rotModel.singerAddWithSong(ui->comboBoxRegSingers->currentText(), ui->comboBoxPosition->currentIndex(), m_songId, ui->spinBoxKeyChange->value(), true);
            close();
        }
        break;
    }
}

void DlgAddSong::rotationSingerSelected()
{
    if (ui->listWidgetRotSingers->currentItem())
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

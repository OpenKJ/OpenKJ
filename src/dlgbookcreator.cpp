#include "dlgbookcreator.h"
#include "ui_dlgbookcreator.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QPdfWriter>
#include <QProgressDialog>
#include <QSqlQuery>
#include <QStandardPaths>


DlgBookCreator::DlgBookCreator(QWidget *parent) :
        QDialog(parent),
        ui(new Ui::DlgBookCreator) {
    m_logger = spdlog::get("logger");
    ui->setupUi(this);
    loadSettings();
    setupConnections();
}

void DlgBookCreator::loadSettings() {
    m_settings.restoreWindowState(this);
    ui->cbxColumns->addItem("2", 2);
    ui->cbxColumns->addItem("3", 3);
    ui->cbxPageSize->addItem(tr("Letter"), QPageSize::Letter);
    ui->cbxPageSize->addItem(tr("Legal"), QPageSize::Legal);
    ui->cbxPageSize->addItem(tr("A4"), QPageSize::A4);
    ui->fontCbxArtist->setCurrentFont(m_settings.bookCreatorArtistFont());
    ui->fontCbxTitle->setCurrentFont(m_settings.bookCreatorTitleFont());
    ui->fontCbxHeader->setCurrentFont(m_settings.bookCreatorHeaderFont());
    ui->fontCbxFooter->setCurrentFont(m_settings.bookCreatorFooterFont());
    ui->spinBoxSizeArtist->setValue(m_settings.bookCreatorArtistFont().pointSize());
    ui->spinBoxSizeTitle->setValue(m_settings.bookCreatorTitleFont().pointSize());
    ui->spinBoxSizeHeader->setValue(m_settings.bookCreatorHeaderFont().pointSize());
    ui->spinBoxSizeFooter->setValue(m_settings.bookCreatorFooterFont().pointSize());
    ui->checkBoxBoldArtist->setChecked(m_settings.bookCreatorArtistFont().bold());
    ui->checkBoxBoldTitle->setChecked(m_settings.bookCreatorTitleFont().bold());
    ui->checkBoxBoldHeader->setChecked(m_settings.bookCreatorHeaderFont().bold());
    ui->checkBoxBoldFooter->setChecked(m_settings.bookCreatorFooterFont().bold());
    ui->doubleSpinBoxLeft->setValue(m_settings.bookCreatorMarginLft());
    ui->doubleSpinBoxRight->setValue(m_settings.bookCreatorMarginRt());
    ui->doubleSpinBoxTop->setValue(m_settings.bookCreatorMarginTop());
    ui->doubleSpinBoxBottom->setValue(m_settings.bookCreatorMarginBtm());
    ui->cbxColumns->setCurrentIndex(m_settings.bookCreatorCols());
    ui->cbxPageSize->setCurrentIndex(m_settings.bookCreatorPageSize());
    ui->lineEditHeaderText->setText(m_settings.bookCreatorHeaderText());
    ui->lineEditFooterText->setText(m_settings.bookCreatorFooterText());
    ui->cbxPageNumbering->setChecked(m_settings.bookCreatorPageNumbering());
}

void DlgBookCreator::setupConnections() const {
    connect(ui->doubleSpinBoxLeft, qOverload<double>(&QDoubleSpinBox::valueChanged), &m_settings,
            &Settings::setBookCreatorMarginLft);
    connect(ui->doubleSpinBoxRight, qOverload<double>(&QDoubleSpinBox::valueChanged), &m_settings,
            &Settings::setBookCreatorMarginRt);
    connect(ui->doubleSpinBoxTop, qOverload<double>(&QDoubleSpinBox::valueChanged), &m_settings,
            &Settings::setBookCreatorMarginTop);
    connect(ui->doubleSpinBoxBottom, qOverload<double>(&QDoubleSpinBox::valueChanged), &m_settings,
            &Settings::setBookCreatorMarginBtm);
    connect(ui->fontCbxFooter, &QFontComboBox::currentFontChanged, this, &DlgBookCreator::saveFontSettings);
    connect(ui->spinBoxSizeFooter, qOverload<int>(&QSpinBox::valueChanged), this, &DlgBookCreator::saveFontSettings);
    connect(ui->checkBoxBoldFooter, qOverload<bool>(&QCheckBox::clicked), this, &DlgBookCreator::saveFontSettings);
    connect(ui->lineEditFooterText, &QLineEdit::textChanged, &m_settings, &Settings::setBookCreatorFooterText);
    connect(ui->fontCbxHeader, &QFontComboBox::currentFontChanged, this, &DlgBookCreator::saveFontSettings);
    connect(ui->spinBoxSizeHeader, qOverload<int>(&QSpinBox::valueChanged), this, &DlgBookCreator::saveFontSettings);
    connect(ui->checkBoxBoldHeader, qOverload<bool>(&QCheckBox::clicked), this, &DlgBookCreator::saveFontSettings);
    connect(ui->lineEditHeaderText, &QLineEdit::textChanged, &m_settings, &Settings::setBookCreatorHeaderText);
    connect(ui->fontCbxArtist, &QFontComboBox::currentFontChanged, this, &DlgBookCreator::saveFontSettings);
    connect(ui->spinBoxSizeArtist, qOverload<int>(&QSpinBox::valueChanged), this, &DlgBookCreator::saveFontSettings);
    connect(ui->checkBoxBoldArtist, qOverload<bool>(&QCheckBox::clicked), this, &DlgBookCreator::saveFontSettings);
    connect(ui->fontCbxTitle, &QFontComboBox::currentFontChanged, this, &DlgBookCreator::saveFontSettings);
    connect(ui->spinBoxSizeTitle, qOverload<int>(&QSpinBox::valueChanged), this, &DlgBookCreator::saveFontSettings);
    connect(ui->checkBoxBoldTitle, qOverload<bool>(&QCheckBox::clicked), this, &DlgBookCreator::saveFontSettings);
    connect(ui->doubleSpinBoxBottom, qOverload<double>(&QDoubleSpinBox::valueChanged), &m_settings,
            &Settings::setBookCreatorMarginBtm);
    connect(ui->doubleSpinBoxLeft, qOverload<double>(&QDoubleSpinBox::valueChanged), &m_settings,
            &Settings::setBookCreatorMarginLft);
    connect(ui->doubleSpinBoxRight, qOverload<double>(&QDoubleSpinBox::valueChanged), &m_settings,
            &Settings::setBookCreatorMarginRt);
    connect(ui->doubleSpinBoxTop, qOverload<double>(&QDoubleSpinBox::valueChanged), &m_settings,
            &Settings::setBookCreatorMarginTop);
    connect(ui->cbxPageNumbering, qOverload<bool>(&QCheckBox::clicked), &m_settings,
            &Settings::setBookCreatorPageNumbering);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &DlgBookCreator::close);
    connect(ui->btnGenerate, &QPushButton::clicked, this, &DlgBookCreator::btnGenerateClicked);
    connect(ui->cbxColumns, qOverload<int>(&QComboBox::currentIndexChanged), &m_settings,
            &Settings::setBookCreatorCols);
    connect(ui->cbxPageSize, qOverload<int>(&QComboBox::currentIndexChanged), &m_settings,
            &Settings::setBookCreatorPageSize);
}

DlgBookCreator::~DlgBookCreator() = default;

void DlgBookCreator::saveFontSettings() {
    QFont tFont(ui->fontCbxTitle->currentFont());
    tFont.setPointSize(ui->spinBoxSizeTitle->value());
    tFont.setBold(ui->checkBoxBoldTitle->isChecked());
    m_settings.setBookCreatorTitleFont(tFont);

    QFont aFont(ui->fontCbxArtist->currentFont());
    aFont.setPointSize(ui->spinBoxSizeArtist->value());
    aFont.setBold(ui->checkBoxBoldArtist->isChecked());
    m_settings.setBookCreatorArtistFont(aFont);

    QFont hFont(ui->fontCbxHeader->currentFont());
    hFont.setPointSize(ui->spinBoxSizeHeader->value());
    hFont.setBold(ui->checkBoxBoldHeader->isChecked());
    m_settings.setBookCreatorHeaderFont(hFont);

    QFont fFont(ui->fontCbxFooter->currentFont());
    fFont.setPointSize(ui->spinBoxSizeFooter->value());
    fFont.setBold(ui->checkBoxBoldFooter->isChecked());
    m_settings.setBookCreatorFooterFont(fFont);
}

QStringList DlgBookCreator::getArtists() {
    QSqlQuery query;
    QStringList artists;
    QString sql = "SELECT DISTINCT artist FROM dbsongs WHERE discid != '!!BAD!!' AND discid != '!!DROPPED!!' ORDER BY artist";
    query.exec(sql);
    while (query.next()) {
        artists.append(query.value("artist").toString());
    }
    return artists;
}

QStringList DlgBookCreator::getTitles(const QString &artist) {
    QSqlQuery query;
    QStringList titles;
    QString sql = "SELECT DISTINCT title FROM dbsongs WHERE artist = :artist AND discid != '!!BAD!!' AND discid != '!!DROPPED!!' ORDER BY title";
    query.prepare(sql);
    query.bindValue(":artist", artist);
    query.exec();
    while (query.next()) {
        titles.append(query.value("title").toString());
    }
    return titles;
}

void DlgBookCreator::writePdf(const QString &filename, int nCols) {
    QProgressDialog progress(this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setCancelButton(nullptr);
    progress.setLabelText("Gathering artist data");
    progress.setValue(0);
    progress.setMaximum(0);
    progress.show();
    m_logger->info("{} Beginning pdf book generation",m_loggingPrefix);
    QFont aFont = m_settings.bookCreatorArtistFont();
    QFont tFont = m_settings.bookCreatorTitleFont();
    QFont hFont = m_settings.bookCreatorHeaderFont();
    QFont fFont = m_settings.bookCreatorFooterFont();
    QPdfWriter pdf(filename);
    pdf.setPageSize(QPageSize(static_cast<QPageSize::PageSizeId>(ui->cbxPageSize->currentData().toInt())));
    pdf.setPageMargins(
            QMarginsF(ui->doubleSpinBoxLeft->value(), ui->doubleSpinBoxTop->value(), ui->doubleSpinBoxRight->value(),
                      ui->doubleSpinBoxBottom->value()), QPageLayout::Inch);
    QPainter painter(&pdf);
    m_logger->info("{} Getting artists",m_loggingPrefix);
    QStringList artists = getArtists();
    m_logger->info("{} Got {} artists",m_loggingPrefix, artists.size());
    QStringList entries;
    m_logger->info("{} Getting titles for artists",m_loggingPrefix);
    progress.setLabelText("Parsing song data");
    progress.setMaximum(artists.size());
    for (int i = 0; i < artists.size(); i++) {
        QApplication::processEvents();
        entries.append("-" + artists.at(i));
        QStringList titles = getTitles(artists.at(i));
        for (int j = 0; j < titles.size(); j++) {
            entries.append("+" + titles.at(j));
        }
        progress.setValue(i);
    }
    m_logger->info("{} Done getting titles",m_loggingPrefix);
    QPen pen;
    pen.setColor(QColor(0, 0, 0));
    pen.setWidth(4);
    painter.setPen(pen);
    painter.setFont(tFont);
    int lineOffset;
    int lineOffset2 = 0;
    QRect txtRect;
    int fontHeight = painter.fontMetrics().height();
    QString lastArtist;
    lineOffset = painter.viewport().width() / 2;
    if (nCols == 3) {
        lineOffset = painter.viewport().width() / 3;
        lineOffset2 = lineOffset + lineOffset;
    }
    int curDrawPos;
    int pages = 0;
    m_logger->info("{} Writing data to pdf",m_loggingPrefix);
    progress.setLabelText("Writing data to PDF");
    progress.setValue(0);
    progress.setMaximum(entries.size());
    int curEntry = 0;
    m_logger->info("{} Generating pages",m_loggingPrefix);
    while (!entries.isEmpty()) {
        QApplication::processEvents();
        pages++;
        m_logger->debug("{} Generating page {}",m_loggingPrefix, pages);
        int topOffset = 40;
        int headerOffset = 0;
        int bottomOffset = 0;
        if (ui->lineEditHeaderText->text() != "") {
            painter.setFont(hFont);
            painter.drawText(0, 0, painter.viewport().width(), painter.fontMetrics().height(), Qt::AlignCenter,
                             ui->lineEditHeaderText->text());
            headerOffset = painter.fontMetrics().height() + 50;
        }
        if (ui->lineEditFooterText->text() != "" || m_settings.bookCreatorPageNumbering()) {
            bottomOffset = 45;
            painter.setFont(fFont);
            int fFontHeight = painter.fontMetrics().height();
            if (m_settings.bookCreatorFooterText() != "")
                painter.drawText(0, painter.viewport().height() - fFontHeight, painter.viewport().width(),
                                 painter.fontMetrics().height(), Qt::AlignCenter, m_settings.bookCreatorFooterText());
            if (m_settings.bookCreatorPageNumbering()) {
                QString pageStr = tr("Page ") + QString::number(pages);
                txtRect = painter.fontMetrics().boundingRect(pageStr);
                painter.drawText(painter.viewport().width() - txtRect.width() - 20,
                                 painter.viewport().height() - txtRect.height(), txtRect.width(), txtRect.height(),
                                 Qt::AlignRight, pageStr);
            }
            bottomOffset = fFontHeight + bottomOffset;
        }
        painter.drawLine(0, headerOffset, 0, painter.viewport().height() - bottomOffset);
        painter.drawLine(painter.viewport().width(), headerOffset, painter.viewport().width(),
                         painter.viewport().height() - bottomOffset);
        painter.drawLine(0, headerOffset, painter.viewport().width(), headerOffset);
        painter.drawLine(0, painter.viewport().height() - bottomOffset, painter.viewport().width(),
                         painter.viewport().height() - bottomOffset);
        if (nCols == 2)
            painter.drawLine(lineOffset, headerOffset, lineOffset, painter.viewport().height() - bottomOffset);
        if (nCols == 3) {
            painter.drawLine(lineOffset, headerOffset, lineOffset, painter.viewport().height() - bottomOffset);
            painter.drawLine(lineOffset2, headerOffset, lineOffset2, painter.viewport().height() - bottomOffset);
        }
        curDrawPos = topOffset + headerOffset;
        while ((curDrawPos + fontHeight) <= (painter.viewport().height() - bottomOffset)) {
            QApplication::processEvents();
            if (entries.isEmpty())
                break;
            QString entry;
            if ((curDrawPos == (topOffset + headerOffset)) && (entries.at(0).at(0) == QString("+"))) {
                // We're at the top and it's not an artist entry, re-display artist
                entry = "-" + lastArtist + tr(" (cont'd)");
            } else if ((curDrawPos + (2 * fontHeight) >= (painter.viewport().height() - bottomOffset)) &&
                       (entries.at(0).at(0) == QString("-"))) {
                // We're on the last line and it's an artist, skip it to the next col/page
                curDrawPos = curDrawPos + fontHeight;
                continue;
            } else
                entry = entries.takeFirst();
            if (entry.at(0) == QString("-")) {
                painter.setFont(aFont);
                txtRect = painter.fontMetrics().boundingRect(entry);
                entry.remove(0, 1);
                painter.drawText(200, curDrawPos, txtRect.width(), txtRect.height(), Qt::AlignLeft, entry);
                lastArtist = entry;
            } else if (entry.at(0) == QString("+")) {
                painter.setFont(tFont);
                txtRect = painter.fontMetrics().boundingRect(entry);
                entry.remove(0, 1);
                painter.drawText(400, curDrawPos, txtRect.width(), txtRect.height(), Qt::AlignLeft, entry);

            }
            curDrawPos = curDrawPos + fontHeight;
        }
        curDrawPos = topOffset + headerOffset;
        while ((curDrawPos + fontHeight) <= (painter.viewport().height() - bottomOffset)) {
            QApplication::processEvents();
            if (entries.isEmpty())
                break;
            QString entry;
            if ((curDrawPos == (topOffset + headerOffset)) && (entries.at(0).at(0) == QString("+"))) {
                // We're at the top and it's not an artist entry, re-display artist
                entry = "-" + lastArtist + tr(" (cont'd)");
            } else if ((curDrawPos + (2 * fontHeight) >= (painter.viewport().height() - bottomOffset)) &&
                       (entries.at(0).at(0) == QString("-"))) {
                // We're on the last line and it's an artist, skip it to the next col/page
                curDrawPos = curDrawPos + fontHeight;
                continue;
            } else
                entry = entries.takeFirst();
            if (entry.at(0) == QString("-")) {
                painter.setFont(aFont);
                txtRect = painter.fontMetrics().boundingRect(entry);
                entry.remove(0, 1);
                painter.drawText(lineOffset + 200, curDrawPos, txtRect.width(), txtRect.height(), Qt::AlignLeft, entry);
                lastArtist = entry;
            } else if (entry.at(0) == QString("+")) {
                painter.setFont(tFont);
                txtRect = painter.fontMetrics().boundingRect(entry);
                entry.remove(0, 1);
                painter.drawText(lineOffset + 400, curDrawPos, txtRect.width(), txtRect.height(), Qt::AlignLeft, entry);
            }
            curDrawPos = curDrawPos + fontHeight;
        }
        if (nCols == 3) {
            curDrawPos = topOffset + headerOffset;
            while ((curDrawPos + fontHeight) <= (painter.viewport().height() - bottomOffset)) {
                QApplication::processEvents();
                if (entries.isEmpty())
                    break;
                QString entry;
                if ((curDrawPos == (topOffset + headerOffset)) && (entries.at(0).at(0) == QString("+"))) {
                    // We're at the top and it's not an artist entry, re-display artist
                    entry = "-" + lastArtist + tr(" (cont'd)");
                } else if ((curDrawPos + (2 * fontHeight) >= (painter.viewport().height() - bottomOffset)) &&
                           (entries.at(0).at(0) == QString("-"))) {
                    // We're on the last line and it's an artist, skip it to the next col/page
                    curDrawPos = curDrawPos + fontHeight;
                    continue;
                } else
                    entry = entries.takeFirst();
                if (entry.at(0) == QString("-")) {
                    painter.setFont(aFont);
                    txtRect = painter.fontMetrics().boundingRect(entry);
                    entry.remove(0, 1);
                    painter.drawText(lineOffset2 + 200, curDrawPos, txtRect.width(), txtRect.height(), Qt::AlignLeft,
                                     entry);
                    lastArtist = entry;
                } else if (entry.at(0) == QString("+")) {
                    painter.setFont(tFont);
                    txtRect = painter.fontMetrics().boundingRect(entry);
                    entry.remove(0, 1);
                    painter.drawText(lineOffset2 + 400, curDrawPos, txtRect.width(), txtRect.height(), Qt::AlignLeft,
                                     entry);
                }
                curDrawPos = curDrawPos + fontHeight;
            }
        }
        if (!entries.isEmpty()) {
            pdf.newPage();
            pdf.setPageMargins(QMarginsF(ui->doubleSpinBoxLeft->value(), ui->doubleSpinBoxTop->value(),
                                         ui->doubleSpinBoxRight->value(), ui->doubleSpinBoxBottom->value()),
                               QPageLayout::Inch);
        }
        curEntry++;
        progress.setValue(curEntry);
    }
    m_logger->info("{} Done writing data to pdf",m_loggingPrefix);
    m_logger->info("{} Finalizing pdf",m_loggingPrefix);
    progress.setLabelText("Finalizing PDF");
    progress.setMaximum(0);
    progress.setValue(0);
    painter.end();
    progress.close();
    QMessageBox msgBox(this);
    msgBox.setText("Songbook PDF generation complete");
    msgBox.exec();
    m_logger->info("{} Songbook generation complete",m_loggingPrefix);
}


void DlgBookCreator::btnGenerateClicked() {
    QString defFn = "Songbook.pdf";
#ifdef Q_OS_LINUX
    QString defaultFilePath =
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + defFn;
    QString saveFilePath = QFileDialog::getSaveFileName(this, "Select songbook filename", defaultFilePath,
                                                        "PDF Files (*.pdf)", nullptr, QFileDialog::DontUseNativeDialog);
#else
    QString defaultFilePath =
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + defFn;
    QString saveFilePath = QFileDialog::getSaveFileName(this, "Select songbook filename", defaultFilePath,
                                                        "PDF Files (*.pdf)", nullptr);
#endif
    if (saveFilePath != "") {
        QApplication::processEvents();
        writePdf(saveFilePath, ui->cbxColumns->currentData().toInt());
    }
}

void DlgBookCreator::resizeEvent(QResizeEvent *event) {
    m_settings.saveWindowState(this);
    QDialog::resizeEvent(event);
}


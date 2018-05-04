#include "dlgbookcreator.h"
#include "ui_dlgbookcreator.h"
#include <QDebug>
#include <QFileDialog>
#include <QPainter>
#include <QPdfWriter>
#include <QPrinter>
#include <QSqlQuery>
#include <QStandardPaths>

DlgBookCreator::DlgBookCreator(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgBookCreator)
{
    setupdone = false;
    ui->setupUi(this);
    ui->cbxColumns->addItem("2", 2);
    ui->cbxColumns->addItem("3", 3);
    ui->cbxPageSize->addItem("Letter", QPagedPaintDevice::Letter);
    ui->cbxPageSize->addItem("Legal", QPagedPaintDevice::Legal);
    ui->cbxPageSize->addItem("A4", QPagedPaintDevice::A4);
    settings = new Settings(this);
    qWarning() << "Header font: " << settings->bookCreatorArtistFont().toString();
    qWarning() << "Item font:   " << settings->bookCreatorTitleFont().toString();
    ui->fontCbxArtist->setCurrentFont(settings->bookCreatorArtistFont());
    ui->fontCbxTitle->setCurrentFont(settings->bookCreatorTitleFont());
    ui->fontCbxHeader->setCurrentFont(settings->bookCreatorHeaderFont());
    ui->spinBoxSizeArtist->setValue(settings->bookCreatorArtistFont().pointSize());
    ui->spinBoxSizeTitle->setValue(settings->bookCreatorTitleFont().pointSize());
    ui->spinBoxSizeHeader->setValue(settings->bookCreatorTitleFont().pointSize());
    ui->checkBoxBoldArtist->setChecked(settings->bookCreatorArtistFont().bold());
    ui->checkBoxBoldTitle->setChecked(settings->bookCreatorTitleFont().bold());
    ui->checkBoxBoldHeader->setChecked(settings->bookCreatorHeaderFont().bold());
    ui->doubleSpinBoxLeft->setValue(settings->bookCreatorMarginLft());
    ui->doubleSpinBoxRight->setValue(settings->bookCreatorMarginRt());
    ui->doubleSpinBoxTop->setValue(settings->bookCreatorMarginTop());
    ui->doubleSpinBoxBottom->setValue(settings->bookCreatorMarginBtm());
    ui->cbxColumns->setCurrentIndex(settings->bookCreatorCols());
    ui->cbxPageSize->setCurrentIndex(settings->bookCreatorPageSize());
    ui->lineEditHeaderText->setText(settings->bookCreatorHeaderText());
    connect(ui->doubleSpinBoxLeft, SIGNAL(valueChanged(double)), settings, SLOT(setBookCreatorMarginLft(double)));
    connect(ui->doubleSpinBoxRight, SIGNAL(valueChanged(double)), settings, SLOT(setBookCreatorMarginRt(double)));
    connect(ui->doubleSpinBoxTop, SIGNAL(valueChanged(double)), settings, SLOT(setBookCreatorMarginTop(double)));
    connect(ui->doubleSpinBoxBottom, SIGNAL(valueChanged(double)), settings, SLOT(setBookCreatorMarginBtm(double)));

    setupdone = true;

}

DlgBookCreator::~DlgBookCreator()
{
    delete ui;
}

void DlgBookCreator::on_buttonBox_clicked(QAbstractButton *button)
{
    Q_UNUSED(button);
    close();
}

void DlgBookCreator::on_fontCbxArtist_currentFontChanged(const QFont &f)
{
    Q_UNUSED(f);
    saveFontSettings();
}
void DlgBookCreator::on_fontCbxHeader_currentFontChanged(const QFont &f)
{
    Q_UNUSED(f);
    saveFontSettings();
}
void DlgBookCreator::on_fontCbxTitle_currentFontChanged(const QFont &f)
{
    Q_UNUSED(f);
    saveFontSettings();
}

void DlgBookCreator::on_spinBoxSizeArtist_valueChanged(int arg1)
{
    Q_UNUSED(arg1);
    saveFontSettings();
}
void DlgBookCreator::on_spinBoxSizeHeader_valueChanged(int arg1)
{
    Q_UNUSED(arg1);
    saveFontSettings();
}
void DlgBookCreator::on_spinBoxSizeTitle_valueChanged(int arg1)
{
    Q_UNUSED(arg1);
    saveFontSettings();
}

void DlgBookCreator::on_checkBoxBoldArtist_stateChanged(int arg1)
{
    Q_UNUSED(arg1);
    saveFontSettings();
}
void DlgBookCreator::on_checkBoxBoldHeader_stateChanged(int arg1)
{
    Q_UNUSED(arg1);
    saveFontSettings();
}
void DlgBookCreator::on_checkBoxBoldTitle_stateChanged(int arg1)
{
    Q_UNUSED(arg1);
    saveFontSettings();
}

void DlgBookCreator::on_comboBoxSort_currentIndexChanged(int index)
{
    if (setupdone)
    {
        settings->setBookCreatorSortCol(index);
        qWarning() << "Set sort col to: " << index;
    }
}

void DlgBookCreator::saveFontSettings()
{
    if (setupdone)
    {
        QFont tFont(ui->fontCbxTitle->currentFont());
        tFont.setPointSize(ui->spinBoxSizeTitle->value());
        tFont.setBold(ui->checkBoxBoldTitle->isChecked());
        settings->setBookCreatorTitleFont(tFont);

        QFont aFont(ui->fontCbxArtist->currentFont());
        aFont.setPointSize(ui->spinBoxSizeArtist->value());
        aFont.setBold(ui->checkBoxBoldArtist->isChecked());
        settings->setBookCreatorArtistFont(aFont);

        QFont hFont(ui->fontCbxHeader->currentFont());
        hFont.setPointSize(ui->spinBoxSizeHeader->value());
        hFont.setBold(ui->checkBoxBoldHeader->isChecked());
        settings->setBookCreatorHeaderFont(hFont);
    }
}

QStringList DlgBookCreator::getArtists()
{
    QSqlQuery query;
    QStringList artists;
    QString sql = "SELECT DISTINCT artist FROM dbsongs ORDER BY artist";
    query.exec(sql);
    while (query.next())
    {
        artists.append(query.value("artist").toString());
    }
    return artists;
}

QStringList DlgBookCreator::getTitles(QString artist)
{
    QSqlQuery query;
    QStringList titles;
    QString sql = "SELECT DISTINCT title FROM dbsongs WHERE artist = :artist AND discid != '!!BAD!!' ORDER BY title";
    query.prepare(sql);
    query.bindValue(":artist", artist);
    query.exec();
    while (query.next())
    {
        titles.append(query.value("title").toString());
    }
    return titles;
}

void DlgBookCreator::writePdf(QString filename, int nCols)
{
    QFont aFont = settings->bookCreatorArtistFont();
    QFont tFont = settings->bookCreatorTitleFont();
    QFont hFont = settings->bookCreatorHeaderFont();
    QPdfWriter pdf(filename);
    pdf.setPageSize(static_cast<QPagedPaintDevice::PageSize>(ui->cbxPageSize->currentData().toInt()));
    pdf.setPageMargins(QMarginsF(ui->doubleSpinBoxLeft->value(),ui->doubleSpinBoxTop->value(),ui->doubleSpinBoxRight->value(),ui->doubleSpinBoxBottom->value()), QPageLayout::Inch);
    QPainter painter(&pdf);
    QStringList artists = getArtists();
    QStringList entries;
    for (int i=0; i < artists.size(); i++)
    {
        entries.append("-" + artists.at(i));
        QStringList titles = getTitles(artists.at(i));
        for (int j=0; j < titles.size(); j++)
        {
            entries.append("+" + titles.at(j));
        }

    }
    QPen pen;
    pen.setColor(QColor(0,0,0));
    pen.setWidth(4);
    painter.setPen(pen);
    painter.setFont(tFont);
    int lineOffset;
    int lineOffset2 = 0;
    QRect txtRect;
    int fontHeight = painter.fontMetrics().height();
    //painter.drawLine(painter.viewport().width() / 2, 0, painter.viewport().width() / 2, painter.viewport().height());
    QString lastArtist;
    lineOffset = painter.viewport().width() / 2;
    if (nCols == 3)
    {
        lineOffset = painter.viewport().width() / 3;
        lineOffset2 = lineOffset + lineOffset;
    }
    int curDrawPos;
    while (!entries.isEmpty())
    {
        int topOffset = 40;
        int headerOffset = 0;
        if (ui->lineEditHeaderText->text() != "")
        {
            painter.setFont(hFont);
            painter.drawText(0, 0, painter.viewport().width(), painter.fontMetrics().height(), Qt::AlignCenter, ui->lineEditHeaderText->text());
            headerOffset = painter.fontMetrics().height() + 50;
        }
        painter.drawLine(0,headerOffset,0,painter.viewport().height());
        painter.drawLine(painter.viewport().width(), headerOffset, painter.viewport().width(), painter.viewport().height());
        painter.drawLine(0,headerOffset,painter.viewport().width(),headerOffset);
        painter.drawLine(0,painter.viewport().height(), painter.viewport().width(), painter.viewport().height());
        if (nCols == 2)
            painter.drawLine(lineOffset, headerOffset, lineOffset, painter.viewport().height());
        if (nCols == 3)
        {
            painter.drawLine(lineOffset, headerOffset, lineOffset, painter.viewport().height());
            painter.drawLine(lineOffset2, headerOffset, lineOffset2, painter.viewport().height());
        }
        curDrawPos = topOffset + headerOffset;
        while ((curDrawPos + fontHeight) <= painter.viewport().height()) {
            if (entries.isEmpty())
                break;
            QString entry;
            if ((curDrawPos == (topOffset + headerOffset)) && (entries.at(0).at(0) == "+"))
            {
                // We're at the top and it's not an artist entry, re-display artist
                entry = "-" + lastArtist + " (cont'd)";
            }
            else if ((curDrawPos + (2 * fontHeight) >= painter.viewport().height()) && (entries.at(0).at(0) == "-"))
            {
                // We're on the last line and it's an artist, skip it to the next col/page
                curDrawPos = curDrawPos + fontHeight;
                continue;
            }
            else
                entry = entries.takeFirst();
            if (entry.at(0) == "-")
            {
                painter.setFont(aFont);
                txtRect = painter.fontMetrics().boundingRect(entry);
                entry.remove(0,1);
                painter.drawText(200, curDrawPos, txtRect.width(), txtRect.height(), Qt::AlignLeft, entry);
                lastArtist = entry;
            }
            else if (entry.at(0) == "+")
            {
                painter.setFont(tFont);
                txtRect = painter.fontMetrics().boundingRect(entry);
                entry.remove(0,1);
                painter.drawText(400, curDrawPos, txtRect.width(), txtRect.height(), Qt::AlignLeft, entry);

            }
            curDrawPos = curDrawPos + fontHeight;
        }
        curDrawPos = topOffset + headerOffset;
        while ((curDrawPos + fontHeight) <= painter.viewport().height()) {
            if (entries.isEmpty())
                break;
            QString entry;
            if ((curDrawPos == (topOffset + headerOffset)) && (entries.at(0).at(0) == "+"))
            {
                // We're at the top and it's not an artist entry, re-display artist
                entry = "-" + lastArtist + " (cont'd)";
            }
            else if ((curDrawPos + (2 * fontHeight) >= painter.viewport().height()) && (entries.at(0).at(0) == "-"))
            {
                // We're on the last line and it's an artist, skip it to the next col/page
                curDrawPos = curDrawPos + fontHeight;
                continue;
            }
            else
                entry = entries.takeFirst();
            if (entry.at(0) == "-")
            {
                painter.setFont(aFont);
                txtRect = painter.fontMetrics().boundingRect(entry);
                entry.remove(0,1);
                painter.drawText(lineOffset + 200, curDrawPos, txtRect.width(), txtRect.height(), Qt::AlignLeft, entry);
                lastArtist = entry;
            }
            else if (entry.at(0) == "+")
            {
                painter.setFont(tFont);
                txtRect = painter.fontMetrics().boundingRect(entry);
                entry.remove(0,1);
                painter.drawText(lineOffset + 400, curDrawPos, txtRect.width(), txtRect.height(), Qt::AlignLeft, entry);
            }
            curDrawPos = curDrawPos + fontHeight;
        }
        if (nCols == 3)
        {
            curDrawPos = topOffset + headerOffset;
            while ((curDrawPos + fontHeight) <= painter.viewport().height()) {
                if (entries.isEmpty())
                    break;
                QString entry;
                if ((curDrawPos == (topOffset + headerOffset)) && (entries.at(0).at(0) == "+"))
                {
                    // We're at the top and it's not an artist entry, re-display artist
                    entry = "-" + lastArtist + " (cont'd)";
                }
                else if ((curDrawPos + (2 * fontHeight) >= painter.viewport().height()) && (entries.at(0).at(0) == "-"))
                {
                    // We're on the last line and it's an artist, skip it to the next col/page
                    curDrawPos = curDrawPos + fontHeight;
                    continue;
                }
                else
                    entry = entries.takeFirst();
                if (entry.at(0) == "-")
                {
                    painter.setFont(aFont);
                    txtRect = painter.fontMetrics().boundingRect(entry);
                    entry.remove(0,1);
                    painter.drawText(lineOffset2 + 200, curDrawPos, txtRect.width(), txtRect.height(), Qt::AlignLeft, entry);
                    lastArtist = entry;
                }
                else if (entry.at(0) == "+")
                {
                    painter.setFont(tFont);
                    txtRect = painter.fontMetrics().boundingRect(entry);
                    entry.remove(0,1);
                    painter.drawText(lineOffset2 + 400, curDrawPos, txtRect.width(), txtRect.height(), Qt::AlignLeft, entry);
                }
                curDrawPos = curDrawPos + fontHeight;
            }
        }
        if (!entries.isEmpty())
        {
            pdf.newPage();
            pdf.setPageMargins(QMarginsF(ui->doubleSpinBoxLeft->value(),ui->doubleSpinBoxTop->value(),ui->doubleSpinBoxRight->value(),ui->doubleSpinBoxBottom->value()), QPageLayout::Inch);
        }

    }
//    painter.drawText(txtRect, Qt::AlignLeft, "This is some title");
    qWarning() << rect();

    painter.end();
}



void DlgBookCreator::on_btnGenerate_clicked()
{
    QString defFn = "Songbook.pdf";
    QString defaultFilePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + defFn;
    qDebug() << "Default save location: " << defaultFilePath;
    QString saveFilePath = QFileDialog::getSaveFileName(this,tr("Select songbook filename"), defaultFilePath, tr("(*.pdf)"));
    if (saveFilePath != "")
    {
        writePdf(saveFilePath, ui->cbxColumns->currentData().toInt());
    }
}

void DlgBookCreator::on_cbxColumns_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    if (!setupdone)
        return;
    settings->setBookCreatorCols(ui->cbxColumns->currentIndex());
}

void DlgBookCreator::on_cbxPageSize_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    if (!setupdone)
        return;
    settings->setBookCreatorPageSize(ui->cbxPageSize->currentIndex());
}

void DlgBookCreator::on_doubleSpinBoxLeft_valueChanged(double arg1)
{
    settings->setBookCreatorMarginLft(arg1);
}

void DlgBookCreator::on_doubleSpinBoxRight_valueChanged(double arg1)
{
    settings->setBookCreatorMarginRt(arg1);
}

void DlgBookCreator::on_doubleSpinBoxTop_valueChanged(double arg1)
{
    settings->setBookCreatorMarginTop(arg1);
}

void DlgBookCreator::on_doubleSpinBoxBottom_valueChanged(double arg1)
{
    settings->setBookCreatorMarginBtm(arg1);
}

void DlgBookCreator::on_lineEditHeaderText_editingFinished()
{
    settings->setBookCreatorHeaderText(ui->lineEditHeaderText->text());
}

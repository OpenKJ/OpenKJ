#include "dlgbookcreator.h"
#include "ui_dlgbookcreator.h"
#include <QDebug>
#include <QFileDialog>
#include <QPrinter>
#include <QSqlQuery>
#include <QStandardPaths>

DlgBookCreator::DlgBookCreator(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgBookCreator)
{
    setupdone = false;
    ui->setupUi(this);
    settings = new Settings(this);
    ui->comboBoxSort->addItem("Artist");
    ui->comboBoxSort->addItem("Title");
    qWarning() << "Header font: " << settings->bookCreatorHeaderFont().toString();
    qWarning() << "Item font:   " << settings->bookCreatorItemFont().toString();
    ui->fontCbxHeader->setCurrentFont(settings->bookCreatorHeaderFont());
    ui->fontCbxItem->setCurrentFont(settings->bookCreatorItemFont());
    ui->spinBoxSizeHeader->setValue(settings->bookCreatorHeaderFont().pointSize());
    ui->spinBoxSizeItem->setValue(settings->bookCreatorItemFont().pointSize());
    ui->checkBoxBoldHeader->setChecked(settings->bookCreatorHeaderFont().bold());
    ui->checkBoxBoldItem->setChecked(settings->bookCreatorItemFont().bold());
    ui->comboBoxSort->setCurrentIndex(settings->bookCreatorSortCol());
    ui->doubleSpinBoxLeft->setValue(settings->bookCreatorMarginLft());
    ui->doubleSpinBoxRight->setValue(settings->bookCreatorMarginRt());
    ui->doubleSpinBoxTop->setValue(settings->bookCreatorMarginTop());
    ui->doubleSpinBoxBottom->setValue(settings->bookCreatorMarginBtm());
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

void DlgBookCreator::on_fontCbxHeader_currentFontChanged(const QFont &f)
{
    Q_UNUSED(f);
    saveFontSettings();
}

void DlgBookCreator::on_fontCbxItem_currentFontChanged(const QFont &f)
{
    Q_UNUSED(f);
    saveFontSettings();
}

void DlgBookCreator::on_spinBoxSizeHeader_valueChanged(int arg1)
{
    Q_UNUSED(arg1);
    saveFontSettings();
}

void DlgBookCreator::on_spinBoxSizeItem_valueChanged(int arg1)
{
    Q_UNUSED(arg1);
    saveFontSettings();
}

void DlgBookCreator::on_checkBoxBoldHeader_stateChanged(int arg1)
{
    Q_UNUSED(arg1);
    saveFontSettings();
}

void DlgBookCreator::on_checkBoxBoldItem_stateChanged(int arg1)
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
        QFont iFont(ui->fontCbxItem->currentFont());
        iFont.setPointSize(ui->spinBoxSizeItem->value());
        iFont.setBold(ui->checkBoxBoldItem->isChecked());
        settings->setBookCreatorItemFont(iFont);

        QFont hFont(ui->fontCbxHeader->currentFont());
        hFont.setPointSize(ui->spinBoxSizeHeader->value());
        hFont.setBold(ui->checkBoxBoldHeader->isChecked());
        settings->setBookCreatorHeaderFont(hFont);
    }
}

QString DlgBookCreator::getTable()
{
    QSqlQuery query;
    QString sql;
    QString sort;
    QString itemStyle;
    QString headerStyle;
    QFont hFont = settings->bookCreatorHeaderFont();
    QFont iFont = settings->bookCreatorItemFont();
    QString iWeight = "normal";
    if (iFont.bold())
        iWeight = "bold";
    QString hWeight = "normal";
    if (hFont.bold())
        hWeight = "bold";
    itemStyle = "font-family: " + iFont.family() + "; font-size: " + QString::number(iFont.pointSize()) + "pt; font-weight: " + iWeight + ";";
    qWarning() << "Item style: " << itemStyle;
    headerStyle = "font-family: " + hFont.family() + "; font-size: " + QString::number(hFont.pointSize()) + "pt; font-weight: " + hWeight + ";";
    qWarning() << "Header style: " << headerStyle;
    QString html;
    html.append("<table border=1 cellspacing=0 width=100%>");
    if (settings->bookCreatorSortCol() == 0)
    {
        sort = "Artist";
        sql = "SELECT DISTINCT artist,title from dbsongs ORDER BY artist,title";
        html.append("<thead><tr><th colspan=2 style=\"" + headerStyle + "\">Karaoke Songs by Artist</th></tr><tr><th style=\"" + headerStyle + "\">Artist</th><th style=\"" + headerStyle + "\">Title</th></tr></thead>");
    }
    else
    {
        sort = "Title";
        sql = "SELECT DISTINCT artist,title from dbsongs ORDER BY title,artist";
        html.append("<thead><tr><th colspan=2 style=\"" + headerStyle + "\">Karaoke Songs by Title</th></tr><tr><th style=\"" + headerStyle + "\">Title</th><th style=\"" + headerStyle + "\">Artist</th></tr></thead>");
    }


    bool alternate = false;
    query.exec(sql);
    while (query.next())
    {
        QString artist = query.value("artist").toString();
        QString title = query.value("title").toString();
        if ((artist == "") && (title == ""))
            continue;
        QString bgcolor = "#FFFFFF";
        if (alternate)
            bgcolor = "#D3D3D3";
        alternate = !alternate;
        QString data;
        if (settings->bookCreatorSortCol() == 0)
            data = "<tr style=\"background-color: " + bgcolor + ";\"><td style=\"" + itemStyle + "\">" + artist + "</td><td style=\"" + itemStyle + "\">" + title + "</td></tr>";
        else
            data = "<tr style=\"background-color: " + bgcolor + ";\"><td style=\"" + itemStyle + "\">" + title + "</td><td style=\"" + itemStyle + "\">" + artist + "</td></tr>";
        html.append(data);
    }
    html.append("</table>");
    return html;
}

void DlgBookCreator::writePdf(QString fileName)
{
    htmlOut.clear();
    htmlOut.append("<html><head></head><body style=\"margin-left: 0; margin-right: 0; margin-bottom: 0; margin-top: 0\">");
    htmlOut.append(getTable());
    htmlOut.append("</body></html>");
    QTextDocument document;
    document.setHtml(htmlOut);
    document.setDocumentMargin(0);
    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPaperSize(QPrinter::Letter);
    printer.setOutputFileName(fileName);
//    printer.setPageMargins(QMarginsF(0, 0, 0, 0), QPageLayout::Inch);
    printer.setPageMargins(settings->bookCreatorMarginLft(), settings->bookCreatorMarginTop(), settings->bookCreatorMarginRt(), settings->bookCreatorMarginBtm(), QPrinter::Inch);
    document.setPageSize(QSizeF(printer.pageRect().size()));
    document.print(&printer);
}

void DlgBookCreator::on_btnGenerate_clicked()
{
    QString defFn = "Songbook_by_Artist.pdf";
    if (settings->bookCreatorSortCol() != 0)
        defFn = "Songbook_by_Title.pdf";
    QString defaultFilePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + defFn;
    qDebug() << "Default save location: " << defaultFilePath;
    QString saveFilePath = QFileDialog::getSaveFileName(this,tr("Select songbook filename"), defaultFilePath, tr("(*.pdf)"));
    if (saveFilePath != "")
    {
        writePdf(saveFilePath);
    }
}

#include "regularimportdialog.h"
#include "ui_regularimportdialog.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>
#include <QMessageBox>
#include <QSqlQuery>

RegularImportDialog::RegularImportDialog(KhSongs *dbsongs, KhRegularSingers *regSingersPtr, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegularImportDialog)
{
    ui->setupUi(this);
    regSingers = regSingersPtr;
    dbSongs = dbsongs;
    curImportFile = "";
}

RegularImportDialog::~RegularImportDialog()
{
    delete ui;
}

void RegularImportDialog::on_pushButtonSelectFile_clicked()
{
    QString importFile = QFileDialog::getOpenFileName(this,tr("Select file to load regulars from"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), tr("(*.xml)"));
    if (importFile != "")
    {
        curImportFile = importFile;
        QStringList singers = regSingers->importLoadSingerList(importFile);
        ui->listWidgetRegulars->clear();
        ui->listWidgetRegulars->addItems(singers);
    }
}

void RegularImportDialog::on_pushButtonClose_clicked()
{
    close();
}

void RegularImportDialog::on_pushButtonImport_clicked()
{
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setStandardButtons(0);
    msgBox->setText("Importing regular singers, please wait...");
    msgBox->show();
    for (int i=0; i < ui->listWidgetRegulars->selectedItems().size(); i++)
    {
        importSinger(ui->listWidgetRegulars->selectedItems().at(i)->text());
    }
    msgBox->close();
    delete msgBox;
}

void RegularImportDialog::on_pushButtonImportAll_clicked()
{
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setStandardButtons(0);
    msgBox->setText("Importing regular singers, please wait...");
    msgBox->show();
    ui->listWidgetRegulars->selectAll();
    for (int i=0; i < ui->listWidgetRegulars->selectedItems().size(); i++)
    {
        importSinger(ui->listWidgetRegulars->selectedItems().at(i)->text());
    }
    msgBox->close();
    delete msgBox;
}

void RegularImportDialog::importSinger(QString name)
{
    qDebug() << "importSinger(" << name << ") called";
    if (regSingers->exists(name))
    {
        QMessageBox::warning(this, tr("Naming conflict"),QString("A regular singer named \"" + name + "\" already exists.  Please remove or rename the existing singer and try again."));
    }
    else
    {
        int regID = regSingers->add(name);
        KhRegularSinger *regSinger = regSingers->getByRegularID(regID);
        QList<KhRegImportSong> songs = regSingers->importLoadSongs(name, curImportFile);
        QSqlQuery query("BEGIN TRANSACTION");
        for (int i=0; i < songs.size(); i++)
        {
            QApplication::processEvents();
            KhSong *exactMatch = findExactSongMatch(songs.at(i));
            QApplication::processEvents();
            if (exactMatch != NULL)
            {
                int songId = exactMatch->ID;
                int keyChg = songs.at(i).keyChange();
                int pos = regSinger->getRegSongs()->getRegSongs()->size();
                regSinger->addSong(songId, keyChg, pos);
            }
            else
                QMessageBox::warning(this, tr("No song match found"),QString("An exact song DB match for the song \"" + songs.at(i).discId() + " - " + songs.at(i).artist() + " - " + songs.at(i).title() + "\" could not be found while importing singer \"" + name + "\", skipping import for this song."));
        }
        query.exec("COMMIT TRANSACTION");
    }
}

KhSong *RegularImportDialog::findExactSongMatch(KhRegImportSong importSong)
{
    for (int i=0; i < dbSongs->size(); i++)
    {
        if ((dbSongs->at(i)->DiscID.toLower() == importSong.discId().toLower()) && (dbSongs->at(i)->Artist.toLower() == importSong.artist().toLower()) && (dbSongs->at(i)->Title.toLower() == importSong.title().toLower()))
            return dbSongs->at(i);
    }
    return NULL;
}

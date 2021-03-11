/*
 * Copyright (c) 2013-2019 Thomas Isaac Lightburn
 *
 *
 * This file is part of OpenKJ.
 *
 * OpenKJ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "dlgrequests.h"
#include "ui_dlgrequests.h"
#include <QDesktopServices>
#include <QMenu>
#include <QMessageBox>
#include "settings.h"
#include "okjsongbookapi.h"
#include <QProgressDialog>
#include "src/models/tableviewtooltipfilter.h"
#include "dlgvideopreview.h"

extern Settings settings;
extern OKJSongbookAPI *songbookApi;

QString toMixedCase(const QString& s)
{
    if (s.isNull())
        return QString();
    if (s.size() < 1)
        return QString();
    QStringList parts = s.split(" ", QString::SkipEmptyParts);
    for (int i=1; i<parts.size(); ++i)
        parts[i].replace(0, 1, parts[i][0].toUpper());
    QString newStr = parts.join(" ");
    newStr.replace(0, 1, newStr.at(0).toUpper());
    return newStr;
}

DlgRequests::DlgRequests(TableModelRotation *rotationModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgRequests)
{
    curRequestId = -1;
    ui->setupUi(this);
    requestsModel = new TableModelRequests(this);
    dbModel.loadData();
    ui->tableViewRequests->setModel(requestsModel);
    ui->tableViewRequests->viewport()->installEventFilter(new TableViewToolTipFilter(ui->tableViewRequests));
    connect(requestsModel, SIGNAL(layoutChanged()), this, SLOT(requestsModified()));
    ui->tableViewSearch->setModel(&dbModel);
    ui->tableViewSearch->viewport()->installEventFilter(new TableViewToolTipFilter(ui->tableViewSearch));
    ui->groupBoxAddSong->setDisabled(true);
    ui->groupBoxSongDb->setDisabled(true);
    connect(ui->tableViewRequests->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(requestSelectionChanged(QItemSelection,QItemSelection)));
    connect(ui->tableViewSearch->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(songSelectionChanged(QItemSelection,QItemSelection)));
    connect(songbookApi, SIGNAL(synchronized(QTime)), this, SLOT(updateReceived(QTime)));
    connect(songbookApi, SIGNAL(sslError()), this, SLOT(sslError()));
    connect(songbookApi, SIGNAL(delayError(int)), this, SLOT(delayError(int)));
    rotModel = rotationModel;
    ui->comboBoxAddPosition->setEnabled(false);
    ui->comboBoxSingers->setEnabled(true);
    ui->lineEditSingerName->setEnabled(false);
    ui->labelAddPos->setEnabled(false);
    QStringList posOptions;
    posOptions << tr("Fair (full rotation)");
    posOptions << tr("Bottom of rotation");
    posOptions << tr("After current singer");
    ui->comboBoxAddPosition->addItems(posOptions);
    ui->comboBoxAddPosition->setCurrentIndex(settings.lastSingerAddPositionType());
    connect(&settings, &Settings::lastSingerAddPositionTypeChanged, ui->comboBoxAddPosition, &QComboBox::setCurrentIndex);
    connect(ui->comboBoxAddPosition, SIGNAL(currentIndexChanged(int)), &settings, SLOT(setLastSingerAddPositionType(int)));
    ui->tableViewSearch->hideColumn(TableModelKaraokeSongs::COL_ID);
    ui->tableViewSearch->hideColumn(TableModelKaraokeSongs::COL_FILENAME);
    ui->tableViewSearch->horizontalHeader()->resizeSection(4,75);
    ui->checkBoxDelOnAdd->setChecked(settings.requestRemoveOnRotAdd());
    connect(songbookApi, SIGNAL(venuesChanged(OkjsVenues)), this, SLOT(venuesChanged(OkjsVenues)));
    connect(ui->lineEditSearch, SIGNAL(escapePressed()), this, SLOT(lineEditSearchEscapePressed()));
    connect(ui->checkBoxDelOnAdd, SIGNAL(clicked(bool)), &settings, SLOT(setRequestRemoveOnRotAdd(bool)));
    ui->cbxAutoShowRequestsDlg->setChecked(settings.requestDialogAutoShow());
    connect(ui->cbxAutoShowRequestsDlg, SIGNAL(clicked(bool)), &settings, SLOT(setRequestDialogAutoShow(bool)));

    QFontMetrics fm(settings.applicationFont());
    QSize mcbSize(fm.height(), fm.height());
    ui->spinBoxKey->setMaximum(12);
    ui->spinBoxKey->setMinimum(-12);
    autoSizeViews();
    if (!settings.testingEnabled())
        ui->pushButtonRunTortureTest->hide();

}

int DlgRequests::numRequests()
{
    return requestsModel->count();
}

DlgRequests::~DlgRequests()
{

    delete ui;
}

void DlgRequests::databaseAboutToUpdate()
{

}

void DlgRequests::databaseUpdateComplete()
{
    dbModel.loadData();
    autoSizeViews();
}

void DlgRequests::databaseSongAdded()
{
    dbModel.loadData();
}

void DlgRequests::rotationChanged()
{
    QString curSelSinger = ui->comboBoxSingers->currentText();
    ui->comboBoxSingers->clear();
    QStringList singers = rotModel->singers();
    singers.sort(Qt::CaseInsensitive);
    ui->comboBoxSingers->addItems(singers);
    if (singers.contains(curSelSinger))
        ui->comboBoxSingers->setCurrentText(curSelSinger);
    int s = -1;
    for (int i=0; i < singers.size(); i++)
    {
        if (singers.at(i).toLower().trimmed() == curSelReqSinger.toLower().trimmed())
        {
            s = i;
            break;
        }
    }
    if (s != -1)
    {
        ui->comboBoxSingers->setCurrentIndex(s);
        ui->radioButtonExistingSinger->setChecked(true);
    }

}

void DlgRequests::updateIcons()
{
    QString thm = (settings.theme() == 1) ? ":/theme/Icons/okjbreeze-dark/" : ":/theme/Icons/okjbreeze/";
    ui->buttonRefresh->setIcon(QIcon(thm + "actions/22/view-refresh.svg"));
    ui->pushButtonClearReqs->setIcon(QIcon(thm + "actions/22/edit-clear-all.svg"));
    ui->pushButtonSearch->setIcon(QIcon(thm + "actions/22/edit-find.svg"));
    ui->pushButtonWebSearch->setIcon(QIcon(thm + "apps/48/internet-web-browser.svg"));
}

void DlgRequests::on_pushButtonClose_clicked()
{
    close();
}

void DlgRequests::requestsModified()
{
    static int testruns = 0;
    if ((requestsModel->count() > 0) && (settings.requestDialogAutoShow()))
    {
        this->show();
        this->raise();
    }
    autoSizeViews();
    if (requestsModel->count() > 0 && settings.testingEnabled() && testTimer.isActive())
    {
        ui->tableViewRequests->selectRow(0);
        QApplication::processEvents();
        ui->tableViewSearch->selectRow(0);
        QApplication::processEvents();
        on_pushButtonAddSong_clicked();
        songbookApi->removeRequest(requestsModel->requests().at(0).requestId());
        if (rotModel->rowCount() > 5)
            rotModel->singerDelete(rotModel->singerIdAtPosition(0));
        ui->pushButtonRunTortureTest->setText("Running Torture Test (" + QString::number(++testruns) + ")");
    }
}

void DlgRequests::on_pushButtonSearch_clicked()
{
    dbModel.search(ui->lineEditSearch->text());
}

void DlgRequests::on_lineEditSearch_returnPressed()
{
    dbModel.search(ui->lineEditSearch->text());
}

void DlgRequests::requestSelectionChanged(const QItemSelection &current, const QItemSelection &previous)
{
    ui->groupBoxAddSong->setDisabled(true);
    if (current.indexes().size() == 0)
    {
        dbModel.search("yeahjustsomethingitllneverfind.imlazylikethat");
        ui->groupBoxSongDb->setDisabled(true);
        ui->comboBoxSingers->setCurrentIndex(0);
        ui->spinBoxKey->setValue(0);
        curSelReqSinger = "";
        return;
    }
    QModelIndex index = current.indexes().at(0);
    Q_UNUSED(previous);
    if ((index.isValid()) && (ui->tableViewRequests->selectionModel()->selectedIndexes().size() > 0))
    {
        ui->groupBoxSongDb->setEnabled(true);
        ui->comboBoxSingers->clear();
        QString singerName = index.sibling(index.row(),0).data().toString();
        curSelReqSinger = singerName;
        QStringList singers = rotModel->singers();
        singers.sort(Qt::CaseInsensitive);
        ui->comboBoxSingers->addItems(singers);

        QString filterStr = index.sibling(index.row(),1).data().toString() + " " + index.sibling(index.row(),2).data().toString();
        dbModel.search(filterStr);
        ui->lineEditSearch->setText(filterStr);
        //ui->lineEditSingerName->setText(singerName);
        ui->lineEditSingerName->setText(toMixedCase(singerName));
        ui->spinBoxKey->setValue(requestsModel->requests().at(index.row()).key());

        int s = -1;
        for (int i=0; i < singers.size(); i++)
        {
            if (singers.at(i).toLower().trimmed() == singerName.toLower().trimmed())
            {
                s = i;
                break;
            }
        }
        if (s != -1)
        {
            ui->comboBoxSingers->setCurrentIndex(s);
            ui->radioButtonExistingSinger->setChecked(true);
        }
        else
        {
            ui->radioButtonNewSinger->setChecked(true);
        }
    }
    else
    {
        dbModel.search("yeahjustsomethingitllneverfind.imlazylikethat");
    }
}

void DlgRequests::songSelectionChanged(const QItemSelection &current, const QItemSelection &previous)
{
    Q_UNUSED(previous);
    if (current.indexes().size() == 0)
    {
        ui->groupBoxAddSong->setDisabled(true);
    }
    else
        ui->groupBoxAddSong->setEnabled(true);
}

void DlgRequests::on_radioButtonExistingSinger_toggled(bool checked)
{
    ui->comboBoxAddPosition->setEnabled(!checked);
    ui->comboBoxSingers->setEnabled(checked);
    ui->lineEditSingerName->setEnabled(!checked);
    ui->labelAddPos->setEnabled(!checked);
}

void DlgRequests::on_pushButtonClearReqs_clicked()
{
    QMessageBox msgBox;
    msgBox.setText("Are you sure?");
    msgBox.setInformativeText("This action will clear all received requests. This operation can not be undone.");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.addButton(QMessageBox::Cancel);
    QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
    msgBox.exec();
    if (msgBox.clickedButton() == yesButton)
    {
        ui->tableViewRequests->selectionModel()->clearSelection();
        ui->lineEditSearch->clear();
        ui->lineEditSingerName->clear();
        ui->comboBoxSingers->clear();
        ui->radioButtonExistingSinger->setChecked(true);
        songbookApi->clearRequests();
    }
}

void DlgRequests::on_tableViewRequests_clicked(const QModelIndex &index)
{
    if (index.column() == 5)
    {
        songbookApi->removeRequest(index.data(Qt::UserRole).toInt());
        ui->tableViewRequests->selectionModel()->clearSelection();
        ui->lineEditSearch->clear();
        ui->lineEditSingerName->clear();
        ui->comboBoxSingers->clear();
        ui->spinBoxKey->setValue(0);
        ui->radioButtonExistingSinger->setChecked(true);
    }
    else
    {
        curRequestId = index.data(Qt::UserRole).toInt();
    }
}

void DlgRequests::on_pushButtonAddSong_clicked()
{
    if (ui->tableViewRequests->selectionModel()->selectedIndexes().size() < 1)
        return;
    if (ui->tableViewSearch->selectionModel()->selectedIndexes().size() < 1)
        return;

    QModelIndex index = ui->tableViewSearch->selectionModel()->selectedIndexes().at(0);
    QModelIndex rIndex = ui->tableViewRequests->selectionModel()->selectedIndexes().at(0);
    int songid = index.sibling(index.row(),TableModelKaraokeSongs::COL_ID).data().toInt();
    int keyChg = requestsModel->requests().at(rIndex.row()).key();
    if (ui->radioButtonNewSinger->isChecked())
    {
        if (ui->lineEditSingerName->text() == "")
            return;
        else if (rotModel->singerExists(ui->lineEditSingerName->text()))
            return;
        else
        {
            int newSingerId = rotModel->singerAdd(ui->lineEditSingerName->text(), ui->comboBoxAddPosition->currentIndex());
            emit addRequestSong(songid, newSingerId, keyChg);
        }
    }
    else if (ui->radioButtonExistingSinger->isChecked())
    {
        emit addRequestSong(songid, rotModel->getSingerId(ui->comboBoxSingers->currentText()), keyChg);
    }
    if (settings.requestRemoveOnRotAdd())
    {

        songbookApi->removeRequest(curRequestId);
        ui->tableViewRequests->selectionModel()->clearSelection();
        ui->lineEditSearch->clear();
        ui->lineEditSingerName->clear();
        ui->comboBoxSingers->clear();
        ui->radioButtonExistingSinger->setChecked(true);
    }
}

void DlgRequests::on_tableViewSearch_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->tableViewSearch->indexAt(pos);
    if (index.isValid())
    {
        rtClickFile = index.sibling(index.row(),5).data().toString();
        QMenu contextMenu(this);
        contextMenu.addAction(tr("Preview"), this, SLOT(previewCdg()));
        contextMenu.exec(QCursor::pos());
    }
}

void DlgRequests::updateReceived(QTime updateTime)
{
    ui->labelLastUpdate->setText(updateTime.toString("hh:mm:ss AP"));
}

void DlgRequests::on_buttonRefresh_clicked()
{
    songbookApi->refreshRequests();
    songbookApi->refreshVenues();
}

void DlgRequests::sslError()
{
    QMessageBox::warning(this, tr("SSL Handshake Error"), tr("An error was encountered while establishing a secure connection to the requests server.  This is usually caused by an invalid or self-signed cert on the server.  You can set the requests client to ignore SSL errors in the network settings dialog."));
}

void DlgRequests::delayError(int seconds)
{
    QMessageBox::warning(this, tr("Possible Connectivity Issue"), tr("It has been ") + QString::number(seconds) + tr(" seconds since we last received a response from the requests server.  You may be missing new submitted requests.  Please ensure that your network connection is up and working."));
}

void DlgRequests::on_checkBoxAccepting_clicked(bool checked)
{
    songbookApi->setAccepting(checked);
}

void DlgRequests::venuesChanged(OkjsVenues venues)
{
    int venue = settings.requestServerVenue();
    ui->comboBoxVenue->clear();
    int selItem = 0;
    for (int i=0; i < venues.size(); i++)
    {
        ui->comboBoxVenue->addItem(venues.at(i).name, venues.at(i).venueId);
        if (venues.at(i).venueId == venue)
        {
            selItem = i;
        }
    }
    ui->comboBoxVenue->setCurrentIndex(selItem);
    settings.setRequestServerVenue(ui->comboBoxVenue->itemData(selItem).toInt());
    ui->checkBoxAccepting->setChecked(songbookApi->getAccepting());
}

void DlgRequests::on_pushButtonUpdateDb_clicked()
{
    ui->pushButtonUpdateDb->setEnabled(false);
    QMessageBox msgBox;
    msgBox.setText(tr("Are you sure?\n\nThis operation can take serveral minutes depending on the size of your song database and the speed of your internet connection.\n"));
//    msgBox.setInformativeText(tr("This operation can take serveral minutes depending on the size of your song database and the speed of your internet connection."));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();
    if (ret == QMessageBox::Yes)
    {
        qInfo() << "Opening progress dialog for remote db update";
        QProgressDialog *progressDialog = new QProgressDialog(this);
//        progressDialog->setCancelButton(0);
        progressDialog->setMinimum(0);
        progressDialog->setMaximum(20);
        progressDialog->setValue(0);
        progressDialog->setLabelText(tr("Updating request server song database"));
        progressDialog->show();
        QApplication::processEvents();
        connect(songbookApi, SIGNAL(remoteSongDbUpdateNumDocs(int)), progressDialog, SLOT(setMaximum(int)));
        connect(songbookApi, SIGNAL(remoteSongDbUpdateProgress(int)), progressDialog, SLOT(setValue(int)));
        connect(progressDialog, SIGNAL(canceled()), songbookApi, SLOT(dbUpdateCanceled()));
        //    progressDialog->show();
        songbookApi->updateSongDb();
        if (songbookApi->updateWasCancelled())
            qInfo() << "Songbook DB update cancelled by user";
        else
        {
            QMessageBox msgBox;
            msgBox.setText(tr("Remote database update completed!"));
            msgBox.exec();
        }
        qInfo() << "Closing progress dialog for remote db update";
        progressDialog->close();
        progressDialog->deleteLater();
    }
    ui->pushButtonUpdateDb->setEnabled(true);
}

void DlgRequests::on_comboBoxVenue_activated(int index)
{
    int venue = ui->comboBoxVenue->itemData(index).toInt();
    settings.setRequestServerVenue(venue);
    songbookApi->refreshRequests();
    ui->checkBoxAccepting->setChecked(songbookApi->getAccepting());
    qInfo() << "Set venue_id to " << venue;
    qInfo() << "Settings now reporting venue as " << settings.requestServerVenue();
}

void DlgRequests::previewCdg()
{
    DlgVideoPreview *videoPreviewDialog = new DlgVideoPreview(rtClickFile, this);
    videoPreviewDialog->setAttribute(Qt::WA_DeleteOnClose);
    videoPreviewDialog->show();
}

void DlgRequests::on_lineEditSearch_textChanged(const QString &arg1)
{
    static QString lastVal;
    if (arg1.trimmed() != lastVal)
    {
        dbModel.search(arg1);
        lastVal = arg1.trimmed();
    }
}

void DlgRequests::lineEditSearchEscapePressed()
{
    QModelIndex index;
    index = ui->tableViewRequests->selectionModel()->selectedIndexes().at(0);
    QString filterStr = index.sibling(index.row(),2).data().toString() + " " + index.sibling(index.row(),3).data().toString();
    dbModel.search(filterStr);
    ui->lineEditSearch->setText(filterStr);
}

void DlgRequests::autoSizeViews()
{
    int fH = QFontMetrics(settings.applicationFont()).height();
#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
    int durationColSize = QFontMetrics(settings.applicationFont()).horizontalAdvance(tr(" Duration "));
    int songidColSize = QFontMetrics(settings.applicationFont()).horizontalAdvance(" AA0000000-0000 ");
#else
    int durationColSize = QFontMetrics(settings.applicationFont()).width(tr(" Duration "));
    int songidColSize = QFontMetrics(settings.applicationFont()).width(" AA0000000-0000 ");
#endif
    int remainingSpace = ui->tableViewSearch->width() - durationColSize - songidColSize - 12;
    int artistColSize = (remainingSpace / 2) - 12;
    int titleColSize = (remainingSpace / 2);
    ui->tableViewSearch->hideColumn(TableModelKaraokeSongs::COL_ID);
    ui->tableViewSearch->hideColumn(TableModelKaraokeSongs::COL_FILENAME);
    ui->tableViewSearch->horizontalHeader()->resizeSection(TableModelKaraokeSongs::COL_ARTIST, artistColSize);
    ui->tableViewSearch->horizontalHeader()->resizeSection(TableModelKaraokeSongs::COL_TITLE, titleColSize);
    ui->tableViewSearch->horizontalHeader()->resizeSection(TableModelKaraokeSongs::COL_DURATION, durationColSize);
    ui->tableViewSearch->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_DURATION, QHeaderView::Fixed);
    ui->tableViewSearch->horizontalHeader()->resizeSection(TableModelKaraokeSongs::COL_SONGID, songidColSize);
#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
    int tsWidth = QFontMetrics(settings.applicationFont()).horizontalAdvance(" 00/00/00 00:00 xx ");
    int keyWidth = QFontMetrics(settings.applicationFont()).horizontalAdvance("_Key_");
    int singerColSize = QFontMetrics(settings.applicationFont()).horizontalAdvance("_Isaac_Lightburn_");
#else
    int tsWidth = QFontMetrics(settings.applicationFont()).width(" 00/00/00 00:00 xx ");
    int keyWidth = QFontMetrics(settings.applicationFont()).width("_Key_");
    int singerColSize = QFontMetrics(settings.applicationFont()).width("_Isaac_Lightburn_");
#endif
    qInfo() << "tsWidth = " << tsWidth;
    int delwidth = fH * 2;
    qInfo() << "singerColSize = " << singerColSize;
    remainingSpace = ui->tableViewRequests->width() - tsWidth - delwidth - singerColSize - keyWidth - 10;
    artistColSize = remainingSpace / 2;
    titleColSize = remainingSpace / 2;
    ui->tableViewRequests->horizontalHeader()->resizeSection(0, singerColSize);
    ui->tableViewRequests->horizontalHeader()->resizeSection(1, artistColSize);
    ui->tableViewRequests->horizontalHeader()->resizeSection(2, titleColSize);
    ui->tableViewRequests->horizontalHeader()->resizeSection(3, tsWidth);
    ui->tableViewRequests->horizontalHeader()->resizeSection(4, keyWidth);
    ui->tableViewRequests->horizontalHeader()->resizeSection(5, delwidth);
    ui->tableViewRequests->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
}


void DlgRequests::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    autoSizeViews();
}


void DlgRequests::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    autoSizeViews();
}

void DlgRequests::on_spinBoxKey_valueChanged(int arg1)
{
    if (arg1 > 0)
        ui->spinBoxKey->setPrefix("+");
    else
        ui->spinBoxKey->setPrefix("");
}

void DlgRequests::on_pushButtonWebSearch_clicked()
{
    QString link = "http://db.openkj.org/?type=All&searchstr=" + ui->lineEditSearch->text();
    QDesktopServices::openUrl(QUrl(link));
}


void DlgRequests::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

void DlgRequests::on_pushButtonRunTortureTest_clicked()
{
    connect(&testTimer, &QTimer::timeout, [&] () {
        qInfo() << "Triggering test songbook add";
       songbookApi->triggerTestAdd();
    });
    testTimer.start(3000);
}

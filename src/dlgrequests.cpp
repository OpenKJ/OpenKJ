/*
 * Copyright (c) 2013-2021 Thomas Isaac Lightburn
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
#include "okjsongbookapi.h"
#include <QProgressDialog>
#include "src/models/tableviewtooltipfilter.h"
#include "dlgvideopreview.h"

#include <spdlog/sinks/basic_file_sink.h>


QString toMixedCase(const QString &s) {
    if (s.isNull())
        return {};
    if (s.size() < 1)
        return {};
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    QStringList parts = s.split(" ", QString::SkipEmptyParts);
#else
    QStringList parts = s.split(' ', Qt::SkipEmptyParts);
#endif
    for (int i = 1; i < parts.size(); ++i)
        parts[i].replace(0, 1, parts[i][0].toUpper());
    QString newStr = parts.join(" ");
    newStr.replace(0, 1, newStr.at(0).toUpper());
    return newStr;
}

DlgRequests::DlgRequests(TableModelRotation &rotationModel, OKJSongbookAPI &songbookAPI, QWidget *parent) :
        QDialog(parent),
        rotModel(rotationModel),
        songbookApi(songbookAPI),
        ui(new Ui::DlgRequests) {
    QString logDir = m_settings.logDir();
    QDir dir;
    dir.mkpath(logDir);
    QString logFilePath;
    QString filename = "openkj-requests-" + QDateTime::currentDateTime().toString("yyyy-MM-dd") + ".log";
    logFilePath = logDir + QDir::separator() + filename;
    m_reqLogger = spdlog::basic_logger_mt("requests", logFilePath.toStdString());
    m_reqLogger->set_level(spdlog::level::info);
    m_reqLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] | %v");
    m_reqLogger->info("New logging session starting up");
    m_reqLogger->flush();
    curRequestId = -1;
    ui->setupUi(this);
    requestsModel = new TableModelRequests(songbookApi, this);
    dbModel.loadData();
    ui->tableViewRequests->setModel(requestsModel);
    ui->tableViewRequests->viewport()->installEventFilter(new TableViewToolTipFilter(ui->tableViewRequests));
    connect(requestsModel, &TableModelRequests::layoutChanged, this, &DlgRequests::requestsModified);
    ui->tableViewSearch->setModel(&dbModel);
    ui->tableViewSearch->viewport()->installEventFilter(new TableViewToolTipFilter(ui->tableViewSearch));
    ui->groupBoxAddSong->setDisabled(true);
    ui->groupBoxSongDb->setDisabled(true);
    connect(ui->tableViewRequests->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &DlgRequests::requestSelectionChanged);
    connect(ui->tableViewSearch->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &DlgRequests::songSelectionChanged);
    connect(&songbookApi, &OKJSongbookAPI::synchronized, this, &DlgRequests::updateReceived);
    connect(&songbookApi, &OKJSongbookAPI::sslError, this, &DlgRequests::sslError);
    connect(&songbookApi, &OKJSongbookAPI::delayError, this, &DlgRequests::delayError);
    ui->comboBoxAddPosition->setEnabled(false);
    ui->comboBoxSingers->setEnabled(true);
    ui->lineEditSingerName->setEnabled(false);
    ui->labelAddPos->setEnabled(false);
    QStringList posOptions;
    posOptions << tr("Fair (full rotation)");
    posOptions << tr("Bottom of rotation");
    posOptions << tr("After current singer");
    ui->comboBoxAddPosition->addItems(posOptions);
    ui->comboBoxAddPosition->setCurrentIndex(m_settings.lastSingerAddPositionType());
    connect(ui->comboBoxAddPosition, qOverload<int>(&QComboBox::currentIndexChanged), &m_settings,
            &Settings::setLastSingerAddPositionType);
    ui->tableViewSearch->hideColumn(TableModelKaraokeSongs::COL_ID);
    ui->tableViewSearch->hideColumn(TableModelKaraokeSongs::COL_FILENAME);
    ui->tableViewSearch->horizontalHeader()->resizeSection(4, 75);
    ui->checkBoxDelOnAdd->setChecked(m_settings.requestRemoveOnRotAdd());
    connect(&songbookApi, &OKJSongbookAPI::venuesChanged, this, &DlgRequests::venuesChanged);
    connect(ui->lineEditSearch, &CustomLineEdit::escapePressed, this, &DlgRequests::lineEditSearchEscapePressed);
    connect(ui->checkBoxDelOnAdd, &QCheckBox::clicked, &m_settings, &Settings::setRequestRemoveOnRotAdd);
    ui->cbxAutoShowRequestsDlg->setChecked(m_settings.requestDialogAutoShow());
    connect(ui->cbxAutoShowRequestsDlg, &QCheckBox::clicked, &m_settings, &Settings::setRequestDialogAutoShow);

    QFontMetrics fm(m_settings.applicationFont());
    QSize mcbSize(fm.height(), fm.height());
    ui->spinBoxKey->setMaximum(12);
    ui->spinBoxKey->setMinimum(-12);
    autoSizeViews();
    if (!m_settings.testingEnabled())
        ui->pushButtonRunTortureTest->hide();
    connect(&songbookApi, &OKJSongbookAPI::requestsChanged, this, &DlgRequests::requestsChanged);

}

int DlgRequests::numRequests() {
    return requestsModel->count();
}

DlgRequests::~DlgRequests() {

    delete ui;
}

void DlgRequests::databaseAboutToUpdate() {

}

void DlgRequests::databaseUpdateComplete() {
    dbModel.loadData();
    autoSizeViews();
}

void DlgRequests::databaseSongAdded() {
    dbModel.loadData();
}

void DlgRequests::rotationChanged() {
    QString curSelSinger = ui->comboBoxSingers->currentText();
    ui->comboBoxSingers->clear();
    QStringList singers = rotModel.singers();
    singers.sort(Qt::CaseInsensitive);
    ui->comboBoxSingers->addItems(singers);
    if (singers.contains(curSelSinger))
        ui->comboBoxSingers->setCurrentText(curSelSinger);
    int s = -1;
    for (int i = 0; i < singers.size(); i++) {
        if (singers.at(i).toLower().trimmed() == curSelReqSinger.toLower().trimmed()) {
            s = i;
            break;
        }
    }
    if (s != -1) {
        ui->comboBoxSingers->setCurrentIndex(s);
        ui->radioButtonExistingSinger->setChecked(true);
    }

}

void DlgRequests::updateIcons() {
    QString thm = (m_settings.theme() == 1) ? ":/theme/Icons/okjbreeze-dark/" : ":/theme/Icons/okjbreeze/";
    ui->buttonRefresh->setIcon(QIcon(thm + "actions/22/view-refresh.svg"));
    ui->pushButtonClearReqs->setIcon(QIcon(thm + "actions/22/edit-clear-all.svg"));
    ui->pushButtonSearch->setIcon(QIcon(thm + "actions/22/edit-find.svg"));
    ui->pushButtonWebSearch->setIcon(QIcon(thm + "apps/48/internet-web-browser.svg"));
}

void DlgRequests::on_pushButtonClose_clicked() {
    close();
}

void DlgRequests::requestsModified() {
    static int testruns = 0;
    if ((requestsModel->count() > 0) && (m_settings.requestDialogAutoShow())) {
        this->show();
        this->raise();
    }
    autoSizeViews();
    if (requestsModel->count() > 0 && m_settings.testingEnabled() && testTimer.isActive()) {
        ui->tableViewRequests->selectRow(0);
        QApplication::processEvents();
        ui->tableViewSearch->selectRow(0);
        QApplication::processEvents();
        on_pushButtonAddSong_clicked();
        songbookApi.removeRequest(requestsModel->requests().at(0).requestId());
        if (rotModel.singerCount() > 5)
            rotModel.singerDelete(rotModel.getSingerAtPosition(0).id);
        ui->pushButtonRunTortureTest->setText("Running Torture Test (" + QString::number(++testruns) + ")");
    }
}

void DlgRequests::on_pushButtonSearch_clicked() {
    dbModel.search(ui->lineEditSearch->text());
}

void DlgRequests::on_lineEditSearch_returnPressed() {
    dbModel.search(ui->lineEditSearch->text());
}

void DlgRequests::requestSelectionChanged(const QItemSelection &current, const QItemSelection &previous) {
    ui->tableViewSearch->clearSelection();
    ui->groupBoxAddSong->setDisabled(true);
    if (current.indexes().size() == 0) {
        dbModel.search("yeahjustsomethingitllneverfind.imlazylikethat");
        ui->groupBoxSongDb->setDisabled(true);
        ui->comboBoxSingers->setCurrentIndex(0);
        ui->spinBoxKey->setValue(0);
        curSelReqSinger = "";
        return;
    }
    QModelIndex index = current.indexes().at(0);
    Q_UNUSED(previous);
    if ((index.isValid()) && (ui->tableViewRequests->selectionModel()->selectedIndexes().size() > 0)) {
        ui->groupBoxSongDb->setEnabled(true);
        ui->comboBoxSingers->clear();
        QString singerName = index.sibling(index.row(), 0).data().toString();
        curSelReqSinger = singerName;
        QStringList singers = rotModel.singers();
        singers.sort(Qt::CaseInsensitive);
        ui->comboBoxSingers->addItems(singers);

        QString filterStr =
                index.sibling(index.row(), 1).data().toString() + " " + index.sibling(index.row(), 2).data().toString();
        dbModel.search(filterStr);
        ui->lineEditSearch->setText(filterStr);
        //ui->lineEditSingerName->setText(singerName);
        ui->lineEditSingerName->setText(toMixedCase(singerName));
        ui->spinBoxKey->setValue(requestsModel->requests().at(index.row()).key());

        int s = -1;
        for (int i = 0; i < singers.size(); i++) {
            if (singers.at(i).toLower().trimmed() == singerName.toLower().trimmed()) {
                s = i;
                break;
            }
        }
        if (s != -1) {
            ui->comboBoxSingers->setCurrentIndex(s);
            ui->radioButtonExistingSinger->setChecked(true);
        } else {
            ui->radioButtonNewSinger->setChecked(true);
        }
    } else {
        dbModel.search("yeahjustsomethingitllneverfind.imlazylikethat");
    }
}

void DlgRequests::songSelectionChanged(const QItemSelection &current, const QItemSelection &previous) {
    Q_UNUSED(previous);
    if (current.indexes().size() == 0) {
        ui->groupBoxAddSong->setDisabled(true);
    } else
        ui->groupBoxAddSong->setEnabled(true);
}

void DlgRequests::on_radioButtonExistingSinger_toggled(bool checked) {
    ui->comboBoxAddPosition->setEnabled(!checked);
    ui->comboBoxSingers->setEnabled(checked);
    ui->lineEditSingerName->setEnabled(!checked);
    ui->labelAddPos->setEnabled(!checked);
}

void DlgRequests::on_pushButtonClearReqs_clicked() {
    QMessageBox msgBox;
    msgBox.setText("Are you sure?");
    msgBox.setInformativeText("This action will clear all received requests. This operation can not be undone.");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.addButton(QMessageBox::Cancel);
    QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
    msgBox.exec();
    if (msgBox.clickedButton() == yesButton) {
        ui->tableViewRequests->selectionModel()->clearSelection();
        ui->lineEditSearch->clear();
        ui->lineEditSingerName->clear();
        ui->comboBoxSingers->clear();
        ui->radioButtonExistingSinger->setChecked(true);
        songbookApi.clearRequests();
        m_reqLogger->info("All requests cleared by host");
        m_reqLogger->flush();
    }
}

void DlgRequests::on_tableViewRequests_clicked(const QModelIndex &index) {
    if (index.column() == 5) {
        songbookApi.removeRequest(index.data(Qt::UserRole).toInt());
        m_reqLogger->info("RequestID: {} | Manually removed by host", index.data(Qt::UserRole).toInt());
        m_reqLogger->flush();
        ui->tableViewRequests->selectionModel()->clearSelection();
        ui->lineEditSearch->clear();
        ui->lineEditSingerName->clear();
        ui->comboBoxSingers->clear();
        ui->spinBoxKey->setValue(0);
        ui->radioButtonExistingSinger->setChecked(true);
    } else {
        curRequestId = index.data(Qt::UserRole).toInt();
    }
}

void DlgRequests::on_pushButtonAddSong_clicked() {
    if (ui->tableViewRequests->selectionModel()->selectedIndexes().empty() ||
        ui->tableViewSearch->selectionModel()->selectedIndexes().empty())
        return;
    auto song = qvariant_cast<std::shared_ptr<okj::KaraokeSong>>(
            ui->tableViewSearch->selectionModel()->selectedIndexes().at(0).data(Qt::UserRole)
    );
    int keyChg = ui->spinBoxKey->value();
    if (ui->radioButtonNewSinger->isChecked()) {
        if (ui->lineEditSingerName->text() == "" || rotModel.singerExists(ui->lineEditSingerName->text()))
            return;
        int newSingerId = rotModel.singerAdd(ui->lineEditSingerName->text(),
                                             ui->comboBoxAddPosition->currentIndex());
        emit addRequestSong(song->id, newSingerId, keyChg);
        m_reqLogger->info(
                "RequestID: {} | Added to new singer | Name: {} | Position: {} | Wait: {} | Song: {} - {} - {} | Key: {}",
                curRequestId,
                ui->lineEditSingerName->text().toStdString(),
                rotModel.getSinger(newSingerId).position,
                rotModel.singerTurnDistance(newSingerId),
                song->songid,
                song->artist,
                song->title,
                keyChg
        );
        m_reqLogger->flush();
    } else if (ui->radioButtonExistingSinger->isChecked()) {
        emit addRequestSong(song->id, rotModel.getSingerByName(ui->comboBoxSingers->currentText()).id, keyChg);
        m_reqLogger->info("RequestID: {} | Added to existing singer | Name: {} | Song: {} - {} - {} | Key: {}",
                          curRequestId,
                          ui->comboBoxSingers->currentText().toStdString(),
                          song->songid.toStdString(),
                          song->artist.toStdString(),
                          song->title.toStdString(),
                          keyChg
        );
        m_reqLogger->flush();
    }
    if (m_settings.requestRemoveOnRotAdd()) {
        songbookApi.removeRequest(curRequestId);
        m_reqLogger->info("RequestID: {} | Auto-removed after add to singer queue", curRequestId);
        m_reqLogger->flush();
        ui->tableViewRequests->selectionModel()->clearSelection();
        ui->lineEditSearch->clear();
        ui->lineEditSingerName->clear();
        ui->comboBoxSingers->clear();
        ui->radioButtonExistingSinger->setChecked(true);
    }
}

void DlgRequests::on_tableViewSearch_customContextMenuRequested(const QPoint &pos) {
    QModelIndex index = ui->tableViewSearch->indexAt(pos);
    if (!index.isValid())
        return;
    auto song = qvariant_cast<std::shared_ptr<okj::KaraokeSong>>(index.data(Qt::UserRole));
    QMenu contextMenu(this);
    contextMenu.addAction(tr("Preview"), [&] () { previewCdg(song); });
    contextMenu.exec(QCursor::pos());
}

void DlgRequests::updateReceived(QTime updateTime) {
    ui->labelLastUpdate->setText(updateTime.toString("hh:mm:ss AP"));
}

void DlgRequests::on_buttonRefresh_clicked() {
    songbookApi.refreshRequests();
    songbookApi.refreshVenues();
}

void DlgRequests::sslError() {
    QMessageBox::warning(this, tr("SSL Handshake Error"),
                         tr("An error was encountered while establishing a secure connection to the requests server.  This is usually caused by an invalid or self-signed cert on the server.  You can set the requests client to ignore SSL errors in the network settings dialog."));
}

void DlgRequests::delayError(int seconds) {
    QMessageBox::warning(this, tr("Possible Connectivity Issue"), tr("It has been ") + QString::number(seconds) +
                                                                  tr(" seconds since we last received a response from the requests server.  You may be missing new submitted requests.  Please ensure that your network connection is up and working."));
}

void DlgRequests::on_checkBoxAccepting_clicked(bool checked) {
    songbookApi.setAccepting(checked);
}

void DlgRequests::venuesChanged(OkjsVenues venues) {
    int venue = m_settings.requestServerVenue();
    ui->comboBoxVenue->clear();
    int selItem = 0;
    for (int i = 0; i < venues.size(); i++) {
        ui->comboBoxVenue->addItem(venues.at(i).name, venues.at(i).venueId);
        if (venues.at(i).venueId == venue) {
            selItem = i;
        }
    }
    ui->comboBoxVenue->setCurrentIndex(selItem);
    m_settings.setRequestServerVenue(ui->comboBoxVenue->itemData(selItem).toInt());
    ui->checkBoxAccepting->setChecked(songbookApi.getAccepting());
}

void DlgRequests::on_pushButtonUpdateDb_clicked() {
    ui->pushButtonUpdateDb->setEnabled(false);
    QMessageBox msgBox;
    msgBox.setText(
            tr("Are you sure?\n\nThis operation can take serveral minutes depending on the size of your song database and the speed of your internet connection.\n"));
//    msgBox.setInformativeText(tr("This operation can take serveral minutes depending on the size of your song database and the speed of your internet connection."));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();
    if (ret == QMessageBox::Yes) {
        qInfo() << "Opening progress dialog for remote db update";
        QProgressDialog *progressDialog = new QProgressDialog(this);
//        progressDialog->setCancelButton(0);
        progressDialog->setMinimum(0);
        progressDialog->setMaximum(20);
        progressDialog->setValue(0);
        progressDialog->setLabelText(tr("Updating request server song database"));
        progressDialog->show();
        QApplication::processEvents();
        connect(&songbookApi, &OKJSongbookAPI::remoteSongDbUpdateNumDocs, progressDialog, &QProgressDialog::setMaximum);
        connect(&songbookApi, &OKJSongbookAPI::remoteSongDbUpdateProgress, progressDialog, &QProgressDialog::setValue);
        connect(progressDialog, &QProgressDialog::canceled, &songbookApi, &OKJSongbookAPI::dbUpdateCanceled);
        //    progressDialog->show();
        songbookApi.updateSongDb();
        if (songbookApi.updateWasCancelled())
            qInfo() << "Songbook DB update cancelled by user";
        else {
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

void DlgRequests::on_comboBoxVenue_activated(int index) {
    int venue = ui->comboBoxVenue->itemData(index).toInt();
    m_settings.setRequestServerVenue(venue);
    songbookApi.refreshRequests();
    ui->checkBoxAccepting->setChecked(songbookApi.getAccepting());
}

void DlgRequests::previewCdg(const std::shared_ptr<okj::KaraokeSong>& song) {
    auto videoPreviewDialog = new DlgVideoPreview(song->path, this);
    videoPreviewDialog->setAttribute(Qt::WA_DeleteOnClose);
    videoPreviewDialog->show();
}

void DlgRequests::on_lineEditSearch_textChanged(const QString &arg1) {
    static QString lastVal;
    if (arg1.trimmed() != lastVal) {
        dbModel.search(arg1);
        lastVal = arg1.trimmed();
    }
}

void DlgRequests::lineEditSearchEscapePressed() {
    QModelIndex index;
    index = ui->tableViewRequests->selectionModel()->selectedIndexes().at(0);
    QString filterStr =
            index.sibling(index.row(), 2).data().toString() + " " + index.sibling(index.row(), 3).data().toString();
    dbModel.search(filterStr);
    ui->lineEditSearch->setText(filterStr);
}

void DlgRequests::autoSizeViews() {
    int fH = QFontMetrics(m_settings.applicationFont()).height();
    ui->tableViewSearch->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_LASTPLAY, QHeaderView::ResizeToContents);
    ui->tableViewSearch->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_PLAYS, QHeaderView::ResizeToContents);
    ui->tableViewSearch->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_DURATION, QHeaderView::ResizeToContents);
    ui->tableViewSearch->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_ARTIST, QHeaderView::Stretch);
    ui->tableViewSearch->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_TITLE, QHeaderView::Stretch);
    ui->tableViewSearch->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_SONGID, QHeaderView::ResizeToContents);
    QApplication::processEvents();
    ui->tableViewSearch->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_ARTIST, QHeaderView::Interactive);
    ui->tableViewSearch->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_TITLE, QHeaderView::Interactive);
    ui->tableViewSearch->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_SONGID, QHeaderView::Interactive);
    ui->tableViewSearch->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_LASTPLAY, QHeaderView::Interactive);
    ui->tableViewSearch->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_PLAYS, QHeaderView::Interactive);
    ui->tableViewSearch->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_DURATION, QHeaderView::Interactive);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    int tsWidth = QFontMetrics(m_settings.applicationFont()).horizontalAdvance(" 00/00/00 00:00 xx ");
    int keyWidth = QFontMetrics(m_settings.applicationFont()).horizontalAdvance("_Key_");
    int singerColSize = QFontMetrics(m_settings.applicationFont()).horizontalAdvance("_Isaac_Lightburn_");
#else
    int tsWidth = QFontMetrics(settings.applicationFont()).width(" 00/00/00 00:00 xx ");
    int keyWidth = QFontMetrics(settings.applicationFont()).width("_Key_");
    int singerColSize = QFontMetrics(settings.applicationFont()).width("_Isaac_Lightburn_");
#endif
    qInfo() << "tsWidth = " << tsWidth;
    int delwidth = fH * 2;
    qInfo() << "singerColSize = " << singerColSize;
    int remainingSpace = ui->tableViewRequests->width() - tsWidth - delwidth - singerColSize - keyWidth - 10;
    int artistColSize = remainingSpace / 2;
    int titleColSize = remainingSpace / 2;
    ui->tableViewRequests->horizontalHeader()->resizeSection(0, singerColSize);
    ui->tableViewRequests->horizontalHeader()->resizeSection(1, artistColSize);
    ui->tableViewRequests->horizontalHeader()->resizeSection(2, titleColSize);
    ui->tableViewRequests->horizontalHeader()->resizeSection(3, tsWidth);
    ui->tableViewRequests->horizontalHeader()->resizeSection(4, keyWidth);
    ui->tableViewRequests->horizontalHeader()->resizeSection(5, delwidth);
    ui->tableViewRequests->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
}


void DlgRequests::resizeEvent(QResizeEvent *event) {
    QDialog::resizeEvent(event);
    autoSizeViews();
}


void DlgRequests::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    autoSizeViews();
    ui->comboBoxAddPosition->setCurrentIndex(m_settings.lastSingerAddPositionType());
}

void DlgRequests::on_spinBoxKey_valueChanged(int arg1) {
    if (arg1 > 0)
        ui->spinBoxKey->setPrefix("+");
    else
        ui->spinBoxKey->setPrefix("");
}

void DlgRequests::on_pushButtonWebSearch_clicked() {
    QString link = "http://db.openkj.org/?type=All&searchstr=" + ui->lineEditSearch->text();
    QDesktopServices::openUrl(QUrl(link));
}


void DlgRequests::closeEvent(QCloseEvent *event) {
    hide();
    event->ignore();
}

void DlgRequests::on_pushButtonRunTortureTest_clicked() {
    connect(&testTimer, &QTimer::timeout, [&]() {
        qInfo() << "Triggering test songbook add";
        songbookApi.triggerTestAdd();
    });
    testTimer.start(3000);
}

void DlgRequests::requestsChanged(OkjsRequests requests) {
    std::vector<int> curRequestsList;
    for (OkjsRequest &request : requests) {
        curRequestsList.push_back(request.requestId);
        auto result = std::find(m_prevRequestList.begin(), m_prevRequestList.end(), request.requestId);
        if (result == m_prevRequestList.end()) {
            m_reqLogger->info("RequestID: {} | Received new request |  Singer: {} | Submitted: {} | Song: {} - {}",
                              request.requestId,
                              request.singer.toStdString(),
                              QDateTime::fromSecsSinceEpoch(request.time).toString(Qt::ISODateWithMs).toStdString(),
                              request.artist.toStdString(),
                              request.title.toStdString()
            );
            m_reqLogger->flush();

        }
    }
    m_prevRequestList = curRequestsList;
}

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

#ifndef KHREQUESTSDIALOG_H
#define KHREQUESTSDIALOG_H

#include <QDialog>
#include <QItemSelection>
#include "src/models/tablemodelrequests.h"
#include "src/models/tablemodelkaraokesongs.h"
#include "src/models/tablemodelrotation.h"
#include "okjsongbookapi.h"
#include <spdlog/spdlog.h>

namespace Ui {
class DlgRequests;
}

class DlgRequests : public QDialog
{
    Q_OBJECT

private:
    Ui::DlgRequests *ui;
    TableModelRequests *requestsModel;
    //SongDBTableModel *songDbModel;
    TableModelKaraokeSongs dbModel;
    TableModelRotation *rotModel;
    QString rtClickFile;
    int curRequestId;
    QString curSelReqSinger;
    QTimer testTimer;
    std::shared_ptr<spdlog::logger> m_reqLogger;
    std::vector<int> m_prevRequestList;


public:
    explicit DlgRequests(TableModelRotation *rotationModel, QWidget *parent = 0);
    int numRequests();
    ~DlgRequests();

signals:
    void addRequestSong(int songId, int singerId, int keyChg);

public slots:
    void databaseAboutToUpdate();
    void databaseUpdateComplete();
    void databaseSongAdded();
    void rotationChanged();
    void updateIcons();

private slots:
    void on_pushButtonClose_clicked();
    void requestsModified();
    void requestsChanged(OkjsRequests requests);
    void on_pushButtonSearch_clicked();
    void on_lineEditSearch_returnPressed();
    void requestSelectionChanged(const QItemSelection & current, const QItemSelection & previous);
    void songSelectionChanged(const QItemSelection & current, const QItemSelection & previous);
    void on_radioButtonExistingSinger_toggled(bool checked);
    void on_pushButtonClearReqs_clicked();
    void on_tableViewRequests_clicked(const QModelIndex &index);
    void on_pushButtonAddSong_clicked();
    void on_tableViewSearch_customContextMenuRequested(const QPoint &pos);
    void updateReceived(QTime updateTime);
    void on_buttonRefresh_clicked();
    void sslError();
    void delayError(int seconds);
    void on_checkBoxAccepting_clicked(bool checked);
    void venuesChanged(OkjsVenues venues);
    void on_pushButtonUpdateDb_clicked();
    void on_comboBoxVenue_activated(int index);
    void previewCdg();
    void on_lineEditSearch_textChanged(const QString &arg1);
    void lineEditSearchEscapePressed();
    void autoSizeViews();

    // QWidget interface
    void on_spinBoxKey_valueChanged(int arg1);

    void on_pushButtonWebSearch_clicked();

    void on_pushButtonRunTortureTest_clicked();

protected:
    void resizeEvent(QResizeEvent *event);

    // QWidget interface
protected:
    void showEvent(QShowEvent *event);

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event);
};

#endif // KHREQUESTSDIALOG_H

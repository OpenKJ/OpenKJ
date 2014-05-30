/*
 * Copyright (c) 2013-2014 Thomas Isaac Lightburn
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

#ifndef DATABASEDIALOG_H
#define DATABASEDIALOG_H

#include <QDialog>
#include <QDir>
#include <QDirIterator>
#include "sourcedirtablemodel.h"
#include "dlgdurationscan.h"
#include "khsong.h"


namespace Ui {
class DlgDatabase;
}

class DlgDatabase : public QDialog
{
    Q_OBJECT
    
public:
    explicit DlgDatabase(KhSongs *songs, QWidget *parent = 0);
    ~DlgDatabase();

signals:
    void databaseUpdated();
    void databaseCleared();

private slots:
    void on_buttonUpdateAll_clicked();

    void on_buttonNew_clicked();

    void on_buttonClose_clicked();

    void on_buttonDelete_clicked();

    void on_treeViewFolders_clicked(const QModelIndex &index);

    void on_buttonUpdate_clicked();

    void on_btnClearDatabase_clicked();

    void dbupdate_thread_finished();

    void on_pushButtonGetDurations_clicked();

private:
    Ui::DlgDatabase *ui;
    SourceDirTableModel *sourcedirmodel;
    DlgDurationScan *dlgDurationScan;
    int selectedRow;
};

#endif // DATABASEDIALOG_H

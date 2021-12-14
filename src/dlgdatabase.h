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

#ifndef DATABASEDIALOG_H
#define DATABASEDIALOG_H

#include <QDialog>
#include <QDir>
#include <QDirIterator>
#include "src/models/tablemodelkaraokesourcedirs.h"
#include "dlgcustompatterns.h"
#include <QSqlDatabase>
#include "dlgdbupdate.h"
#include "models/tablemodelkaraokesongs.h"
#include "directorymonitor.h"
#include "settings.h"

namespace Ui {
class DlgDatabase;
}

class DlgDatabase : public QDialog
{
    Q_OBJECT
    
private:
    Ui::DlgDatabase *ui;
    TableModelKaraokeSourceDirs *sourcedirmodel;
    DlgCustomPatterns *customPatternsDlg;
    TableModelKaraokeSongs &m_dbModel;
    DlgDbUpdate *dbUpdateDlg;
    Settings m_settings;
    DirectoryMonitor *m_directoryMonitor {nullptr};

    void scan(bool scanAllPaths);
    void updateButtonsState();

public:
    explicit DlgDatabase(TableModelKaraokeSongs &dbModel, QWidget *parent = nullptr);
    ~DlgDatabase() override;

signals:
    void databaseAboutToUpdate();
    void databaseUpdateComplete();
    void databaseCleared();
    void databaseSongAdded();

public slots:
    void singleSongAdd(const QString &path);

private slots:
    void on_buttonUpdateAll_clicked();
    void on_buttonNew_clicked();
    void on_buttonClose_clicked();
    void on_buttonDelete_clicked();
    void on_buttonUpdate_clicked();
    void on_btnClearDatabase_clicked();
    static void showDbUpdateErrors(const QStringList& errors);
    void on_btnCustomPatterns_clicked();
    void on_btnExport_clicked();
    void on_foldersSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
};

#endif // DATABASEDIALOG_H

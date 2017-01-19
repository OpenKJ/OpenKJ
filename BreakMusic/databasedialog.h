/*
 * Copyright (c) 2013-2017 Thomas Isaac Lightburn
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
#include <QSqlTableModel>
#include <QSqlDatabase>

namespace Ui {
class DatabaseDialog;
}

class DatabaseDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit DatabaseDialog(QSqlDatabase *db, QWidget *parent = 0);
    ~DatabaseDialog();
    
private slots:
    void on_pushButtonAdd_clicked();

    void on_pushButtonUpdateAll_clicked();

    void on_pushButtonClose_clicked();

    void on_pushButtonClearDb_clicked();

    void on_pushButtonDelete_clicked();

    void on_tableViewPaths_clicked(const QModelIndex &index);

    void on_pushButtonUpdate_clicked();

signals:
    void dbUpdated();
    void dbCleared();

private:
    Ui::DatabaseDialog *ui;
    QSqlTableModel *pathsModel;
    int selectedDirectoryIdx;
    QSqlDatabase *m_db;

};

#endif // DATABASEDIALOG_H

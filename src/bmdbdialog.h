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

#ifndef BMDBDIALOG_H
#define BMDBDIALOG_H

#include <QDialog>
#include <QSqlTableModel>
#include <QSqlDatabase>
#include "dlgdbupdate.h"


namespace Ui {
class BmDbDialog;
}

class BmDbDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit BmDbDialog(QWidget *parent = nullptr);
    ~BmDbDialog() override;

private slots:
    void pushButtonAddClicked();
    void pushButtonUpdateAllClicked();
    void pushButtonClearDbClicked();
    void pushButtonDeleteClicked();
    void pushButtonUpdateClicked();

signals:
    void bmDbAboutToUpdate();
    void bmDbUpdated();
    void bmDbCleared();

private:
    std::unique_ptr<Ui::BmDbDialog> ui;
    QSqlTableModel m_pathsModel;
    DlgDbUpdate m_dbUpdateDlg{this};
};

#endif // BMDBDIALOG_H

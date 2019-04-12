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

#ifndef REGULAREXPORTDIALOG_H
#define REGULAREXPORTDIALOG_H

#include <QDialog>
#include <QSqlTableModel>
#include "rotationmodel.h"

namespace Ui {
class DlgRegularExport;
}

class DlgRegularExport : public QDialog
{
    Q_OBJECT

private:
    Ui::DlgRegularExport *ui;
    QSqlTableModel *regModel;
    void exportSingers(QList<int> regSingerIds, QString savePath);
    RotationModel *rotModel;

public:
    explicit DlgRegularExport(RotationModel *rotationModel, QWidget *parent = 0);
    ~DlgRegularExport();

private slots:
    void on_pushButtonClose_clicked();
    void on_pushButtonExport_clicked();
    void on_pushButtonExportAll_clicked();

};

#endif // REGULAREXPORTDIALOG_H

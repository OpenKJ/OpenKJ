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

#ifndef REGULARIMPORTDIALOG_H
#define REGULARIMPORTDIALOG_H

#include <QDialog>
#include <QStringList>
#include "rotationmodel.h"

namespace Ui {
class DlgRegularImport;
}

class DlgRegularImport : public QDialog
{
    Q_OBJECT

private:
    Ui::DlgRegularImport *ui;
    QString curImportFile;
    QStringList loadSingerList(QString fileName);
    void importSinger(QString name);
    RotationModel *rotModel;

public:
    explicit DlgRegularImport(RotationModel *rotationModel, QWidget *parent = 0);
    ~DlgRegularImport();

private slots:
    void on_pushButtonSelectFile_clicked();
    void on_pushButtonClose_clicked();
    void on_pushButtonImport_clicked();
    void on_pushButtonImportAll_clicked();

};

#endif // REGULARIMPORTDIALOG_H

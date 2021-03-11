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
#include "src/models/tablemodelhistorysingers.h"
#include "src/models/tablemodelhistorysongs.h"

namespace Ui {
class DlgRegularExport;
}

class DlgRegularExport : public QDialog
{
    Q_OBJECT

private:
    Ui::DlgRegularExport *ui;
    TableModelHistorySingers m_historySingersModel;
    TableModelHistorySongs m_historySongsModel;
    void exportSingers(const std::vector<int> &historySingerIds, const QString &savePath);

public:
    explicit DlgRegularExport(QWidget *parent = 0);
    ~DlgRegularExport();

private slots:
    void on_pushButtonClose_clicked();
    void on_pushButtonExport_clicked();
    void on_pushButtonExportAll_clicked();


    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;

    // QDialog interface
public slots:
    void done(int) override;
};

#endif // REGULAREXPORTDIALOG_H

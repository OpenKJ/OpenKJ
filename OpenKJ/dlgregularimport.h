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
#include "historysongstablemodel.h"
#include "historysingerstablemodel.h"

namespace Ui {
class DlgRegularImport;
}

class DlgRegularImport : public QDialog
{
    Q_OBJECT

private:
    Ui::DlgRegularImport *ui;
    QString m_curImportFile;
    QStringList legacyLoadSingerList(const QString &fileName);
    QStringList loadSingerList(const QString &filename);
    QStringList legacyImportSinger(const QString &name);
    QStringList importSinger(const QString &name);
    HistorySingersTableModel m_historySingersModel;
    HistorySongsTableModel m_historySongsModel;

public:
    explicit DlgRegularImport(QWidget *parent = 0);
    ~DlgRegularImport();

private slots:
    void on_pushButtonSelectFile_clicked();
    void on_pushButtonClose_clicked();
    void on_pushButtonImport_clicked();
    void on_pushButtonImportAll_clicked();


    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;

    // QDialog interface
public slots:
    void done(int) override;
};

#endif // REGULARIMPORTDIALOG_H

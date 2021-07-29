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

#ifndef REGULARSINGERSDIALOG_H
#define REGULARSINGERSDIALOG_H

#include <QDialog>
#include <QSortFilterProxyModel>
#include <QSqlTableModel>
#include "src/models/tablemodelrotation.h"
#include "src/models/tablemodelhistorysingers.h"
#include "settings.h"

namespace Ui {
class DlgRegularSingers;
}

class DlgRegularSingers : public QDialog
{
    Q_OBJECT

private:
    int m_rtClickHistorySingerId;
    Ui::DlgRegularSingers *ui;
    TableModelHistorySingers m_historySingersModel;
    ItemDelegateHistorySingers m_historySingersDelegate;
    TableModelRotation *m_rotModel;
    Settings m_settings;

public:
    explicit DlgRegularSingers(TableModelRotation *rotationModel, QWidget *parent = 0);
    ~DlgRegularSingers();
    TableModelHistorySingers& historySingersModel() { return m_historySingersModel; }

signals:
    void regularSingerDeleted(const int regularID);
    void regularSingerRenamed(const int regularID, const QString newName);

private slots:
    void on_btnClose_clicked();
    void on_tableViewRegulars_clicked(const QModelIndex &index);
    void on_tableViewRegulars_customContextMenuRequested(const QPoint &pos);
    void renameHistorySinger();
    void on_lineEditSearch_textChanged(const QString &arg1);
    void on_tableViewRegulars_doubleClicked(const QModelIndex &index);

public slots:
    void regularsChanged();
    void toggleVisibility();

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // REGULARSINGERSDIALOG_H

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

#ifndef REGULARSINGERSDIALOG_H
#define REGULARSINGERSDIALOG_H

#include <QDialog>
#include "regularsingermodel.h"
#include "khsinger.h"

namespace Ui {
class RegularSingersDialog;
}

class RegularSingersDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegularSingersDialog(KhRegularSingers *regSingers, KhSingers *rotSingers, QWidget *parent = 0);
    ~RegularSingersDialog();

signals:
    void regularSingerDeleted(int regularID);
    void regularSingerRenamed(int regularID, QString newName);

private slots:
    void on_btnClose_clicked();
    void on_treeViewRegulars_clicked(const QModelIndex &index);
    void editSingerDuplicateError();

private:
    void addRegularToRotation(int ListIndex);
    Ui::RegularSingersDialog *ui;
    RegularSingerModel *regularSingerModel;
    KhSingers *m_rotSingers;
    KhRegularSingers *m_regSingers;
};

#endif // REGULARSINGERSDIALOG_H

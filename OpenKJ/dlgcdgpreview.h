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

#ifndef CDGPREVIEWDIALOG_H
#define CDGPREVIEWDIALOG_H

#include <QDialog>
#include <QTimer>
#include <QTemporaryDir>
#include "libCDG/include/libCDG.h"
#include "audiobackendgstreamer.h"

namespace Ui {
class DlgCdgPreview;
}

class DlgCdgPreview : public QDialog
{
    Q_OBJECT

private:
    Ui::DlgCdgPreview *ui;
    QString m_srcFile;
    QTimer timer;
    QTemporaryDir *cdgTempDir;
    CdgParser cdg;
//    AudioBackendGstreamer *mediaBackend;
    unsigned int cdgPosition;

public:
    explicit DlgCdgPreview(QWidget *parent = 0);
    ~DlgCdgPreview();
    void setSourceFile(QString srcFile);

private slots:
    void timerTimeout();
    void on_pushButtonClose_clicked();
    void videoFrameReceived(QImage frame, QString backendName);

public slots:
    void preview();


    // QWidget interface
protected:
    void closeEvent(QCloseEvent *);

    // QDialog interface
public slots:
    void reject();
};

#endif // CDGPREVIEWDIALOG_H

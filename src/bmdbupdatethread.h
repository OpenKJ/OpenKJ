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

#ifndef BMDBUPDATETHREAD_H
#define BMDBUPDATETHREAD_H

#include <QThread>
#include <QStringList>
#include <QtSql>

class BmDbUpdateThread : public QThread
{
    Q_OBJECT
public:
    explicit BmDbUpdateThread(QObject *parent = nullptr);
    void run() override;
    void startUnthreaded();
    [[nodiscard]] QString path() const;
    void setPath(const QString &path);

signals:
    void progressMessage(QString msg);
    void stateChanged(QString state);
    void progressChanged(int progress, int max);
    
public slots:

private:
    QString m_path;
    QStringList findMediaFiles(const QString& directory);
    QStringList supportedExtensions;
    QSqlDatabase database;

    
};

#endif // BMDBUPDATETHREAD_H

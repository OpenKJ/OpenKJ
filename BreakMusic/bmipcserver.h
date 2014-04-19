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

#ifndef KHBMIPCSERVER_H
#define KHBMIPCSERVER_H

#include <QObject>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>


class BmIPCServer : public QObject
{
    Q_OBJECT
public:
    enum IpcCmd{CMD_NOOP=-1,CMD_FADE_OUT=0,CMD_FADE_IN,CMD_STOP,CMD_PAUSE,CMD_PLAY};
    BmIPCServer(QString servername, QObject *parent);
    ~BmIPCServer();

    int lastIpcCmd() const;
    void setLastIpcCmd(const int &lastIpcCmd);

signals:
    void messageReceived(int);

public slots:
    void socket_new_connection();

private:
    QLocalServer*       m_server;
    int m_lastIpcCmd;
};




#endif // KHBMIPCSERVER_H

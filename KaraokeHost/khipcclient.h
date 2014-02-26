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


#ifndef KHIPCCLIENT_H
#define KHIPCCLIENT_H

#include <QObject>
#include <QtNetwork/QLocalSocket>

class KhIPCClient : public QObject
{
    Q_OBJECT
public:
    KhIPCClient(QString remoteServername, QObject *parent = 0);
    ~KhIPCClient();
    enum{CMD_FADE_OUT=0,CMD_FADE_IN,CMD_STOP,CMD_PAUSE,CMD_PLAY};

signals:

public slots:
//    void send_MessageToServer(QString message);
    void send_MessageToServer(int command);

    void socket_connected();
    void socket_disconnected();

    void socket_readReady();
    void socket_error(QLocalSocket::LocalSocketError);

private:
    QLocalSocket*   m_socket;
    quint16 m_blockSize;
    QString m_message;
    QString m_serverName;
    int m_command;
    
};

#endif // KHIPCCLIENT_H

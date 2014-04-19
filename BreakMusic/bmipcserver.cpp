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

#include "bmipcserver.h"
#include <QDataStream>
#include <QFile>

BmIPCServer::BmIPCServer(QString servername, QObject *parent)
    :QObject(parent) {
    m_server = new QLocalServer(this);
    m_lastIpcCmd = CMD_NOOP;
    qDebug() << "Attempting to open command socket...";
    if (!m_server->listen(servername)) {
        qDebug() << "Failure: Error creating socket, trying to delete socket and trying again";
        QFile::remove("/tmp/" + servername);
        if (m_server->listen(servername))
            qDebug() << "Success: Listening on socket " << servername;
        else
            qDebug() << "Failure: Unable to open socket";

    }
    else
        qDebug() << "Success: Listening on socket " << servername;

    connect(m_server, SIGNAL(newConnection()), this, SLOT(socket_new_connection()));
}

BmIPCServer::~BmIPCServer() {

}


void BmIPCServer::socket_new_connection() {

    QLocalSocket *clientConnection = m_server->nextPendingConnection();

    while (clientConnection->bytesAvailable() < (int)sizeof(quint32))
        clientConnection->waitForReadyRead();


    connect(clientConnection, SIGNAL(disconnected()),
            clientConnection, SLOT(deleteLater()));

    QDataStream in(clientConnection);
    in.setVersion(QDataStream::Qt_5_0);
    if (clientConnection->bytesAvailable() < (int)sizeof(quint16)) {
        return;
    }
    int command;
    QString message;
    in >> command;

    m_lastIpcCmd = command;
    emit messageReceived(command);
}
int BmIPCServer::lastIpcCmd() const
{
    return m_lastIpcCmd;
}

void BmIPCServer::setLastIpcCmd(const int &lastIpcCmd)
{
    m_lastIpcCmd = lastIpcCmd;
}


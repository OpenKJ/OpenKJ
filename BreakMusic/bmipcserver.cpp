/*
 * Copyright (c) 2013-2017 Thomas Isaac Lightburn
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
#include <QApplication>
#include <QDataStream>
#include <QFile>

BmIPCServer::BmIPCServer(QString servername, QObject *parent)
    :QObject(parent) {
    m_server = new QLocalServer(this);
    m_lastIpcCmd = CMD_NOOP;
    qDebug() << "IPC Server - Attempting to open command socket...";
    if (!m_server->listen(servername)) {
        qDebug() << "IPC Server - Error creating socket, attempting to delete socket and trying again";
        QFile::remove("/tmp/" + servername);
        if (m_server->listen(servername))
            qDebug() << "IPC Server - Success! Listening on socket" << servername;
        else
            qDebug() << "IPC Server - Failure! Unable to open socket!";

    }
    else
        qDebug() << "IPC Server - Socket opened successfully.  Listening on socket: " << servername;
    clientConnection = NULL;
    connect(m_server, SIGNAL(newConnection()), this, SLOT(socket_new_connection()));
}

BmIPCServer::~BmIPCServer()
{
}


void BmIPCServer::socket_new_connection()
{
    qDebug() << "IPC Server - New client connected";
    clientConnection = m_server->nextPendingConnection();
    connect(clientConnection, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
    connect(clientConnection, SIGNAL(readyRead()), this, SLOT(dataReady()));
}

void BmIPCServer::dataReady()
{
    qDebug() << "IPC Server - Data received";
    if (clientConnection != NULL)
    {
        QDataStream in(clientConnection);
        in.setVersion(QDataStream::Qt_5_0);
        if (clientConnection->bytesAvailable() < 1) {
            return;
        }
        int command;
        in >> command;
        m_lastIpcCmd = command;
        emit messageReceived(command);
    }
}

void BmIPCServer::clientDisconnected()
{
    qDebug() << "IPC Server - Client disconnected";
    clientConnection->deleteLater();
}

int BmIPCServer::lastIpcCmd() const
{
    return m_lastIpcCmd;
}



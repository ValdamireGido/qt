/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativedebugservertcpconnection_p.h"

#include "qdeclarativedebugserver_p.h"
#include "private/qpacketprotocol_p.h"

#include <QtNetwork/qtcpserver.h>
#include <QtNetwork/qtcpsocket.h>


QT_BEGIN_NAMESPACE

class QDeclarativeDebugServerTcpConnectionPrivate {
public:
    QDeclarativeDebugServerTcpConnectionPrivate();

    int port;
    QTcpSocket *socket;
    QPacketProtocol *protocol;
    QTcpServer *tcpServer;

    QDeclarativeDebugServer *debugServer;
};

QDeclarativeDebugServerTcpConnectionPrivate::QDeclarativeDebugServerTcpConnectionPrivate() :
    port(0),
    socket(0),
    protocol(0),
    tcpServer(0),
    debugServer(0)
{
}

QDeclarativeDebugServerTcpConnection::QDeclarativeDebugServerTcpConnection(int port, QDeclarativeDebugServer *server) :
    QObject(server),
    d_ptr(new QDeclarativeDebugServerTcpConnectionPrivate)
{
    Q_D(QDeclarativeDebugServerTcpConnection);
    d->port = port;
    d->debugServer = server;
}

QDeclarativeDebugServerTcpConnection::~QDeclarativeDebugServerTcpConnection()
{
    delete d_ptr;
}

bool QDeclarativeDebugServerTcpConnection::isConnected() const
{
    Q_D(const QDeclarativeDebugServerTcpConnection);
    return d->socket && d->socket->state() == QTcpSocket::ConnectedState;
}

void QDeclarativeDebugServerTcpConnection::send(const QByteArray &message)
{
    Q_D(QDeclarativeDebugServerTcpConnection);

    if (!isConnected())
        return;

    QPacket pack;
    pack.writeRawData(message.data(), message.length());

    d->protocol->send(pack);
    d->socket->flush();
}

void QDeclarativeDebugServerTcpConnection::disconnect()
{
    Q_D(QDeclarativeDebugServerTcpConnection);

    delete d->protocol;
    d->protocol = 0;
    delete d->socket;
    d->socket = 0;
}

void QDeclarativeDebugServerTcpConnection::listen()
{
    Q_D(QDeclarativeDebugServerTcpConnection);

    d->tcpServer = new QTcpServer(this);
    QObject::connect(d->tcpServer, SIGNAL(newConnection()), this, SLOT(newConnection()));
    if (d->tcpServer->listen(QHostAddress::Any, d->port))
        qWarning("QDeclarativeDebugServer: Waiting for connection on port %d...", d->port);
    else
        qWarning("QDeclarativeDebugServer: Unable to listen on port %d", d->port);
}

void QDeclarativeDebugServerTcpConnection::waitForConnection()
{
    Q_D(QDeclarativeDebugServerTcpConnection);
    d->tcpServer->waitForNewConnection(-1);
}

void QDeclarativeDebugServerTcpConnection::readyRead()
{
    Q_D(QDeclarativeDebugServerTcpConnection);
    QPacket packet = d->protocol->read();

    QByteArray content = packet.data();
    d->debugServer->receiveMessage(content);
}

void QDeclarativeDebugServerTcpConnection::newConnection()
{
    Q_D(QDeclarativeDebugServerTcpConnection);

    if (d->socket) {
        qWarning("QDeclarativeDebugServer error: another client is already connected");
        QTcpSocket *faultyConnection = d->tcpServer->nextPendingConnection();
        delete faultyConnection;
        return;
    }

    d->socket = d->tcpServer->nextPendingConnection();
    d->socket->setParent(this);
    d->protocol = new QPacketProtocol(d->socket, this);
    QObject::connect(d->protocol, SIGNAL(readyRead()), this, SLOT(readyRead()));
}


QT_END_NAMESPACE
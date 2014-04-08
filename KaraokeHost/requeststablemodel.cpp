#include "requeststablemodel.h"
#include <QDebug>

RequestsTableModel::RequestsTableModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    timer = new QTimer(this);
    timer->setInterval(10000);
    timer->start();
    connect(timer, SIGNAL(timeout()), this, SLOT(timerExpired()));
}

void RequestsTableModel::timerExpired()
{
    qDebug() << "Timer tick";
}


int RequestsTableModel::rowCount(const QModelIndex &parent) const
{
    return requests.size();
}

int RequestsTableModel::columnCount(const QModelIndex &parent) const
{
    return 5;
}

QVariant RequestsTableModel::data(const QModelIndex &index, int role) const
{
    return QString();
}

QVariant RequestsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        switch(section) {
        case REQUESTID:
            return "RequestID";
        case SINGER:
            return "Singer";
        case ARTIST:
            return "Artist";
        case TITLE:
            return "Title";
        case TIMESTAMP:
            return "Received";
        }
    }
    else
        return QVariant();
}

Qt::ItemFlags RequestsTableModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

Request::Request(int RequestId, QString Singer, QString Artist, QString Title, int ts)
{
    m_requestId = RequestId;
    m_singer = Singer;
    m_artist = Artist;
    m_title = Title;
    m_timeStamp = ts;
}

int Request::requestId() const
{
    return m_requestId;
}

void Request::setRequestId(int requestId)
{
    m_requestId = requestId;
}
int Request::timeStamp() const
{
    return m_timeStamp;
}

void Request::setTimeStamp(int timeStamp)
{
    m_timeStamp = timeStamp;
}
QString Request::artist() const
{
    return m_artist;
}

void Request::setArtist(const QString &artist)
{
    m_artist = artist;
}
QString Request::title() const
{
    return m_title;
}

void Request::setTitle(const QString &title)
{
    m_title = title;
}
QString Request::singer() const
{
    return m_singer;
}

void Request::setSinger(const QString &singer)
{
    m_singer = singer;
}





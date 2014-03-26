#include "khqsinger.h"


int KhQSinger::dbIndex() const
{
    return m_dbIndex;
}

void KhQSinger::setDbIndex(int dbIndex)
{
    m_dbIndex = dbIndex;
}

int KhQSinger::index() const
{
    return m_index;
}

void KhQSinger::setIndex(int index)
{
    m_index = index;
}

QString KhQSinger::name() const
{
    return m_name;
}

void KhQSinger::setName(const QString &name)
{
    m_name = name;
}

KhQSongs *KhQSinger::qSongs()
{
    return m_qSongs;
}

void KhQSinger::setQSongs(KhQSongs *qSongs)
{
    m_qSongs = qSongs;
}

bool KhQSinger::regular() const
{
    return m_regular;
}

void KhQSinger::setRegular(bool regular)
{
    m_regular = regular;
}
KhQSinger::KhQSinger(QObject *parent) :
    QObject(parent)
{
}

KhQSingers::KhQSingers(QObject *parent) :
    QObject(parent)
{
}

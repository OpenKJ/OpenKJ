#include "khqsong.h"


int KhQSong::dbIndex() const
{
    return m_dbIndex;
}

void KhQSong::setDbIndex(int dbIndex)
{
    m_dbIndex = dbIndex;
}

int KhQSong::index() const
{
    return m_index;
}

void KhQSong::setIndex(int index)
{
    m_index = index;
}

KhSong *KhQSong::song() const
{
    return m_song;
}

void KhQSong::setSong(KhSong *song)
{
    m_song = song;
}

int KhQSong::keyChange() const
{
    return m_keyChange;
}

void KhQSong::setKeyChange(int keyChange)
{
    m_keyChange = keyChange;
}

bool KhQSong::played() const
{
    return m_played;
}

void KhQSong::setPlayed(bool played)
{
    m_played = played;
}

bool KhQSong::regular() const
{
    return m_regular;
}

void KhQSong::setRegular(bool regular)
{
    m_regular = regular;
}

int KhQSong::regSingerDbIndex() const
{
    return m_regSingerDbIndex;
}

void KhQSong::setRegSingerDbIndex(int regSingerDbIndex)
{
    m_regSingerDbIndex = regSingerDbIndex;
}

int KhQSong::regSongDbIndex() const
{
    return m_regSongDbIndex;
}

void KhQSong::setRegSongDbIndex(int regSongDbIndex)
{
    m_regSongDbIndex = regSongDbIndex;
}

int KhQSong::qSingerDbIndex() const
{
    return m_qSingerDbIndex;
}

void KhQSong::setQSingerDbIndex(int qSingerDbIndex)
{
    m_qSingerDbIndex = qSingerDbIndex;
}
KhQSong::KhQSong(QObject *parent) :
    QObject(parent)
{
}


QList<KhQSong *> *KhQSongs::getSongs() const
{
    return songs;
}

void KhQSongs::setSongs(QList<KhQSong *> *value)
{
    songs = value;
}

int KhQSongs::add(KhQSong *song)
{

}

void KhQSongs::insert(KhQSong *song, int index)
{

}

void KhQSongs::remove(int index)
{

}

void KhQSongs::move(int oldIndex, int newIndex)
{

}

bool KhQSongs::hasUnplayedSong()
{

}

KhQSong *KhQSongs::currentSong()
{

}

KhQSong *KhQSongs::nextSong()
{

}
KhQSongs::KhQSongs(QObject *parent) :
    QObject(parent)
{
}

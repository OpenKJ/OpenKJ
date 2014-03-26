#ifndef KHQSONG_H
#define KHQSONG_H

#include <QObject>
#include <khsong.h>
//#include <khqsinger.h>

class KhQSong : public QObject
{
    Q_OBJECT
private:
    int m_dbIndex;
    int m_index;
    KhSong *m_song;
//    KhQSinger *m_qSinger;
    int m_keyChange;
    bool m_played;
    bool m_regular;
    int m_regSingerDbIndex;
    int m_regSongDbIndex;
    int m_qSingerDbIndex;

public:
    explicit KhQSong(QObject *parent = 0);
    int dbIndex() const;
    void setDbIndex(int dbIndex);
    int index() const;
    void setIndex(int index);
    KhSong *song() const;
    void setSong(KhSong *song);
    int keyChange() const;
    void setKeyChange(int keyChange);
    bool played() const;
    void setPlayed(bool played);
    bool regular() const;
    void setRegular(bool regular);
    int regSingerDbIndex() const;
    void setRegSingerDbIndex(int regSingerDbIndex);
    int regSongDbIndex() const;
    void setRegSongDbIndex(int regSongDbIndex);
    int qSingerDbIndex() const;
    void setQSingerDbIndex(int qSingerDbIndex);

signals:
    void dataAboutToChange();
    void dataChanged();

public slots:

};

class KhQSongs : public QObject
{
    Q_OBJECT
private:
    QList<KhQSong*> *songs;
public:
    explicit KhQSongs(QObject *parent = 0);
    QList<KhQSong *> *getSongs() const;
    void setSongs(QList<KhQSong *> *value);
    int add(KhQSong *song);
    void insert(KhQSong *song, int index);
    void remove(int index);
    KhQSong *at(int index) { return songs->at(index); }
    int size() { return songs->size(); }
    void move(int oldIndex, int newIndex);
    bool hasUnplayedSong();
    KhQSong *currentSong();
    KhQSong *nextSong();

signals:
    void dataAboutToChange();
    void dataChanged();

public slots:

};

#endif // KHQSONG_H

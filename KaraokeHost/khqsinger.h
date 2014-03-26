#ifndef KHQSINGER_H
#define KHQSINGER_H

#include <QObject>
#include <khqsong.h>

class KhQSinger : public QObject
{
    Q_OBJECT
private:
    int m_dbIndex;
    int m_index;
    QString m_name;
    KhQSongs *m_qSongs;
    bool m_regular;

public:
    explicit KhQSinger(QObject *parent = 0);

    int dbIndex() const;
    void setDbIndex(int dbIndex);

    int index() const;
    void setIndex(int index);

    QString name() const;
    void setName(const QString &name);

    KhQSongs *qSongs();
    void setQSongs(KhQSongs *qSongs);

    bool regular() const;
    void setRegular(bool regular);

signals:

public slots:

};

class KhQSingers : public QObject
{
    Q_OBJECT
private:
    QList<KhQSinger*> *singers;

public:
    explicit KhQSingers(QObject *parent = 0);
    /**
     * @brief Add a new singer to the queue.
     * @param singer Takes a pointer to a KhQSinger object
     * @return Returns the index of the newly created singer
     */
    int add(KhQSinger *singer);
    void insert(KhQSinger *singer, int index);
    void remove(int index);
    void move(int oldIndex, int newIndex);
    KhQSinger *at(int index) { return singers->at(index); }
    int size() { return singers->size(); }
    KhQSinger *currentSinger();
    KhQSinger *nextSinger();

signals:
    void dataAboutToChange();
    void dataChanged();

public slots:


};

#endif // KHQSINGER_H

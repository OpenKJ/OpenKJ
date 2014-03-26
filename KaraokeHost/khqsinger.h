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
     * @param singer Pointer to a new KhQSinger object
     * @return Returns the index of the newly created singer
     */
    int add(KhQSinger *singer);
    /**
     * @brief Insert a singer to the queue at an index position
     * @param singer Pointer to a new KhQSinger object
     * @param index The index at which the singer should be added.  This is not range checked.
     */
    void insert(KhQSinger *singer, int index);
    /**
     * @brief Remove a singer from the queue
     * @param index The index of the KhQSinger to be removed
     */
    void remove(int index);
    /**
     * @brief Move a singer within the queue
     * @param oldIndex The index of the KhQSinger being moved
     * @param newIndex The destination index that the KhQSinger is to be moved to
     */
    void move(int oldIndex, int newIndex);
    /**
     * @brief Retrieve the singer at the specified index
     * @param index The index of the KhQSinger to be retrieved
     * @return Returns a pointer to the KhQSinger at the given index
     */
    KhQSinger *at(int index) { return singers->at(index); }
    /**
     * @brief Get the size of the queue
     * @return Returns the number of elements in the KhQSingers object
     */
    int size() { return singers->size(); }
    /**
     * @brief Get the currently active singer
     * @return Returns a pointer the currently active KhQSinger (playing or most recently played).  If the KhQSingers object is empty or no singer is set as active return will be NULL.
     */
    KhQSinger *currentSinger();
    /**
     * @brief Get the next singer in the KhQSingers object, based upon the current singer.
     * @return Returns the singer after the current one based on index order.  If the KhQSingers object is empty, return will be NULL.  If no singer is currently active, a pointer to the KhQSinger at index 0 will be returned.
     */
    KhQSinger *nextSinger();

signals:
    /**
     * @brief Signal that should be emitted any time a change is about to be made to the object or its children.  Primarily for use with QAbstractTableModel for updating views.
     */
    void dataAboutToChange();
    /**
     * @brief Signal that shoulch be emitted any time a change has been made to the object or its children.  Primarily for use with QAbstractTableModel for updating views.
     */
    void dataChanged();

public slots:


};

#endif // KHQSINGER_H

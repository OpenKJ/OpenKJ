#ifndef KHQSINGER_H
#define KHQSINGER_H

#include <QObject>
#include <khqsong.h>

/**
 * @brief The KhQSinger class represents singers in the program's singer queue.
 * Most of the changes also update the database to keep it in sync with the program unless skipdb is specified where applicable.  Any code that is going to
 * be doing lots of updates should wrap them in a SQL transaction to keep things speedy.
 */
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
    /**
     * @brief KhQSinger default constructor
     * @param parent Pointer to a parent QObject based object, usually a KhQSingers object
     */
    explicit KhQSinger(QObject *parent = 0);
    /**
     * @brief Get the database index
     * @return An int containing the db index for the table row associated with this KhQSinger
     */
    int dbIndex() const;
    /**
     * @brief Set the database index.
     * This should generally only be used when populating KhQSinger objects while loading the database.
     * @param dbIndex The db index for the table row associated with this KhQSinger
     */
    void setDbIndex(int dbIndex);
    /**
     * @brief Get the position index
     * @return An int containing the index
     */
    int index() const;
    /**
     * @brief Set the position index
     * This should only be used when loading the database or by the KhQSingers object when manipulating the order.
     * @param index The position index.
     */
    void setIndex(int index);
    /**
     * @brief Get the name of the singer
     * @return Returns a QString containing the singer's name
     */
    QString name() const;
    /**
     * @brief Set the singer's name
     * @param name A QString set to the desired singer name
     */
    void setName(const QString &name);
    /**
     * @brief Get a pointer to the KhQSongs object associated with this singer
     * @return Returns a pointer to the KhQSongs object
     */
    KhQSongs *qSongs();
    /**
     * @brief setQSongs Set the KhQSongs object this KhQSinger should use
     * This is only intended to be used when creating new singers based on the database or on regular singers being loaded.
     * @param qSongs A pointer to a KhQSongs object
     */
    void setQSongs(KhQSongs *qSongs);
    /**
     * @brief Gets whether this singer is being tracked as a regular
     * @return Returns true if the singer is being tracked as a regular, false otherwise
     */
    bool regular() const;
    /**
     * @brief Sets whether the singers is being tracked as a regular
     * @param regular A bool variable that should be true if the singer is being tracked as a regular, false otherwise.
     */
    void setRegular(bool regular);

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

/**
 * @brief The KhQSingers class is a container for the KhQSinger objects which are used to populate the singer queue.
 */
class KhQSingers : public QObject
{
    Q_OBJECT
private:
    QList<KhQSinger*> *singers;

public:
    /**
     * @brief KhQSingers default constructor
     * @param parent Pointer to a parent QObject based object
     */
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

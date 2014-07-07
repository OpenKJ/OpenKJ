#ifndef PLAYLISTITEMDELEGATE_H
#define PLAYLISTITEMDELEGATE_H

#include <QItemDelegate>

class PlaylistItemDelegate : public QItemDelegate
{
    Q_OBJECT
private:
    int m_currentSong;

public:
    explicit PlaylistItemDelegate(QObject *parent = 0);

signals:

public slots:


    // QAbstractItemDelegate interface
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    int currentSong() const;
    void setCurrentSong(int value);

    // QAbstractItemDelegate interface
public:
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // PLAYLISTITEMDELEGATE_H

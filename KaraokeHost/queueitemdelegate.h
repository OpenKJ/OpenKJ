#ifndef QUEUEITEMDELEGATE_H
#define QUEUEITEMDELEGATE_H

#include <QItemDelegate>
#include <QPainter>


class QueueItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    explicit QueueItemDelegate(QObject *parent = 0);

signals:

public slots:


    // QAbstractItemDelegate interface
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // QUEUEITEMDELEGATE_H

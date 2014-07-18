#ifndef DBITEMDELEGATE_H
#define DBITEMDELEGATE_H

#include <QItemDelegate>
#include <QPainter>

class DbItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    explicit DbItemDelegate(QObject *parent = 0);

signals:

public slots:


    // QAbstractItemDelegate interface
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // DBITEMDELEGATE_H

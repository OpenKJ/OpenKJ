#ifndef REGITEMDELEGATE_H
#define REGITEMDELEGATE_H

#include <QItemDelegate>
#include <QPainter>

class RegItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    explicit RegItemDelegate(QObject *parent = 0);

signals:

public slots:


    // QAbstractItemDelegate interface
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // REGITEMDELEGATE_H

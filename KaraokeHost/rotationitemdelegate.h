#ifndef ROTATIONITEMDELEGATE_H
#define ROTATIONITEMDELEGATE_H

#include <QItemDelegate>
#include <QPainter>

class RotationItemDelegate : public QItemDelegate
{
    Q_OBJECT
private:
    int m_currentSingerId;
public:
    explicit RotationItemDelegate(QObject *parent = 0);
    int currentSinger();
    void setCurrentSinger(int currentSingerId);

signals:

public slots:


    // QAbstractItemDelegate interface
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // ROTATIONITEMDELEGATE_H

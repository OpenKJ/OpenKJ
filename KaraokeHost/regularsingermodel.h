#ifndef REGULARSINGERMODEL_H
#define REGULARSINGERMODEL_H

#include <QAbstractTableModel>
#include <khregularsinger.h>

class RegularSingerModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit RegularSingerModel(QObject *parent = 0);
    enum {REGID=0,SINGER,SONGCOUNT,LOAD,RENAME,DELETE};

signals:

public slots:


    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

private:
    KhRegularSingers *regularSingers;
};

#endif // REGULARSINGERMODEL_H

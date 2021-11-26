#ifndef CUSTOMPATTERNSMODEL_H
#define CUSTOMPATTERNSMODEL_H

#include <QAbstractTableModel>
#include <custompattern.h>

class TableModelCustomNamingPatterns : public QAbstractTableModel
{
    Q_OBJECT

    QList<CustomPattern> myData;

public:
    explicit TableModelCustomNamingPatterns(QObject *parent = 0);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:

    // QAbstractItemModel interface
public:
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    CustomPattern &getPattern(int index);
    void loadFromDB();
};

#endif // CUSTOMPATTERNSMODEL_H

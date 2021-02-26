#include "tablemodelrotation.h"

#include <QSqlQuery>

TableModelRotation::TableModelRotation(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant TableModelRotation::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section) {
        case COL_ID:
            return QVariant();
        case COL_NAME:
            return "Name";
        default:
            return QVariant();

        }
    }
    return QVariant();
}

int TableModelRotation::rowCount([[maybe_unused]]const QModelIndex &parent) const
{
    return m_singers.size();
}

int TableModelRotation::columnCount([[maybe_unused]]const QModelIndex &parent) const
{
    return 5;
}

QVariant TableModelRotation::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (role == Qt::DisplayRole)
    {
        switch (index.column()) {
        case COL_ID:
            return m_singers.at(index.row()).id;
        case COL_NAME:
            return m_singers.at(index.row()).name;
        case COL_POSITION:
            return m_singers.at(index.row()).position;
        case COL_REGULAR:
            return m_singers.at(index.row()).regular;
        case COL_ADDTS:
            return m_singers.at(index.row()).addTs;

        }
    }
    return QVariant();
}

void TableModelRotation::loadData()
{
    emit layoutAboutToBeChanged();
    m_singers.clear();
    QSqlQuery query;
    query.exec("SELECT singerid,name,position,regular,addts FROM rotationsingers");
    while (query.next())
    {
        m_singers.emplace_back(RotationSinger{
                                   query.value(0).toInt(),
                                   query.value(1).toString(),
                                   query.value(2).toInt(),
                                   query.value(3).toBool(),
                                   query.value(4).toDateTime()
                               });
    }
    emit layoutChanged();
}

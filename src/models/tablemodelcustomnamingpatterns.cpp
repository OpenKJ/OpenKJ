#include "tablemodelcustomnamingpatterns.h"
#include <QSqlQuery>

void TableModelCustomNamingPatterns::loadFromDB()
{
    layoutAboutToBeChanged();
    QSqlQuery query;
    myData.clear();
    query.exec("SELECT * from custompatterns ORDER BY name");
    while (query.next())
    {
        CustomPattern pattern(
            query.value("name").toString(),
            query.value("artistregex").toString(),
            query.value("artistcapturegrp").toInt(),
            query.value("titleregex").toString(),
            query.value("titlecapturegrp").toInt(),
                    // TODO: disk id or song id??
            query.value("discidregex").toString(),
            query.value("discidcapturegrp").toInt()
           );
        myData.append(pattern);
    }
    layoutChanged();
}

TableModelCustomNamingPatterns::TableModelCustomNamingPatterns(QObject *parent)
    : QAbstractTableModel(parent)
{
    loadFromDB();
}

QVariant TableModelCustomNamingPatterns::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:
            return tr("Name");
        case 1:
            return tr("Artist RegEx");
        case 2:
            return tr("Group");
        case 3:
            return tr("Title RegEx");
        case 4:
            return tr("Group");
        case 5:
            return tr("SongID RegEx");
        case 6:
            return tr("Group");

        }
    }
    return QVariant();
}

int TableModelCustomNamingPatterns::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return myData.count();
}

int TableModelCustomNamingPatterns::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 7;
}

QVariant TableModelCustomNamingPatterns::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(index.row() >= myData.size() || index.row() < 0)
        return QVariant();
    if(role == Qt::DisplayRole)
    {
        switch(index.column())
        {
        case 0:
            return myData.at(index.row()).getName();
        case 1:
            return myData.at(index.row()).getArtistRegex();
        case 2:
            return myData.at(index.row()).getArtistCaptureGrp();
        case 3:
            return myData.at(index.row()).getTitleRegex();
        case 4:
            return myData.at(index.row()).getTitleCaptureGrp();
        case 5:
            return myData.at(index.row()).getSongIdRegex();
        case 6:
            return myData.at(index.row()).getSongIdCaptureGrp();
        }
    }
    return QVariant();
}

Qt::ItemFlags TableModelCustomNamingPatterns::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

CustomPattern &TableModelCustomNamingPatterns::getPattern(int index)
{
    return myData[index];
}


















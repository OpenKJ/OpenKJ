#include "custompatternsmodel.h"
#include <QSqlQuery>
#include <QDebug>

void CustomPatternsModel::loadFromDB()
{
    layoutAboutToBeChanged();
    QSqlQuery query;
    myData.clear();
    query.exec("SELECT * from custompatterns ORDER BY name");
    while (query.next())
    {
        Pattern pattern;
        pattern.setName(query.value("name").toString());
        pattern.setArtistRegex(query.value("artistregex").toString());
        pattern.setArtistCaptureGrp(query.value("artistcapturegrp").toInt());
        pattern.setTitleRegex(query.value("titleregex").toString());
        pattern.setTitleCaptureGrp(query.value("titlecapturegrp").toInt());
        pattern.setDiscIdRegex(query.value("discidregex").toString());
        pattern.setDiscIdCaptureGrp(query.value("discidcapturegrp").toInt());
        myData.append(pattern);
    }
    layoutChanged();
}

CustomPatternsModel::CustomPatternsModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    loadFromDB();
    for (int i=0; i < myData.count(); i++)
    {
        qWarning() << myData.at(i).getName() << " - " << myData.at(i).getArtistRegex() << " - " << myData.at(i).getTitleRegex() << " - " << myData.at(i).getDiscIdRegex();
    }
}

QVariant CustomPatternsModel::headerData(int section, Qt::Orientation orientation, int role) const
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
            return tr("DiscID RegEx");
        case 6:
            return tr("Group");

        }
    }
    return QVariant();
}

int CustomPatternsModel::rowCount(const QModelIndex &parent) const
{
    return myData.count();
}

int CustomPatternsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 7;
}

QVariant CustomPatternsModel::data(const QModelIndex &index, int role) const
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
            return myData.at(index.row()).getDiscIdRegex();
        case 6:
            return myData.at(index.row()).getDiscIdCaptureGrp();
        }
    }
    return QVariant();
}

Qt::ItemFlags CustomPatternsModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

Pattern CustomPatternsModel::getPattern(int index)
{
    return myData.at(index);
}


















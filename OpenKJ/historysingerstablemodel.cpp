#include "historysingerstablemodel.h"

#include <QSize>
#include <QSqlQuery>
#include <QSqlError>
#include "settings.h"

extern Settings settings;

HistorySingersTableModel::HistorySingersTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    loadSingers();
    QString thm = (settings.theme() == 1) ? ":/theme/Icons/okjbreeze-dark/" : ":/theme/Icons/okjbreeze/";
    m_iconDelete16 = QIcon(thm + "actions/16/edit-delete.svg");
    m_iconDelete22 = QIcon(thm + "actions/22/edit-delete.svg");
    m_iconLoadReg16 = QIcon(thm + "actions/22/list-add-user.svg");
    m_iconLoadReg22 = QIcon(thm + "actions/16/list-add-user.svg");
}

QVariant HistorySingersTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section) {
        case 0:
            return "id";
        case 1:
            return "Name";
        case 2:
            return "Songs";
        default:
            return QVariant();
        }
    }
    return QVariant();
}

int HistorySingersTableModel::rowCount([[maybe_unused]]const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_singers.size();
}

int HistorySingersTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 5;
}

QVariant HistorySingersTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (role == Qt::TextAlignmentRole)
    {
        switch (index.column()) {
        case 2:
            return Qt::AlignRight;
        case 3:
            return Qt::AlignHCenter;
        case 4:
            return Qt::AlignHCenter;
        }
    }
    if (role == Qt::DecorationRole)
    {
        QSize sbSize(QFontMetrics(settings.applicationFont()).height(), QFontMetrics(settings.applicationFont()).height());
        switch (index.column()) {
        case 3:
            if (sbSize.height() > 18)
                return m_iconLoadReg22.pixmap(sbSize);
            else
                return m_iconLoadReg16.pixmap(sbSize);
        case 4:
            if (sbSize.height() > 18)
                return m_iconDelete22.pixmap(sbSize);
            else
                return m_iconDelete16.pixmap(sbSize);
        }
    }
    if (role == Qt::DisplayRole)
    {
        switch (index.column()) {
        case 0:
            return m_singers.at(index.row()).historySingerId;
        case 1:
            return m_singers.at(index.row()).name;
        case 2:
            return m_singers.at(index.row()).songCount;
        }
    }
    return QVariant();
}

int HistorySingersTableModel::getSongCount(const int historySingerId) const
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(id) FROM historySongs WHERE historySinger = :historySinger");
    query.bindValue(":historySinger", historySingerId);
    query.exec();
    if (query.next())
        return query.value(0).toInt();
    return 0;
}

void HistorySingersTableModel::loadSingers()
{
    emit layoutAboutToBeChanged();
    m_singers.clear();
    QSqlQuery query;
    if (m_filterString == QString())
        query.exec("SELECT id,name FROM historySingers ORDER BY name");
    else
    {
        query.prepare("SELECT id,name FROM historySingers WHERE name LIKE :likestr ORDER BY name");
        query.bindValue(":likestr", m_filterString);
        query.exec();
    }
    while (query.next())
    {
        m_singers.emplace_back(HistorySinger{query.value("id").toInt(),query.value("name").toString(),getSongCount(query.value("id").toInt())});
    }
    emit layoutChanged();
}

QString HistorySingersTableModel::getName(const int historySingerId) const
{
    QString name;
    std::for_each(m_singers.begin(), m_singers.end(), [&historySingerId, &name] (auto singer) {
        if (singer.historySingerId == historySingerId)
            name = singer.name;
    });
    return name;
}

bool HistorySingersTableModel::exists(const QString &name) const
{
    bool retval{false};
    std::for_each(m_singers.begin(), m_singers.end(), [&name, &retval] (auto singer) {
        if (singer.name.toLower() == name.toLower())
            retval = true;
    });
    return retval;
}

int HistorySingersTableModel::getId(const QString &historySingerName) const
{
    int id{-1};
    std::for_each(m_singers.begin(), m_singers.end(), [&historySingerName, &id] (auto singer) {
        if (singer.name.toLower() == historySingerName.toLower())
            id = singer.historySingerId;
    });
    return id;
}

void HistorySingersTableModel::deleteHistory(const int historySingerId)
{
    QSqlQuery query;
    query.prepare("DELETE from historySongs WHERE historySinger = :historySingerId");
    query.bindValue(":historySingerId", historySingerId);
    query.exec();
    qInfo() << query.lastError();
    query.prepare("DELETE FROM historySingers WHERE id = :historySingerId");
    query.bindValue(":historySingerId", historySingerId);
    query.exec();
    qInfo() << query.lastError();
    emit historySingersModified();
    loadSingers();
}

bool HistorySingersTableModel::rename(const int historySingerId, const QString &newName)
{
    if (exists(newName))
        return false;
    QSqlQuery query;
    query.prepare("UPDATE historySingers SET name = :newName WHERE id = :historySingerId");
    query.bindValue(":newName", newName);
    query.bindValue(":historySingerId", historySingerId);
    query.exec();
    loadSingers();
    emit historySingersModified();
    return true;
}

void HistorySingersTableModel::filter(const QString &filterString)
{
    m_filterString = filterString;
    m_filterString.replace(" ", "%");
    m_filterString = "%" + m_filterString + "%";
    loadSingers();
}

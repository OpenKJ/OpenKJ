#include "tablemodelhistorysingers.h"

#include <QSize>
#include <QSqlQuery>
#include <QSqlError>
#include <QPainter>
#include <QSvgRenderer>


TableModelHistorySingers::TableModelHistorySingers(QObject *parent)
    : QAbstractTableModel(parent)
{
    m_logger = spdlog::get("logger");
    loadSingers();
}

QVariant TableModelHistorySingers::headerData(int section, Qt::Orientation orientation, int role) const
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
            return {};
        }
    }
    return {};
}

int TableModelHistorySingers::rowCount([[maybe_unused]]const QModelIndex &parent) const
{
    return static_cast<int>(m_singers.size());
}

int TableModelHistorySingers::columnCount([[maybe_unused]]const QModelIndex &parent) const
{
    return 5;
}

QVariant TableModelHistorySingers::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};
    if (role == Qt::TextAlignmentRole)
    {
        switch (index.column()) {
        case 2:
            return Qt::AlignRight + Qt::AlignVCenter;
        case 3:
        case 4:
            return Qt::AlignHCenter + Qt::AlignVCenter;
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
    return {};
}

int TableModelHistorySingers::getSongCount(const int historySingerId)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(id) FROM historySongs WHERE historySinger = :historySinger");
    query.bindValue(":historySinger", historySingerId);
    query.exec();
    if (query.next())
        return query.value(0).toInt();
    return 0;
}

void TableModelHistorySingers::loadSingers()
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
        m_singers.emplace_back(okj::HistorySinger{query.value("id").toInt(),query.value("name").toString(),getSongCount(query.value("id").toInt())});
    }
    emit layoutChanged();
}

QString TableModelHistorySingers::getName(const int historySingerId) const
{
    auto match = std::find_if(m_singers.begin(), m_singers.end(), [&historySingerId] (auto singer) {
        return (singer.historySingerId == historySingerId);
    });
    if (match != m_singers.end())
        return match->name;
    return {};
}

bool TableModelHistorySingers::exists(const QString &name) const
{
    auto match = std::find_if(m_singers.begin(), m_singers.end(), [&name] (auto singer) {
        return (singer.name.toLower() == name.toLower());
    });
    return (match != m_singers.end());
}

int TableModelHistorySingers::getId(const QString &historySingerName) const
{
    auto match = std::find_if(m_singers.begin(), m_singers.end(), [&historySingerName] (auto singer) {
        return (singer.name.toLower() == historySingerName.toLower());
    });
    if (match != m_singers.end())
        return match->historySingerId;
    return -1;
}

void TableModelHistorySingers::deleteHistory(const int historySingerId)
{
    QSqlQuery query;
    query.prepare("DELETE from historySongs WHERE historySinger = :historySingerId");
    query.bindValue(":historySingerId", historySingerId);
    query.exec();
    query.prepare("DELETE FROM historySingers WHERE id = :historySingerId");
    query.bindValue(":historySingerId", historySingerId);
    query.exec();
    emit historySingersModified();
    loadSingers();
}

bool TableModelHistorySingers::rename(const int historySingerId, const QString &newName)
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

void TableModelHistorySingers::filter(const QString &filterString)
{
    m_filterString = filterString;
    m_filterString.replace(" ", "%");
    m_filterString = "%" + m_filterString + "%";
    loadSingers();
    auto blah = singers();
}

std::vector<okj::HistorySinger> &TableModelHistorySingers::singers()
{
    return m_singers;
}

okj::HistorySinger TableModelHistorySingers::getSinger(const int historySingerId)
{
    auto result = std::find_if(m_singers.begin(), m_singers.end(), [&historySingerId] (const okj::HistorySinger& singer) {
       return (singer.historySingerId == historySingerId);
    });
    if (result != m_singers.end())
        return *result;
    return {};
}

void ItemDelegateHistorySingers::resizeIconsForFont(const QFont& font)
{
    QString thm = (m_settings.theme() == 1) ? ":/theme/Icons/okjbreeze-dark/" : ":/theme/Icons/okjbreeze/";
    m_curFontHeight = QFontMetrics(font).height();
    m_iconDelete = QImage(m_curFontHeight, m_curFontHeight, QImage::Format_ARGB32);
    m_iconLoadReg = QImage(m_curFontHeight, m_curFontHeight, QImage::Format_ARGB32);
    m_iconDelete.fill(Qt::transparent);
    m_iconLoadReg.fill(Qt::transparent);
    QPainter painterDelete(&m_iconDelete);
    QPainter painterLoad(&m_iconLoadReg);
    QSvgRenderer svgrndrDelete(thm + "actions/16/edit-delete.svg");
    QSvgRenderer svgrndrLoad(thm + "actions/16/list-add-user.svg");
    svgrndrDelete.render(&painterDelete);
    svgrndrLoad.render(&painterLoad);
}

ItemDelegateHistorySingers::ItemDelegateHistorySingers(QObject *parent) :
    QItemDelegate(parent)
{
    resizeIconsForFont(m_settings.applicationFont());
}

void ItemDelegateHistorySingers::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid())
        return;
    switch (index.column()) {
    case 3:
    {
        int topPad = (option.rect.height() - m_curFontHeight) / 2;
        int leftPad = (option.rect.width() - m_curFontHeight) / 2;
        painter->drawImage(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, m_curFontHeight, m_curFontHeight),m_iconLoadReg);
        return;
    }
    case 4:
    {
        int topPad = (option.rect.height() - m_curFontHeight) / 2;
        int leftPad = (option.rect.width() - m_curFontHeight) / 2;
        painter->drawImage(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, m_curFontHeight, m_curFontHeight),m_iconDelete);
        return;
    }
    default:
        return QItemDelegate::paint(painter, option, index);
    }
}

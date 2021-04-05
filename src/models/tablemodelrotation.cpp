#include "tablemodelrotation.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>
#include <QSvgRenderer>
#include <QMimeData>
#include <QJsonArray>
#include <QJsonDocument>
#include "settings.h"

extern Settings settings;

TableModelRotation::TableModelRotation(QObject *parent)
    : QAbstractTableModel(parent)
{
    resizeIconsForFont(settings.applicationFont());
}

QVariant TableModelRotation::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::SizeHintRole)
    {
        switch (section) {
        case COL_REGULAR:
        case COL_DELETE:
        case COL_ID:
            auto fHeight = QFontMetrics(settings.applicationFont()).height();
            return QSize(fHeight,fHeight);
        }
    }
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
    return 6;
}

QVariant TableModelRotation::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (role == Qt::ToolTipRole)
    {
        QString toolTipText;
        auto curSingerPos{0};
        if (m_currentSingerId != -1)
            curSingerPos = getSingerPosition(m_currentSingerId);
        auto hoverSingerPos{index.sibling(index.row(), COL_POSITION).data().toInt()};
        int totalWaitDuration = 0;
        int singerId = index.data(Qt::UserRole).toInt();
        int qSongsSung = numSongsSung(singerId);
        int qSongsUnsung = numSongsUnsung(singerId);
        if (m_currentSingerId == singerId)
        {
            toolTipText = "Current singer - Sung: " + QString::number(qSongsSung) + " - Unsung: " + QString::number(qSongsUnsung);
        }
        else if (curSingerPos < hoverSingerPos)
        {
            toolTipText = "Wait: " + QString::number(hoverSingerPos - curSingerPos) + " - Sung: " + QString::number(qSongsSung) + " - Unsung: " + QString::number(qSongsUnsung);
            for (int i=curSingerPos; i < hoverSingerPos; i++)
            {
                int sId = singerIdAtPosition(i);
                if (i == curSingerPos)
                {
                    totalWaitDuration += m_remainSecs;
                }
                else if (sId != singerId)
                {
                    int nextDuration = nextSongDurationSecs(sId);
                    totalWaitDuration += nextDuration;
                }
            }
        }
        else if (curSingerPos > hoverSingerPos)
        {
            toolTipText = "Wait: " + QString::number(hoverSingerPos + (rowCount() - curSingerPos)) + " - Sung: " + QString::number(qSongsSung) + " - Unsung: " + QString::number(qSongsUnsung);
            for (int i=0; i < hoverSingerPos; i++)
            {
                int sId = singerIdAtPosition(i);
                if (sId != singerId)
                {
                    int nextDuration = nextSongDurationSecs(sId);
                    totalWaitDuration += nextDuration;
                }
            }
            for (int i=curSingerPos; i < rowCount(); i++)
            {
                int sId = singerIdAtPosition(i);
                if (i == curSingerPos)
                    totalWaitDuration += 240;
                else if (sId != singerId)
                {
                    int nextDuration = nextSongDurationSecs(sId);
                    totalWaitDuration += nextDuration;
                }
            }
        }
        toolTipText += "\nTime Added: " + m_singers.at(index.row()).addTs.toString("h:mm a");

        if (totalWaitDuration > 0)
        {
            int hours = 0;
            int minutes = totalWaitDuration / 60;
            int seconds = totalWaitDuration % 60;
            if (seconds > 0)
                minutes++;
            if (minutes > 60)
            {
                hours = minutes / 60;
                minutes = minutes % 60;
                if (hours > 1)
                    toolTipText += "\nEst wait time: " + QString::number(hours) + " hours " + QString::number(minutes) + " min";
                else
                    toolTipText += "\nEst wait time: " + QString::number(hours) + " hour " + QString::number(minutes) + " min";
            }
            else
                toolTipText += "\nEst wait time: " + QString::number(minutes) + " min";
        }
        return QString(toolTipText);
    }

    if (role == Qt::UserRole)
    {
        return m_singers.at(index.row()).id;
    }
    if (role == Qt::SizeHintRole)
    {
        switch (index.column()) {
        case COL_REGULAR:
        case COL_DELETE:
        case COL_ID:
            auto fHeight = QFontMetrics(settings.applicationFont()).height();
            return QSize(fHeight,fHeight);
        }
    }
    if (role == Qt::DecorationRole && index.column() == COL_NAME)
    {
        if (numSongsUnsung(index.data(Qt::UserRole).toInt()) > 0)
            return m_iconGreenCircle;
        return m_iconYellowCircle;
    }
    if (role == Qt::TextAlignmentRole && index.column() == COL_ID)
        return Qt::AlignCenter;
    if (role == Qt::BackgroundRole && m_singers.at(index.row()).id == m_currentSingerId)
    {
        if (index.column() > 0)
        return (settings.theme() == 1) ? QColor(180,180,0) : QColor("yellow");
    }
    if (role == Qt::ForegroundRole && m_singers.at(index.row()).id == m_currentSingerId)
    {
        if (index.column() > 0)
        return QColor("black");
    }
    if (role == Qt::DisplayRole)
    {
        switch (index.column()) {
        case COL_ID:
            if (settings.rotationDisplayPosition())
            {
                int curSingerPos{getSingerPosition(m_currentSingerId)};
                int wait{0};
                if (curSingerPos < index.row())
                    wait = index.row() - curSingerPos;
                else if (curSingerPos > index.row())
                    wait = index.row() + (rowCount() - curSingerPos);
                if (wait > 0)
                    return wait;
                return QVariant();
            }
            return QVariant();
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
    query.exec("SELECT singerid,name,position,regular,addts FROM rotationsingers ORDER BY position");
    qInfo() << "TableModelRotation - SQL error on load: " << query.lastError();
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
    qInfo() << "Loaded " << m_singers.size() << " rotation singers";
}

void TableModelRotation::commitChanges()
{
    QSqlQuery query;
    query.exec("BEGIN TRANSACTION");
    query.exec("DELETE FROM rotationsingers");
    query.prepare("INSERT INTO rotationsingers (singerid,name,position,regular,regularid,addts) VALUES(:singerid,:name,:pos,:regular,:regularid,:addts)");
    std::for_each(m_singers.begin(), m_singers.end(), [&] (RotationSinger &singer) {
        query.bindValue(":singerid", singer.id);
        query.bindValue(":name", singer.name);
        query.bindValue(":pos", singer.position);
        query.bindValue(":regular", singer.regular);
        query.bindValue(":regularid", -1);
        query.bindValue(":addts", singer.addTs);
        query.exec();
    });
    query.exec("COMMIT");
}

int TableModelRotation::singerAdd(const QString &name, const int positionHint)
{
    auto curTs = QDateTime::currentDateTime();
    int addPos = m_singers.size();
    QSqlQuery query;
    query.prepare("INSERT INTO rotationsingers (name,position,regular,regularid,addts) VALUES(:name,:pos,:regular,:regularid,:addts)");
    query.bindValue(":name", name);
    query.bindValue(":pos", addPos);
    query.bindValue(":regular", false);
    query.bindValue(":regularid", -1);
    query.bindValue(":addts", curTs);
    query.exec();
    int singerId = query.lastInsertId().toInt();

    if (singerId == -1)
    {
        qCritical() << "ERROR ADDING SINGER TO DB!!!";
        return -1;
    }
    emit layoutAboutToBeChanged();
    m_singers.emplace_back(RotationSinger{
                              singerId,
                               name,
                               addPos,
                               false,
                               curTs
                           });
    emit layoutChanged();

    int curSingerPos = getSingerPosition(m_currentSingerId);
    switch (positionHint) {
    case ADD_FAIR:
    {
        if (curSingerPos > 0 && !settings.rotationAltSortOrder())
            singerMove(addPos, getSingerPosition(m_currentSingerId));
        break;
    }
    case ADD_NEXT:
        if (curSingerPos != rowCount() - 2)
            singerMove(addPos, getSingerPosition(m_currentSingerId) + 1);
        break;
    }

    emit rotationModified();
    outputRotationDebug();
    return singerId;
}

void TableModelRotation::singerMove(const int oldPosition, const int newPosition, const bool skipCommit)
{
    qInfo() << "singerMove called - oldpos: " << oldPosition << " newpos: " << newPosition;
    if (oldPosition == newPosition)
        return;
    emit layoutAboutToBeChanged();
    if (oldPosition > newPosition)
    {
        // moving up
        std::for_each(m_singers.begin(), m_singers.end(), [&oldPosition, &newPosition] (RotationSinger &singer) {
           if (singer.position == oldPosition)
               singer.position = newPosition;
           else if(singer.position >= newPosition && singer.position < oldPosition)
               singer.position++;
        });
    }
    else
    {
        // moving down
        std::for_each(m_singers.begin(), m_singers.end(), [&oldPosition, &newPosition] (RotationSinger &singer) {
            if (singer.position == oldPosition)
                singer.position = newPosition;
            else if (singer.position > oldPosition && singer.position <= newPosition)
                singer.position--;
        });
    }
    std::sort(m_singers.begin(), m_singers.end(), [] (RotationSinger &a, RotationSinger &b) {
        return (a.position < b.position);
    });
    if (!skipCommit)
        commitChanges();
    emit layoutChanged();
    emit rotationModified();
    outputRotationDebug();
}

void TableModelRotation::singerSetName(const int singerId, const QString &newName)
{
    auto it = std::find_if(m_singers.begin(), m_singers.end(), [&singerId] (RotationSinger &singer){
       return (singer.id == singerId);
    });
    if (it == m_singers.end())
    {
        qCritical() << "singerSetName - Unable to find singer!!!";
        return;
    }
    it->name = newName;
    emit dataChanged(this->index(it->position, COL_NAME), this->index(it->position, COL_NAME),QVector<int>{Qt::DisplayRole});
    QSqlQuery query;
    query.prepare("UPDATE rotationsingers SET name = :name WHERE singerid = :singerid");
    query.bindValue(":name", newName);
    query.bindValue(":singerid", singerId);
    query.exec();
    emit rotationModified();
    outputRotationDebug();
}

void TableModelRotation::singerDelete(const int singerId)
{
    emit layoutAboutToBeChanged();
    auto it = std::remove_if(m_singers.begin(), m_singers.end(), [&singerId] (RotationSinger &singer) {
       return (singer.id == singerId);
    });
    m_singers.erase(it, m_singers.end());
    int pos{0};
    std::for_each(m_singers.begin(), m_singers.end(), [&pos] (RotationSinger &singer) {
       singer.position = pos++;
    });
    emit layoutChanged();
    commitChanges();
    outputRotationDebug();
}

bool TableModelRotation::singerExists(const QString &name)
{
    auto it = std::find_if(m_singers.begin(), m_singers.end(), [&name] (RotationSinger &singer) {
        return (name.toLower() == singer.name.toLower());
    });
    return (it != m_singers.end());
}

bool TableModelRotation::singerIsRegular(const int singerId)
{
    auto it = std::find_if(m_singers.begin(), m_singers.end(), [&singerId] (RotationSinger &singer) {
        return (singerId == singer.id);
    });
    if (it == m_singers.end())
        return false;
    return it->regular;
}

void TableModelRotation::singerSetRegular(const int singerId, const bool isRegular)
{
    auto it = std::find_if(m_singers.begin(), m_singers.end(), [&singerId] (RotationSinger &singer) {
        return (singerId == singer.id);
    });
    it->regular = isRegular;
    emit dataChanged(this->index(it->position, COL_REGULAR), this->index(it->position, COL_REGULAR),QVector<int>{Qt::DisplayRole});
    QSqlQuery query;
    query.prepare("UPDATE rotationsingers SET regular = :regular WHERE singerid = :singerid");
    query.bindValue(":regular", isRegular);
    query.bindValue(":singerid", singerId);
    query.exec();
}

void TableModelRotation::singerMakeRegular(const int singerId)
{
    singerSetRegular(singerId, true);
}

void TableModelRotation::singerDisableRegularTracking(const int singerId)
{
    singerSetRegular(singerId, false);
}

bool TableModelRotation::historySingerExists(const QString &name)
{
    auto hSingers = historySingers();
    auto it = std::find_if(hSingers.begin(), hSingers.end(), [&name] (QString &hSinger) {
        return (name.toLower() == hSinger.toLower());
    });
    return (it != hSingers.end());
}

QString TableModelRotation::getSingerName(const int singerId)
{
    auto it = std::find_if(m_singers.begin(), m_singers.end(), [&singerId] (RotationSinger &singer) {
        return (singerId == singer.id);
    });
    if (it == m_singers.end())
        return QString();
    return it->name;
}

int TableModelRotation::getSingerId(const QString &name)
{
    auto it = std::find_if(m_singers.begin(), m_singers.end(), [&name] (RotationSinger &singer) {
        return (singer.name.toLower() == name.toLower());
    });
    if (it == m_singers.end())
        return -1;
    return it->id;
}

int TableModelRotation::getSingerPosition(const int singerId) const
{
    auto it = std::find_if(m_singers.begin(), m_singers.end(), [&singerId] (RotationSinger singer) {
        return (singerId == singer.id);
    });
    if (it == m_singers.end())
        return -1;
    return it->position;
}

int TableModelRotation::singerIdAtPosition(int position) const
{
    auto it = std::find_if(m_singers.begin(), m_singers.end(), [&position] (const RotationSinger &singer) {
        return (singer.position == position);
    });
    if (it == m_singers.end())
        return -1;
    return it->id;
}

QStringList TableModelRotation::singers()
{
    QStringList names;
    names.reserve(m_singers.size());
    std::for_each(m_singers.begin(), m_singers.end(), [&names] (RotationSinger &singer) {
        names.push_back(singer.name);
    });
    return names;
}

QStringList TableModelRotation::historySingers() const
{
    QStringList names;
    QSqlQuery query;
    query.exec("SELECT name FROM historySingers");
    while (query.next())
        names << query.value(0).toString();
    return names;
}

QString TableModelRotation::nextSongPath(const int singerId) const
{
    QSqlQuery query;
    query.prepare("SELECT dbsongs.path FROM dbsongs,queuesongs WHERE queuesongs.singer = :singerid AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
    query.bindValue(":singerid", singerId);
    query.exec();
    if (query.first())
        return query.value(0).toString();
    return QString();
}

QString TableModelRotation::nextSongArtist(const int singerId) const
{
    QSqlQuery query;
    query.prepare("SELECT dbsongs.artist FROM dbsongs,queuesongs WHERE queuesongs.singer = :singerid AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
    query.bindValue(":singerid", singerId);
    query.exec();
    if (query.first())
        return query.value(0).toString();
    return QString();
}

QString TableModelRotation::nextSongTitle(const int singerId) const
{
    QSqlQuery query;
    query.prepare("SELECT dbsongs.title FROM dbsongs,queuesongs WHERE queuesongs.singer = :singerid AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
    query.bindValue(":singerid", singerId);
    query.exec();
    if (query.first())
        return query.value(0).toString();
    return QString();
}

QString TableModelRotation::nextSongSongId(const int singerId) const
{
    QSqlQuery query;
    query.prepare("SELECT dbsongs.discid FROM dbsongs,queuesongs WHERE queuesongs.singer = :singerid AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
    query.bindValue(":singerid", singerId);
    query.exec();
    if (query.first())
        return query.value(0).toString();
    return QString();
}

int TableModelRotation::nextSongDurationSecs(const int singerId) const
{
    QSqlQuery query;
    query.prepare("SELECT dbsongs.duration FROM dbsongs,queuesongs WHERE queuesongs.singer = :singerid AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
    query.bindValue(":singerid", singerId);
    query.exec();
    if (query.first())
        return (query.value(0).toInt() / 1000) + settings.estimationSingerPad();
    else if (!settings.estimationSkipEmptySingers())
        return settings.estimationEmptySongLength() + settings.estimationSingerPad();
    return 0;
}

int TableModelRotation::rotationDuration()
{
    int secs = 0;
    std::for_each(m_singers.begin(), m_singers.end(), [&] (RotationSinger &singer) {
        secs += nextSongDurationSecs(singer.id);
    });
    return secs;
}

int TableModelRotation::nextSongKeyChg(const int singerId) const
{
    QSqlQuery query;
    query.prepare("SELECT keychg FROM queuesongs WHERE singer = :singerid AND played = 0 ORDER BY position LIMIT 1");
    query.bindValue(":singerid", singerId);
    query.exec();
    if (query.first())
        return query.value(0).toInt();
    return 0;
}

int TableModelRotation::nextSongQueueId(const int singerId) const
{
    QSqlQuery query;
    query.prepare("SELECT qsongid FROM queuesongs WHERE singer = :singerid AND played = 0 ORDER BY position LIMIT 1");
    query.bindValue(":singerid", singerId);
    query.exec();
    if (query.first())
        return query.value(0).toInt();
    return -1;
}

void TableModelRotation::clearRotation()
{
    emit layoutAboutToBeChanged();
    QSqlQuery query;
    query.exec("DELETE from queuesongs");
    query.exec("DELETE FROM rotationsingers");
    m_singers.clear();
    settings.setCurrentRotationPosition(-1);
    m_currentSingerId = -1;
    emit layoutChanged();
    emit rotationModified();
}

int TableModelRotation::currentSinger() const
{
    return m_currentSingerId;
}

void TableModelRotation::setCurrentSinger(const int currentSingerId)
{
    emit layoutAboutToBeChanged();
    m_currentSingerId = currentSingerId;
    emit rotationModified();
    emit layoutChanged();
    settings.setCurrentRotationPosition(currentSingerId);
}

bool TableModelRotation::rotationIsValid()
{
    return true;
}

int TableModelRotation::numSongs(const int singerId) const
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(qsongid) FROM queuesongs WHERE singer = :singerid");
    query.bindValue(":singerid", singerId);
    query.exec();
    if (query.first())
        return query.value(0).toInt();
    return -1;
}

int TableModelRotation::numSongsSung(const int singerId) const
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(qsongid) FROM queuesongs WHERE singer = :singerid AND played = true");
    query.bindValue(":singerid", singerId);
    query.exec();
    if (query.first())
        return query.value(0).toInt();
    return -1;
}

int TableModelRotation::numSongsUnsung(const int singerId) const
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(qsongid) FROM queuesongs WHERE singer = :singerid AND played = false");
    query.bindValue(":singerid", singerId);
    query.exec();
    if (query.first())
        return query.value(0).toInt();
    return -1;
}

QDateTime TableModelRotation::timeAdded(const int singerId)
{
    auto it = std::find_if(m_singers.begin(), m_singers.end(), [&singerId] (RotationSinger &singer) {
        return (singerId == singer.id);
    });
    if (it == m_singers.end())
        return QDateTime();
    return it->addTs;
}

void TableModelRotation::outputRotationDebug()
{
    qInfo() << " -- Rotation debug output -- ";
    qInfo() << "singerid,position,regular,added,name";
    int expectedPosition = 0;
    bool needsRepair = false;
    std::for_each(m_singers.begin(), m_singers.end(), [&expectedPosition, &needsRepair] (RotationSinger &singer) {
        qInfo() << singer.id << "," << singer.position << "," << singer.regular << "," << singer.addTs << "," << singer.name;
        if (singer.position != expectedPosition)
        {
            needsRepair = true;
            qInfo() << "ERROR DETECTED!!! - Singer position does not match expected position";
        }
        expectedPosition++;
    });
    if (needsRepair)
        fixSingerPositions();
    qInfo() << " -- Rotation debug output end -- ";
}

void TableModelRotation::fixSingerPositions()
{
    emit layoutAboutToBeChanged();
    int pos{0};
    std::for_each(m_singers.begin(), m_singers.end(), [&pos] (RotationSinger &singer) {
       singer.position = pos++;
    });
    emit layoutChanged();
    commitChanges();
}

void TableModelRotation::resizeIconsForFont(const QFont &font)
{
    m_curFontHeight = QFontMetrics(font).height();
    m_iconGreenCircle = QImage(m_curFontHeight / 3, m_curFontHeight / 3, QImage::Format_ARGB32);
    m_iconYellowCircle = QImage(m_curFontHeight / 3, m_curFontHeight / 3, QImage::Format_ARGB32);
    m_iconGreenCircle.fill(Qt::transparent);
    m_iconYellowCircle.fill(Qt::transparent);
    QPainter painterGreenCircle(&m_iconGreenCircle);
    QPainter painterYellowCircle(&m_iconYellowCircle);
    painterGreenCircle.setBrush(Qt::green);
    painterGreenCircle.drawEllipse(m_iconGreenCircle.rect().center(), m_curFontHeight / 6, m_curFontHeight / 6);
    painterYellowCircle.setBrush(Qt::darkGray);
    painterYellowCircle.drawEllipse(m_iconYellowCircle.rect().center(), m_curFontHeight / 6, m_curFontHeight / 6);
}

void ItemDelegateRotation::resizeIconsForFont(const QFont &font)
{
    QString thm = (settings.theme() == 1) ? ":/theme/Icons/okjbreeze-dark/" : ":/theme/Icons/okjbreeze/";
    m_curFontHeight = QFontMetrics(font).height();
    m_iconDelete = QImage(m_curFontHeight, m_curFontHeight, QImage::Format_ARGB32);
    m_iconCurSinger = QImage(m_curFontHeight, m_curFontHeight, QImage::Format_ARGB32);
    m_iconRegularOn = QImage(m_curFontHeight, m_curFontHeight, QImage::Format_ARGB32);
    m_iconRegularOff = QImage(m_curFontHeight, m_curFontHeight, QImage::Format_ARGB32);
    m_iconDelete.fill(Qt::transparent);
    m_iconCurSinger.fill(Qt::transparent);
    m_iconRegularOn.fill(Qt::transparent);
    m_iconRegularOff.fill(Qt::transparent);
    QPainter painterDelete(&m_iconDelete);
    QPainter painterPlaying(&m_iconCurSinger);
    QPainter painterRegularOn(&m_iconRegularOn);
    QPainter painterRegularOff(&m_iconRegularOff);
    QSvgRenderer svgrndrDelete(thm + "actions/16/edit-delete.svg");
    QSvgRenderer svgrndrCurSinger(thm + "status/16/mic-on.svg");
    QSvgRenderer svgrndrRegularOn(thm + "actions/16/im-user-online.svg");
    QSvgRenderer svgrndrRegularOff(thm + "actions/16/im-user.svg");
    svgrndrDelete.render(&painterDelete);
    svgrndrCurSinger.render(&painterPlaying);
    svgrndrRegularOn.render(&painterRegularOn);
    svgrndrRegularOff.render(&painterRegularOff);
}

ItemDelegateRotation::ItemDelegateRotation(QObject *parent) :
    QItemDelegate(parent)
{
    resizeIconsForFont(settings.applicationFont());
    connect(&settings, &Settings::applicationFontChanged, this, &ItemDelegateRotation::resizeIconsForFont);
}

void ItemDelegateRotation::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() == TableModelRotation::COL_ID)
    {
        if (index.data(Qt::UserRole).toInt() == m_currentSinger)
        {
            int topPad = (option.rect.height() - m_curFontHeight) / 2;
            int leftPad = (option.rect.width() - m_curFontHeight) / 2;
            painter->drawImage(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, m_curFontHeight, m_curFontHeight),m_iconCurSinger);
            return;
        }
    }
    else if (index.column() == TableModelRotation::COL_REGULAR)
    {
        int topPad = (option.rect.height() - m_curFontHeight) / 2;
        int leftPad = (option.rect.width() - m_curFontHeight) / 2;
        if (index.data().toBool())
            painter->drawImage(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, m_curFontHeight, m_curFontHeight),m_iconRegularOn);
        else
            painter->drawImage(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, m_curFontHeight, m_curFontHeight),m_iconRegularOff);
        return;
    }
    else if (index.column() == TableModelRotation::COL_DELETE)
    {
        int topPad = (option.rect.height() - m_curFontHeight) / 2;
        int leftPad = (option.rect.width() - m_curFontHeight) / 2;
        painter->drawImage(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, m_curFontHeight, m_curFontHeight),m_iconDelete);
        return;
    }
    QItemDelegate::paint(painter, option, index);
}

int ItemDelegateRotation::currentSinger() const
{
    return m_currentSinger;
}

void ItemDelegateRotation::setCurrentSinger(const int singerId)
{
    m_currentSinger = singerId;
}


QStringList TableModelRotation::mimeTypes() const
{
    QStringList types;
    types << "integer/songid";
    types << "integer/rotationpos";
    types << "application/rotsingers";
    return types;
}

QMimeData *TableModelRotation::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    mimeData->setData("integer/rotationpos", indexes.at(0).sibling(indexes.at(0).row(), COL_POSITION).data().toByteArray().data());
    if (indexes.size() > 1)
    {
        QJsonArray jArr;
        std::for_each(indexes.begin(), indexes.end(), [&] (QModelIndex index) {
            // only act on one column to avoid duplicate singerids in the list for each col
            if (index.column() != COL_NAME)
                return;
            jArr.append(index.data(Qt::UserRole).toInt());
        });
        QJsonDocument jDoc(jArr);
        mimeData->setData("application/rotsingers", jDoc.toJson());
        qInfo() << "Rotation singers mime data: " << jDoc.toJson();
    }
    return mimeData;
}

bool TableModelRotation::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(action);
    Q_UNUSED(column);
    Q_UNUSED(row);
    if (parent.row() == -1  && !data->hasFormat("integer/rotationpos"))
        return false;
    if ((data->hasFormat("integer/songid")) || (data->hasFormat("integer/rotationpos")))
        return true;
    return false;
}

bool TableModelRotation::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(column);
    if (action == Qt::MoveAction && data->hasFormat("application/rotsingers"))
    {
        QJsonDocument jDoc = QJsonDocument::fromJson(data->data("application/rotsingers"));
        QJsonArray jArr = jDoc.array();
        auto ids = jArr.toVariantList();
        qInfo() << "mime data dropped: " << jDoc.toJson();
        int droprow{0};
        if (parent.row() >= 0)
            droprow = parent.row();
        else if (row >= 0)
            droprow = row;
        else
            droprow = rowCount() - 1;
        if (getSingerPosition(ids.at(0).toInt()) > droprow)
            std::reverse(ids.begin(),ids.end());
        std::for_each(ids.begin(), ids.end(), [&] (auto val) {
            singerMove(getSingerPosition(val.toInt()), droprow, false);
        });
        commitChanges();
        qInfo() << "droprow: " << droprow;
        emit rotationModified();
        if (droprow == rowCount() - 1)
        {
            qInfo() << "moving to bottom";
            // moving to bottom
            emit singersMoved(rowCount() - ids.size(), 0, rowCount() - 1, columnCount() - 1);
        }
        else if (getSingerPosition(ids.at(0).toInt()) < droprow)
        {
            // moving down
            emit singersMoved(droprow - ids.size() + 1, 0, droprow, columnCount() - 1);
        }
        else
        {
            // moving up
            emit singersMoved(droprow, 0, droprow + ids.size() - 1, columnCount() - 1);
        }
        return true;
    }

    if (data->hasFormat("integer/rotationpos"))
    {
        int droprow;
        if (parent.row() >= 0)
            droprow = parent.row();
        else if (row >= 0)
            droprow = row;
        else
            droprow = rowCount() - 1;
        int oldPosition;
        QByteArray bytedata = data->data("integer/rotationpos");
        oldPosition =  QString(bytedata.data()).toInt();
        if (droprow == oldPosition)
        {
            // Singer dropped, but would result in same position, ignore to prevent rotation corruption.
            return false;
        }
        if ((oldPosition < droprow) && (droprow != rowCount() - 1))
            singerMove(oldPosition, droprow);
        else
            singerMove(oldPosition, droprow);
        emit singersMoved(droprow, 0, droprow, columnCount() - 1);
        return true;
    }


    if (data->hasFormat("integer/songid"))
    {
        unsigned int dropRow;
        if (parent.row() >= 0)
            dropRow = parent.row();
        else if (row >= 0)
            dropRow = row;
        else
            dropRow = rowCount();
        int songId = data->data("integer/songid").toInt();
        int singerId = index(dropRow,0).data(Qt::UserRole).toInt();
        emit songDroppedOnSinger(singerId, songId, dropRow);
    }
    return false;
}

Qt::DropActions TableModelRotation::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

Qt::ItemFlags TableModelRotation::flags([[maybe_unused]]const QModelIndex &index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

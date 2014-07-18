#include "rotationmodel.h"
#include <QSqlQuery>
#include <QDebug>


int RotationModel::currentSinger() const
{
    return m_currentSingerId;
}

void RotationModel::setCurrentSinger(int currentSingerId)
{
    emit layoutAboutToBeChanged();
    m_currentSingerId = currentSingerId;
    emit rotationModified();
    emit layoutChanged();
}

RotationModel::RotationModel(QObject *parent, QSqlDatabase db) :
    QSqlTableModel(parent, db)
{
    m_currentSingerId = -1;
    setTable("rotationsingers");
    sort(2, Qt::AscendingOrder);
}

void RotationModel::singerAdd(QString name)
{
    QSqlQuery query;
    QString sql = "INSERT INTO rotationsingers (name,position,regular,regularid) VALUES(\"" + name + "\"," + QString::number(rowCount()) + ",0,-1)";
    query.exec(sql);
    qDebug() << sql;
    select();
    emit rotationModified();
}

void RotationModel::singerMove(int oldPosition, int newPosition)
{
    qDebug() << "moveSinger(" << oldPosition << "," << newPosition << ")";
    if (oldPosition == newPosition)
        return;
    QSqlQuery query;
    QString sql;
    int qSingerId = index(oldPosition,0).data().toInt();
    query.exec("BEGIN TRANSACTION");
    if (newPosition > oldPosition)
    {
        // Moving down
        sql = "UPDATE rotationsingers SET position = position - 1 WHERE position > " + QString::number(oldPosition) + " AND position <= " + QString::number(newPosition) + " AND singerid != " + QString::number(qSingerId);
    }
    else if (newPosition < oldPosition)
    {
        // Moving up
        sql = "UPDATE rotationsingers SET position = position + 1 WHERE position >= " + QString::number(newPosition) + " AND position < " + QString::number(oldPosition) + " AND singerid != " + QString::number(qSingerId);
    }
    query.exec(sql);
    sql = "UPDATE rotationsingers SET position = " + QString::number(newPosition) + " WHERE singerid == " + QString::number(qSingerId);
    query.exec(sql);
    query.exec("COMMIT TRANSACTION");
    select();
    emit rotationModified();
}

void RotationModel::singerDelete(int singerId)
{
    int position = getSingerPosition(singerId);
    QSqlQuery query;
    QString sql = "DELETE FROM queuesongs WHERE singer == " + QString::number(singerId);
    query.exec(sql);
    sql = "UPDATE rotationsingers SET position = position - 1 WHERE position > " + QString::number(position);
    query.exec(sql);
    sql = "DELETE FROM rotationsingers WHERE singerid == " + QString::number(singerId);
    query.exec(sql);
    select();
    emit rotationModified();
}

bool RotationModel::singerExists(QString name)
{
    QStringList names = singers();
    for (int i=0; i < names.size(); i++)
    {
        if (names.at(i).toLower() == name.toLower())
            return true;
    }
    return false;
}

QString RotationModel::getSingerName(int singerId)
{
    QString sql = "select name from rotationsingers WHERE singerid = " + QString::number(singerId) + " LIMIT 1";
    QSqlQuery query(sql);
    if (query.first())
        return query.value(0).toString();

    return QString();
}

int RotationModel::getSingerId(QString name)
{
    QString sql = "select singerid from rotationsingers WHERE name == \"" + name + "\" LIMIT 1";
    QSqlQuery query(sql);
    if (query.first())
        return query.value(0).toInt();

    return -1;
}

int RotationModel::getSingerPosition(int singerId)
{
    QString sql = "select position from rotationsingers WHERE singerid = " + QString::number(singerId) + " LIMIT 1";
    QSqlQuery query(sql);
    if (query.first())
        return query.value(0).toInt();

    return -1;
}

int RotationModel::singerIdAtPosition(int position)
{
    QString sql = "SELECT singerId FROM rotationsingers WHERE position == " + QString::number(position) + " LIMIT 1";
    QSqlQuery query(sql);
    if (query.first())
        return query.value(0).toInt();

    return -1;
}

QStringList RotationModel::singers()
{
    QStringList singers;
    for (int i=0; i < rowCount(); i++)
    {
        singers << index(i,1).data().toString();
    }
    return singers;
}

QString RotationModel::nextSongPath(int singerId)
{
    QString sql = "select dbsongs.path from dbsongs,queuesongs WHERE queuesongs.singer = " + QString::number(singerId) + " AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1";
    QSqlQuery query(sql);
    if (query.first())
        return query.value(0).toString();

    return QString();
}

QString RotationModel::nextSongArtist(int singerId)
{
    QString sql = "select dbsongs.artist from dbsongs,queuesongs WHERE queuesongs.singer = " + QString::number(singerId) + " AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1";
    QSqlQuery query(sql);
    if (query.first())
        return query.value(0).toString();

    return QString();
}

QString RotationModel::nextSongTitle(int singerId)
{
    QString sql = "select dbsongs.title from dbsongs,queuesongs WHERE queuesongs.singer = " + QString::number(singerId) + " AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1";
    QSqlQuery query(sql);
    if (query.first())
        return query.value(0).toString();

    return QString();
}

int RotationModel::nextSongId(int singerId)
{
    QString sql = "select dbsongs.songid from dbsongs,queuesongs WHERE queuesongs.singer = " + QString::number(singerId) + " AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1";
    QSqlQuery query(sql);
    if (query.first())
        return query.value(0).toInt();

    return -1;
}

int RotationModel::nextSongQueueId(int singerId)
{
    QString sql = "select qsongid from queuesongs WHERE singer = " + QString::number(singerId) + " AND played = 0 ORDER BY position LIMIT 1";
    QSqlQuery query(sql);
    if (query.first())
        return query.value(0).toInt();

    return -1;
}

void RotationModel::queueModified()
{
    emit layoutAboutToBeChanged();
    emit layoutChanged();
}


QStringList RotationModel::mimeTypes() const
{
    QStringList types;
    types << "integer/songid";
    types << "integer/rotationpos";
    return types;
}

QMimeData *RotationModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    mimeData->setData("integer/rotationpos", indexes.at(0).sibling(indexes.at(0).row(), 2).data().toByteArray().data());
    return mimeData;
}

bool RotationModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(column);
    Q_UNUSED(parent);
    if ((data->hasFormat("integer/songid")) || (data->hasFormat("integer/rotationpos")))
        return true;

    return false;
}

bool RotationModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(action);
    Q_UNUSED(column);

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
        if ((oldPosition < droprow) && (droprow != rowCount() - 1))
            singerMove(oldPosition, droprow - 1);
        else
            singerMove(oldPosition, droprow);
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
        int singerId = index(dropRow,0).data().toInt();
        emit songDroppedOnSinger(singerId, songId, dropRow);
    }
    return false;
}

Qt::ItemFlags RotationModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable;
}

Qt::DropActions RotationModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

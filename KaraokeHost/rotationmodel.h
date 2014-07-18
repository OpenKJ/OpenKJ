#ifndef ROTATIONMODEL_H
#define ROTATIONMODEL_H

#include <QSqlTableModel>
#include <QMimeData>
#include <QStringList>

class RotationModel : public QSqlTableModel
{
    Q_OBJECT
private:
    int m_currentSingerId;
public:
    explicit RotationModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase());
    void singerAdd(QString name);
    void singerMove(int oldPosition, int newPosition);
    void singerDelete(int singerId);
    bool singerExists(QString name);
    QString getSingerName(int singerId);
    int getSingerId(QString name);
    int getSingerPosition(int singerId);
    int singerIdAtPosition(int position);
    QStringList singers();
    QString nextSongPath(int singerId);
    QString nextSongArtist(int singerId);
    QString nextSongTitle(int singerId);
    int nextSongId(int singerId);
    int nextSongQueueId(int singerId);
    void clearRotation();

signals:
    void songDroppedOnSinger(int singerId, int songId, int dropRow);
    void rotationModified();

public slots:
    void queueModified();


    // QAbstractItemModel interface
public:
    QStringList mimeTypes() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    Qt::DropActions supportedDropActions() const;
    int currentSinger() const;
    void setCurrentSinger(int currentSingerId);
};

#endif // ROTATIONMODEL_H

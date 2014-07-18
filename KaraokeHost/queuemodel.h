#ifndef QUEUEMODEL_H
#define QUEUEMODEL_H

#include <QSqlRelationalTableModel>
#include <QMimeData>
#include <QStringList>

class QueueModel : public QSqlRelationalTableModel
{
    Q_OBJECT
    int m_singerId;
public:
    explicit QueueModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase());
    void setSinger(int singerId);
    int singer();
    int getSongPosition(int songId);
    bool getSongPlayed(int songId);
    int getSongKey(int songId);
    void songMove(int oldPosition, int newPosition);
    void songAdd(int songId);
    void songInsert(int songId, int position);
    void songDelete(int songId);
    void songSetKey(int songId, int semitones);
    void songSetPlayed(int qSongId, bool played = true);
    void clearQueue();
    QStringList mimeTypes() const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const;
    Qt::DropActions supportedDropActions() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

signals:
    void queueModified();

public slots:
    void songAdd(int songId, int singerId);

};

#endif // QUEUEMODEL_H

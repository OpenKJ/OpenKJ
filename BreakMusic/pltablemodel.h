#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QSqlRelationalTableModel>
#include <QMimeData>
#include <QStringList>

class PlTableModel : public QSqlRelationalTableModel
{
    Q_OBJECT
private:
    int m_playlistId;

public:
    explicit PlTableModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase());
    void moveSong(int oldPosition, int newPosition);
    void addSong(int songId);
    void insertSong(int songId, int position);
    void deleteSong(int position);
    void setCurrentPlaylist(int playlistId);
    int currentPlaylist();
    int getSongIdByFilePath(QString filePath);
    QString currentSongString();
    QString nextSongString();

signals:

public slots:


    // QAbstractItemModel interface
public:
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

    // QAbstractItemModel interface
public:
    QStringList mimeTypes() const;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const;
    Qt::DropActions supportedDropActions() const;

    // QAbstractItemModel interface
public:
    QMimeData *mimeData(const QModelIndexList &indexes) const;


    // QAbstractItemModel interface
public:
    Qt::ItemFlags flags(const QModelIndex &index) const;
};

#endif // PLAYLISTMODEL_H

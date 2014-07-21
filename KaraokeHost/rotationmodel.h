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
    enum {ADD_FAIR=0,ADD_BOTTOM,ADD_NEXT};
    int singerAdd(QString name);
    void singerMove(int oldPosition, int newPosition);
    void singerDelete(int singerId);
    bool singerExists(QString name);
    bool singerIsRegular(int singerId);
    int singerRegSingerId(int singerId);
    void singerMakeRegular(int singerId);
    void singerDisableRegularTracking(int singerId);
    int regularAdd(QString name);
    void regularDelete(int regSingerId);
    bool regularExists(QString name);
    void regularUpdate(int singerId);
    void regularLoad(int regSingerId, int positionHint);
    QString getSingerName(int singerId);
    QString getRegularName(int regSingerId);
    int getSingerId(QString name);
    QString getRegSingerId(QString name);
    int getSingerPosition(int singerId);
    int singerIdAtPosition(int position);
    QStringList singers();
    QStringList regulars();
    QString nextSongPath(int singerId);
    QString nextSongArtist(int singerId);
    QString nextSongTitle(int singerId);
    int nextSongId(int singerId);
    int nextSongQueueId(int singerId);
    void clearRotation();

signals:
    void songDroppedOnSinger(int singerId, int songId, int dropRow);
    void regularAddNameConflict(QString name);
    void regularLoadNameConflict(QString name);
    void regularAddError(QString errorText);
    void rotationModified();
    void regularsModified();

public slots:
    void queueModified(int singerId);


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

#ifndef DBTABLEMODEL_H
#define DBTABLEMODEL_H

#include <QSqlTableModel>

class DbTableModel : public QSqlTableModel
{
    Q_OBJECT
public:
    explicit DbTableModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase());

signals:

public slots:


    // QAbstractItemModel interface
public:
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
};

#endif // DBTABLEMODEL_H

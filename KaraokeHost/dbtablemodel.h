#ifndef DBTABLEMODEL_H
#define DBTABLEMODEL_H

#include <QSqlTableModel>

class DbTableModel : public QSqlTableModel
{
    Q_OBJECT
private:
    int sortColumn;
    QString artistOrder;
    QString titleOrder;
    QString discIdOrder;
    QString durationOrder;

public:
    explicit DbTableModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase());
    enum {SORT_ARTIST=1,SORT_TITLE=2,SORT_DISCID=3,SORT_DURATION=4};
    void search(QString searchString);

signals:

public slots:


    // QAbstractItemModel interface
public:
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    void sort(int column, Qt::SortOrder order);


    // QSqlTableModel interface
protected:
    QString orderByClause() const;
};

#endif // DBTABLEMODEL_H

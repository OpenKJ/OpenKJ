#ifndef SONGSTABLEMODEL_H
#define SONGSTABLEMODEL_H

#include <QSqlTableModel>

class DbTableModel : public QSqlTableModel
{
    Q_OBJECT
private:
    int sortColumn;
    QString artistOrder;
    QString titleOrder;
    QString filenameOrder;
public:
    explicit DbTableModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase());
    void search(QString searchString);
    enum {SORT_ARTIST=1,SORT_TITLE=2,SORT_FILENAME=4,SORT_DURATION=5};

    // QAbstractItemModel interface
public:
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    void sort(int column, Qt::SortOrder order);
    Qt::ItemFlags flags(const QModelIndex &index) const;

    // QSqlTableModel interface
protected:
    QString orderByClause() const;

};

#endif // SONGSTABLEMODEL_H

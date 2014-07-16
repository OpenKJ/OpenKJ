#ifndef DBTABLEMODEL_H
#define DBTABLEMODEL_H

#include <QSqlTableModel>

class DbTableModel : public QSqlTableModel
{
    Q_OBJECT
public:
    explicit DbTableModel(QObject *parent = 0);

signals:

public slots:

};

#endif // DBTABLEMODEL_H

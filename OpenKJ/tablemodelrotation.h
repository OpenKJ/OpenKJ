#ifndef TABLEMODELROTATION_H
#define TABLEMODELROTATION_H

#include <QAbstractTableModel>
#include <QDateTime>

struct RotationSinger {
    int id{0};
    QString name;
    int position{0};
    bool regular{false};
    QDateTime addTs;
};

class TableModelRotation : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum {COL_ID=0,COL_NAME,COL_POSITION,COL_REGULAR,COL_ADDTS};
    explicit TableModelRotation(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void loadData();

private:
    std::vector<RotationSinger> m_singers;
};

#endif // TABLEMODELROTATION_H

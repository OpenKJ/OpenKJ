#ifndef HISTORYSINGERSTABLEMODEL_H
#define HISTORYSINGERSTABLEMODEL_H

#include <QAbstractTableModel>
#include <QIcon>

struct HistorySinger {
    int historySingerId{-1};
    QString name;
    int songCount{0};
};

class HistorySingersTableModel : public QAbstractTableModel
{
    Q_OBJECT
private:
    std::vector<HistorySinger> m_singers;
    QIcon m_iconDelete16;
    QIcon m_iconDelete22;
    QIcon m_iconLoadReg16;
    QIcon m_iconLoadReg22;
    QString m_filterString;

public:
    explicit HistorySingersTableModel(QObject *parent = nullptr);
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int getSongCount(const int historySingerId) const;
    void loadSingers();
    QString getName(const int historySingerId) const;
    bool exists(const QString &name) const;
    int getId(const QString &historySingerName) const;
    void deleteHistory(const int historySingerId);
    bool rename(const int historySingerId, const QString &newName);
    void filter(const QString &filterString);

signals:
    void historySingersModified();
};

#endif // HISTORYSINGERSTABLEMODEL_H

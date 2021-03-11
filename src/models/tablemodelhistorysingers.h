#ifndef HISTORYSINGERSTABLEMODEL_H
#define HISTORYSINGERSTABLEMODEL_H

#include <QAbstractTableModel>
#include <QIcon>
#include <QItemDelegate>

struct HistorySinger {
    int historySingerId{-1};
    QString name;
    int songCount{0};
};

class ItemDelegateHistorySingers : public QItemDelegate
{
    Q_OBJECT
private:
    QImage m_iconDelete;
    QImage m_iconLoadReg;
    int m_curFontHeight;
    void resizeIconsForFont(QFont font);

public:
    explicit ItemDelegateHistorySingers(QObject *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

};

class TableModelHistorySingers : public QAbstractTableModel
{
    Q_OBJECT
private:
    std::vector<HistorySinger> m_singers;
    QString m_filterString;

public:
    explicit TableModelHistorySingers(QObject *parent = nullptr);
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
    std::vector<HistorySinger> &singers();
    HistorySinger getSinger(const int historySingerId);

signals:
    void historySingersModified();

};

#endif // HISTORYSINGERSTABLEMODEL_H

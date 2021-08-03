#ifndef HISTORYSINGERSTABLEMODEL_H
#define HISTORYSINGERSTABLEMODEL_H

#include <QAbstractTableModel>
#include <QIcon>
#include <QItemDelegate>
#include "settings.h"

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
    int m_curFontHeight{0};
    Settings m_settings;

public:
    explicit ItemDelegateHistorySingers(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

public slots:
    void resizeIconsForFont(const QFont& font);

};

class TableModelHistorySingers : public QAbstractTableModel
{
    Q_OBJECT
private:
    std::vector<HistorySinger> m_singers;
    QString m_filterString;
    Settings m_settings;

public:
    explicit TableModelHistorySingers(QObject *parent = nullptr);
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] static int getSongCount(int historySingerId) ;
    void loadSingers();
    [[nodiscard]] QString getName(int historySingerId) const;
    [[nodiscard]] bool exists(const QString &name) const;
    [[nodiscard]] int getId(const QString &historySingerName) const;
    void deleteHistory(int historySingerId);
    bool rename(int historySingerId, const QString &newName);
    void filter(const QString &filterString);
    std::vector<HistorySinger> &singers();
    HistorySinger getSinger(int historySingerId);

signals:
    void historySingersModified();

};

#endif // HISTORYSINGERSTABLEMODEL_H

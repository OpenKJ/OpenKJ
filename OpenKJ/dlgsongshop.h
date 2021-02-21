#ifndef DLGSONGSHOP_H
#define DLGSONGSHOP_H

#include <QDialog>
#include "tablemodelsongshopsongs.h"
#include "dlgsongshoppurchase.h"
#include "settings.h"

extern Settings settings;

namespace Ui {
class DlgSongShop;
}

class DlgSongShop : public QDialog
{
    Q_OBJECT

public:
    explicit DlgSongShop(SongShop *songShop, QWidget *parent = 0);
    ~DlgSongShop();

private slots:
    void on_btnClose_clicked();

    void on_lineEditSearch_textChanged(const QString &arg1);

    void on_btnPurchase_clicked();

private:
    Ui::DlgSongShop *ui;
    TableModelSongShopSongs *modelSongs;
    SortFilterProxyModelSongShopSongs *sortFilterModel;
    DlgSongShopPurchase *dlgPurchase;
    SongShop *shop;

    // QWidget interface
public slots:
    void setVisible(bool visible);
    void autoSizeView();

signals:
    void karaokeSongDownloaded(QString path);

    // QWidget interface
protected:
    void resizeEvent(QResizeEvent *event);
};

#endif // DLGSONGSHOP_H

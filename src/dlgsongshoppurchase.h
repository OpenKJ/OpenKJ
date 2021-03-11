#ifndef DLGSONGSHOPPURCHASE_H
#define DLGSONGSHOPPURCHASE_H

#include <QDialog>
#include "songshop.h"
#include <QMessageBox>
#include "dlgpurchaseprogress.h"

namespace Ui {
class DlgSongShopPurchase;
}

class DlgSongShopPurchase : public QDialog
{
    Q_OBJECT

public:
    explicit DlgSongShopPurchase(SongShop *songShop, QWidget *parent = 0);
    ~DlgSongShopPurchase();
    void setArtist(QString artist);
    void setTitle(QString title);
    void setSongId(QString songId);
    void setPrice(QString price);
    void doAuth();

private slots:
    void on_pushButtonTestLogin_clicked();
    void knLoginSuccess();
    void knLoginFailure();
    void paymentProcessingFailed();
    void purchaseSuccess();

    void on_pushButtonCancel_clicked();

    void on_btnPurchase_clicked();

    void on_cbxSaveAccount_stateChanged(int arg1);

    void on_cbxSaveCard_stateChanged(int arg1);

    void on_lineEditKNUser_textChanged(const QString &arg1);

    void on_lineEditKNPass_textChanged(const QString &arg1);

    void on_lineEditCCN_textChanged(const QString &arg1);

    void on_lineEditCCM_textChanged(const QString &arg1);

    void on_lineEditCCY_textChanged(const QString &arg1);

    void on_lineEditCCV_textChanged(const QString &arg1);

    void on_lineEditCCN_cursorPositionChanged(int arg1, int arg2);

    void on_lineEditCCN_editingFinished();
    void downloadProgress(qint64 received, qint64 total);

private:
    Ui::DlgSongShopPurchase *ui;
    QString songId;
    QString artist;
    QString title;
    SongShop *shop;
    bool knLoginTest;
    bool authenticated;
    QString password;
    bool setupDone;
    DlgPurchaseProgress *msgBoxInfo;

    // QWidget interface
protected:
    void showEvent(QShowEvent *event);

    // QWidget interface
public slots:
    void setVisible(bool visible);
};

#endif // DLGSONGSHOPPURCHASE_H

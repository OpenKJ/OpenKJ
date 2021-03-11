#ifndef DLGPURCHASEPROGRESS_H
#define DLGPURCHASEPROGRESS_H

#include <QDialog>

namespace Ui {
class DlgPurchaseProgress;
}

class DlgPurchaseProgress : public QDialog
{
    Q_OBJECT

public:
    explicit DlgPurchaseProgress(QWidget *parent = 0);
    ~DlgPurchaseProgress();

private:
    Ui::DlgPurchaseProgress *ui;

public slots:
    void setText(QString message);
    void setProgress(qint64 current, qint64 total);
};

#endif // DLGPURCHASEPROGRESS_H

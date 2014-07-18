#ifndef DLGDURATIONSCAN_H
#define DLGDURATIONSCAN_H

#include <QDialog>
#include "khzip.h"
#include "khdb.h"
#include <QStringList>

namespace Ui {
class DlgDurationScan;
}

class DlgDurationScan : public QDialog
{
    Q_OBJECT

public:
    explicit DlgDurationScan(QWidget *parent = 0);
    ~DlgDurationScan();

private slots:
    void on_buttonClose_clicked();
    void on_buttonStop_clicked();
    void on_buttonStart_clicked();

private:
    QStringList findNeedUpdateSongs();
    Ui::DlgDurationScan *ui;
    KhZip zip;
    bool stopProcessing;
    KhDb db;
};

#endif // DLGDURATIONSCAN_H

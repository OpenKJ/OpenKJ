#ifndef DLGDURATIONSCAN_H
#define DLGDURATIONSCAN_H

#include <QDialog>
#include "khsong.h"
#include "khzip.h"
#include "khdb.h"

namespace Ui {
class DlgDurationScan;
}

class DlgDurationScan : public QDialog
{
    Q_OBJECT

public:
    explicit DlgDurationScan(KhSongs *songs,QWidget *parent = 0);
    ~DlgDurationScan();

private slots:
    void on_buttonClose_clicked();
    void on_buttonStop_clicked();
    void on_buttonStart_clicked();

private:
    void findNeedUpdateSongs();
    Ui::DlgDurationScan *ui;
    KhSongs *m_songs;
    KhSongs *m_needUpdate;
    KhZip zip;
    bool stopProcessing;
    KhDb db;
};

#endif // DLGDURATIONSCAN_H

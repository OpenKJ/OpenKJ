#ifndef KHREQUESTSDIALOG_H
#define KHREQUESTSDIALOG_H

#include <QDialog>
#include <QItemSelection>
#include "requeststablemodel.h"
//#include "songdbtablemodel.h"
#include "dbtablemodel.h"
#include "dbitemdelegate.h"
//#include "rotationtablemodel.h"
#include "rotationmodel.h"
//#include "khsinger.h"
#include "dlgcdgpreview.h"

namespace Ui {
class DlgRequests;
}

class DlgRequests : public QDialog
{
    Q_OBJECT

public:
    explicit DlgRequests(RotationModel *rotationModel, QWidget *parent = 0);
    ~DlgRequests();

private slots:
    void on_pushButtonClose_clicked();
    void requestsModified();

    void on_pushButtonSearch_clicked();

    void on_lineEditSearch_returnPressed();
    void requestSelectionChanged(const QItemSelection & current, const QItemSelection & previous);
    void songSelectionChanged(const QItemSelection & current, const QItemSelection & previous);
    void on_radioButtonExistingSinger_toggled(bool checked);

    void on_pushButtonClearReqs_clicked();

    void on_treeViewRequests_clicked(const QModelIndex &index);

    void on_pushButtonAddSong_clicked();

    void on_tableViewSearch_customContextMenuRequested(const QPoint &pos);
    void updateReceived(QTime updateTime);

    void on_buttonRefresh_clicked();
    void authError();
    void sslError();
    void delayError(int seconds);
    void on_checkBoxAccepting_clicked(bool checked);

signals:
    void addRequestSong(int songId, int singerId);

private:
    Ui::DlgRequests *ui;
    RequestsTableModel *requestsModel;
    //SongDBTableModel *songDbModel;
    DbTableModel *dbModel;
    DbItemDelegate *dbDelegate;
    RotationModel *rotModel;
    DlgCdgPreview *cdgPreviewDialog;
};

#endif // KHREQUESTSDIALOG_H

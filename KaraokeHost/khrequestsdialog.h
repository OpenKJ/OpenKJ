#ifndef KHREQUESTSDIALOG_H
#define KHREQUESTSDIALOG_H

#include <QDialog>
#include "requeststablemodel.h"
#include "songdbtablemodel.h"
#include "rotationtablemodel.h"
#include "khsinger.h"
#include "cdgpreviewdialog.h"

namespace Ui {
class KhRequestsDialog;
}

class KhRequestsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KhRequestsDialog(KhSongs *fullData,RotationTableModel *rotationModel,QWidget *parent = 0);
    ~KhRequestsDialog();

private slots:
    void on_pushButtonClose_clicked();
    void requestsModified();

    void on_pushButtonSearch_clicked();

    void on_lineEditSearch_returnPressed();
    void requestSelectionChanged(const QModelIndex & current, const QModelIndex & previous);
    void songSelectionChanged(const QModelIndex & current, const QModelIndex & previous);

    void on_radioButtonExistingSinger_toggled(bool checked);

    void on_pushButtonClearReqs_clicked();

    void on_treeViewRequests_clicked(const QModelIndex &index);

    void on_pushButtonAddSong_clicked();

    void on_treeViewSearch_customContextMenuRequested(const QPoint &pos);

private:
    Ui::KhRequestsDialog *ui;
    RequestsTableModel *requestsModel;
    SongDBTableModel *songDbModel;
    RotationTableModel *m_rotationModel;
    CdgPreviewDialog *cdgPreviewDialog;
};

#endif // KHREQUESTSDIALOG_H

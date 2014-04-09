#ifndef KHREQUESTSDIALOG_H
#define KHREQUESTSDIALOG_H

#include <QDialog>
#include <requeststablemodel.h>
#include <songdbtablemodel.h>

namespace Ui {
class KhRequestsDialog;
}

class KhRequestsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KhRequestsDialog(KhSongs *fullData, QWidget *parent = 0);
    ~KhRequestsDialog();

private slots:
    void on_pushButtonClose_clicked();
    void requestsModified();

    void on_pushButtonSearch_clicked();

    void on_lineEditSearch_returnPressed();
    void requestSelectionChanged(const QModelIndex & current, const QModelIndex & previous);

private:
    Ui::KhRequestsDialog *ui;
    RequestsTableModel *requestsModel;
    SongDBTableModel *songDbModel;
};

#endif // KHREQUESTSDIALOG_H

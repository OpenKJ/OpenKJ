#ifndef KHREQUESTSDIALOG_H
#define KHREQUESTSDIALOG_H

#include <QDialog>
#include <requeststablemodel.h>

namespace Ui {
class KhRequestsDialog;
}

class KhRequestsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KhRequestsDialog(QWidget *parent = 0);
    ~KhRequestsDialog();

private slots:
    void on_pushButtonClose_clicked();

private:
    Ui::KhRequestsDialog *ui;
    RequestsTableModel *requestsModel;
};

#endif // KHREQUESTSDIALOG_H

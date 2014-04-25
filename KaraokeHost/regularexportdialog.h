#ifndef REGULAREXPORTDIALOG_H
#define REGULAREXPORTDIALOG_H

#include <QDialog>
#include "khregularsinger.h"
#include "regularsingermodel.h"

namespace Ui {
class RegularExportDialog;
}

class RegularExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegularExportDialog(KhRegularSingers *regularSingers, QWidget *parent = 0);
    ~RegularExportDialog();

private slots:
    void on_pushButtonClose_clicked();

    void on_pushButtonExport_clicked();

private:
    Ui::RegularExportDialog *ui;
    RegularSingerModel *regSingersModel;
    KhRegularSingers *regSingers;
};

#endif // REGULAREXPORTDIALOG_H

#ifndef REGULAREXPORTDIALOG_H
#define REGULAREXPORTDIALOG_H

#include <QDialog>
#include "khregularsinger.h"
#include "regularsingermodel.h"

namespace Ui {
class DlgRegularExport;
}

class DlgRegularExport : public QDialog
{
    Q_OBJECT

public:
    explicit DlgRegularExport(KhRegularSingers *regularSingers, QWidget *parent = 0);
    ~DlgRegularExport();

private slots:
    void on_pushButtonClose_clicked();

    void on_pushButtonExport_clicked();

    void on_pushButtonExportAll_clicked();

private:
    Ui::DlgRegularExport *ui;
    RegularSingerModel *regSingersModel;
    KhRegularSingers *regSingers;
};

#endif // REGULAREXPORTDIALOG_H

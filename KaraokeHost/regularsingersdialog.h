#ifndef REGULARSINGERSDIALOG_H
#define REGULARSINGERSDIALOG_H

#include <QDialog>
#include <regularsingermodel.h>

namespace Ui {
class RegularSingersDialog;
}

class RegularSingersDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegularSingersDialog(QWidget *parent = 0);
    ~RegularSingersDialog();

private slots:
    void on_btnClose_clicked();

private:
    Ui::RegularSingersDialog *ui;
    RegularSingerModel *regularSingerModel;
};

#endif // REGULARSINGERSDIALOG_H

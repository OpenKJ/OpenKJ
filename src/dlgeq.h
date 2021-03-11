#ifndef DLGEQ_H
#define DLGEQ_H

#include <QDialog>

namespace Ui {
class DlgEq;
}

class DlgEq : public QDialog
{
    Q_OBJECT

public:
    explicit DlgEq(QWidget *parent = 0);
    ~DlgEq();

private:
    Ui::DlgEq *ui;

};

#endif // DLGEQ_H

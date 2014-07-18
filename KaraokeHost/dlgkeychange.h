#ifndef DLGKEYCHANGE_H
#define DLGKEYCHANGE_H

#include <QDialog>
#include "queuemodel.h"

namespace Ui {
class DlgKeyChange;
}

class DlgKeyChange : public QDialog
{
    Q_OBJECT

public:
    explicit DlgKeyChange(QueueModel *queueModel, QWidget *parent = 0);
    void setActiveSong(int songId);
    ~DlgKeyChange();

private slots:
    void on_buttonBox_accepted();

    void on_spinBoxKey_valueChanged(int arg1);

private:
    Ui::DlgKeyChange *ui;
    QueueModel *qModel;
    int m_activeSong;
};

#endif // DLGKEYCHANGE_H

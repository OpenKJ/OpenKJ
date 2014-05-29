#ifndef DLGKEYCHANGE_H
#define DLGKEYCHANGE_H

#include <QDialog>
#include "rotationtablemodel.h"

namespace Ui {
class DlgKeyChange;
}

class DlgKeyChange : public QDialog
{
    Q_OBJECT

public:
    explicit DlgKeyChange(RotationTableModel *rotationModel, QWidget *parent = 0);
    void setActiveSong(KhQueueSong *song);
    ~DlgKeyChange();

private slots:
    void on_buttonBox_accepted();

    void on_spinBoxKey_valueChanged(int arg1);

private:
    Ui::DlgKeyChange *ui;
    RotationTableModel *m_rotationModel;
    KhQueueSong *m_activeSong;
};

#endif // DLGKEYCHANGE_H

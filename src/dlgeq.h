#ifndef DLGEQ_H
#define DLGEQ_H

#include <QDialog>
#include "settings.h"

namespace Ui {
    class DlgEq;
}

class DlgEq : public QDialog {
Q_OBJECT

public:
    explicit DlgEq(QWidget *parent = 0);
    ~DlgEq();

private:
    Ui::DlgEq *ui;
    Settings m_settings;

signals:
    void karEqBypassChanged(bool enabled);
    void bmEqBypassChanged(bool enabled);
    void karEqLevelChanged(int band, int newValue);
    void bmEqLevelChanged(int band, int newValue);

};

#endif // DLGEQ_H

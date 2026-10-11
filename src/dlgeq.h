#ifndef DLGEQ_H
#define DLGEQ_H

#include <QWidget>
#include <QList>

class QLabel;
#include "settings.h"

namespace Ui {
    class DlgEq;
}

class DlgEq : public QWidget {
Q_OBJECT

public:
    explicit DlgEq(QWidget *parent = 0);
    ~DlgEq();
    void setFrequencyLabelsVisible(bool visible);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::DlgEq *ui;
    Settings m_settings;
    QList<QLabel *> m_frequencyLabels;

signals:
    void karEqBypassChanged(bool enabled);
    void bmEqBypassChanged(bool enabled);
    void karEqLevelChanged(int band, int newValue);
    void bmEqLevelChanged(int band, int newValue);

};

#endif // DLGEQ_H

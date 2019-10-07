#ifndef DLGDEBUGOUTPUT_H
#define DLGDEBUGOUTPUT_H

#include <QDialog>
#include <QTimer>

extern QStringList *logContents;

namespace Ui {
class DlgDebugOutput;
}

class DlgDebugOutput : public QDialog
{
    Q_OBJECT

public:
    explicit DlgDebugOutput(QWidget *parent = nullptr);
    ~DlgDebugOutput();

private:
    Ui::DlgDebugOutput *ui;
    QTimer *timer;

private slots:
    void timerTimeout();

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event);
};

#endif // DLGDEBUGOUTPUT_H

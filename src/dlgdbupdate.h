#ifndef DLGDBUPDATE_H
#define DLGDBUPDATE_H

#include <QDialog>
#include <QBasicTimer>

namespace Ui {
class DlgDbUpdate;
}

class DlgDbUpdate : public QDialog
{
    Q_OBJECT

public:
    explicit DlgDbUpdate(QWidget *parent = 0);
    ~DlgDbUpdate();

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    Ui::DlgDbUpdate *ui;
    QBasicTimer m_logFlushTimer;
    QString m_log;

public slots:
    void addLogMsg(const QString& msg);
    void changeStatusTxt(QString txt);
    void changeProgress(int progress, int max);
    void reset();
};

#endif // DLGDBUPDATE_H

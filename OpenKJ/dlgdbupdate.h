#ifndef DLGDBUPDATE_H
#define DLGDBUPDATE_H

#include <QDialog>

namespace Ui {
class DlgDbUpdate;
}

class DlgDbUpdate : public QDialog
{
    Q_OBJECT

public:
    explicit DlgDbUpdate(QWidget *parent = 0);
    ~DlgDbUpdate();

private:
    Ui::DlgDbUpdate *ui;

public slots:
    void addProgressMsg(QString msg);
    void changeStatusTxt(QString txt);
    void changeProgress(int progress);
    void setProgressMax(int max);
    void changeDirectory(QString dir);
    void reset();
};

#endif // DLGDBUPDATE_H

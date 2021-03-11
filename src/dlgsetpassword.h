#ifndef DLGSETPASSWORD_H
#define DLGSETPASSWORD_H

#include <QDialog>

namespace Ui {
class DlgSetPassword;
}

class DlgSetPassword : public QDialog
{
    Q_OBJECT

public:
    explicit DlgSetPassword(QWidget *parent = 0);
    QString getPassword() { return password; }
    ~DlgSetPassword();

private slots:
    void on_pushButtonClose_clicked();
    void on_pushButtonOk_clicked();

private:
    Ui::DlgSetPassword *ui;
    QString password;

};

#endif // DLGSETPASSWORD_H

#ifndef DLGPASSWORD_H
#define DLGPASSWORD_H

#include <QDialog>

namespace Ui {
class DlgPassword;
}

class DlgPassword : public QDialog
{
    Q_OBJECT

public:
    explicit DlgPassword(QWidget *parent = 0);
    ~DlgPassword();
    QString getPassword() { return password; }

private slots:
    void on_pushButtonOk_clicked();

    void on_pushButtonCancel_clicked();

    void on_pushButtonReset_clicked();

private:
    Ui::DlgPassword *ui;
    QString password;
};

#endif // DLGPASSWORD_H

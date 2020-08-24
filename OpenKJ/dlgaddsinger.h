#ifndef DLGADDSINGER_H
#define DLGADDSINGER_H

#include <QDialog>
#include <rotationmodel.h>

namespace Ui {
class DlgAddSinger;
}

class DlgAddSinger : public QDialog
{
    Q_OBJECT

public:
    explicit DlgAddSinger(RotationModel *rotModel, QWidget *parent = 0);
    ~DlgAddSinger();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::DlgAddSinger *ui;
    RotationModel *rotModel;

    // QWidget interface
protected:
    void showEvent(QShowEvent *event) override;
};

#endif // DLGADDSINGER_H

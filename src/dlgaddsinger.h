#ifndef DLGADDSINGER_H
#define DLGADDSINGER_H

#include <QDialog>
#include <src/models/tablemodelrotation.h>

namespace Ui {
class DlgAddSinger;
}

class DlgAddSinger : public QDialog
{
    Q_OBJECT

public:
    explicit DlgAddSinger(TableModelRotation *rotModel, QWidget *parent = 0);
    ~DlgAddSinger();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::DlgAddSinger *ui;
    TableModelRotation *rotModel;

    // QWidget interface
protected:
    void showEvent(QShowEvent *event) override;

signals:
    void newSingerAdded(const int position);
};

#endif // DLGADDSINGER_H

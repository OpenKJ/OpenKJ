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
    explicit DlgAddSinger(TableModelRotation &rotationModel, QWidget *parent = nullptr);
    ~DlgAddSinger() override;

private slots:
    void addSinger();

private:
    std::unique_ptr<Ui::DlgAddSinger> ui;
    TableModelRotation &m_rotModel;

protected:
    void showEvent(QShowEvent *event) override;

signals:
    void newSingerAdded(int position);
};

#endif // DLGADDSINGER_H

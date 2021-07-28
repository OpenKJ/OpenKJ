#ifndef DLGADDSINGER_H
#define DLGADDSINGER_H

#include <memory>
#include <QDialog>
#include <src/models/tablemodelrotation.h>

namespace Ui {
    class DlgAddSinger;
}

class DlgAddSinger : public QDialog {
Q_OBJECT

public:
    explicit DlgAddSinger(TableModelRotation &rotationModel, QWidget *parent = nullptr);
    ~DlgAddSinger() override;

private:
    std::unique_ptr<Ui::DlgAddSinger> ui;
    TableModelRotation &m_rotModel;

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void addSinger();

signals:
    void newSingerAdded(int position);
};

#endif // DLGADDSINGER_H

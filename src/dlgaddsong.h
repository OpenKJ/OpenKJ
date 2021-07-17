#ifndef DLGADDSONG_H
#define DLGADDSONG_H

#include <memory>
#include <QButtonGroup>
#include <QDialog>
#include "src/models/tablemodelrotation.h"
#include "src/models/tablemodelqueuesongs.h"

namespace Ui {
    class DlgAddSong;
}

class DlgAddSong : public QDialog {
Q_OBJECT

public:
    explicit DlgAddSong(TableModelRotation &rotationModel, TableModelQueueSongs &queueModel, int songId,
                        QWidget *parent = nullptr);
    ~DlgAddSong() override;

private:
    std::unique_ptr<Ui::DlgAddSong> ui;
    QButtonGroup m_rButtons;
    TableModelRotation &m_rotModel;
    TableModelQueueSongs &m_queueModel;
    int m_songId;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void pushButtonCancelClicked();
    void spinBoxKeyChangeChanged(int arg1);
    void pushButtonAddClicked();
    void comboBoxRotSingersActivated(int index);
    void lineEditNewSingerNameTextChanged(const QString &arg1);
    void comboBoxRegSingersActivated(int index);

signals:
    void newSingerAdded(int position);
};

#endif // DLGADDSONG_H

#ifndef DLGADDSONG_H
#define DLGADDSONG_H

#include <QButtonGroup>
#include <QDialog>
#include "tablemodelrotation.h"
#include "tablemodelqueuesongs.h"

namespace Ui {
class DlgAddSong;
}

class DlgAddSong : public QDialog
{
    Q_OBJECT

public:
    explicit DlgAddSong(TableModelRotation *rotationModel, TableModelQueueSongs *queueModel, int songId, QWidget *parent = nullptr);
    ~DlgAddSong();

private:
    Ui::DlgAddSong *ui;
    QButtonGroup m_rButtons;
    TableModelRotation *m_rotModel;
    TableModelQueueSongs *m_queueModel;
    int m_songId;
    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
private slots:
    void on_pushButtonCancel_clicked();
    void on_pushButtonKeyDown_clicked();
    void on_pushButtonKeyUp_clicked();
    void on_spinBoxKeyChange_valueChanged(int arg1);
    void on_pushButtonAdd_clicked();

    void on_comboBoxRotSingers_activated(int index);

    void on_lineEditNewSingerName_returnPressed();

    void on_lineEditNewSingerName_textChanged(const QString &arg1);

    void on_comboBoxRegSingers_activated(int index);

signals:
    void newSingerAdded(const int position);
};

#endif // DLGADDSONG_H

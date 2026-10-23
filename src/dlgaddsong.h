#ifndef DLGADDSONG_H
#define DLGADDSONG_H

#include <memory>
#include <QButtonGroup>
#include <QDialog>
#include "src/models/tablemodelrotation.h"
#include "src/models/tablemodelqueuesongs.h"
#include "settings.h"

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
    Settings m_settings;
    bool m_singerListReady{false};
    bool m_singerLayoutPending{false};
    int m_singerColumnWidth{0};
    void updateSingerListLayout();
    void updateControlLayout();

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void pushButtonCancelClicked();
    void spinBoxKeyChangeChanged(int arg1);
    void pushButtonAddClicked();
    void rotationSingerSelected();
    void lineEditNewSingerNameTextChanged(const QString &arg1);
    void comboBoxRegSingersActivated(int index);

signals:
    void newSingerAdded(int position);
};

#endif // DLGADDSONG_H

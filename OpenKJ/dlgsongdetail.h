#ifndef DLGSONGDETAIL_H
#define DLGSONGDETAIL_H

#include <QDialog>

#include "dbtablemodel.h"

namespace Ui {
class DlgSongDetail;
}

class DlgSongDetail : public QDialog
{
    Q_OBJECT

public:
    explicit DlgSongDetail(QWidget *parent = 0);
    ~DlgSongDetail();

    void accept() Q_DECL_OVERRIDE;
    void editSong(int song_id, DbTableModel *dbModel);

private:
    Ui::DlgSongDetail *ui;
    int m_song_id;
    DbTableModel *m_dbModel;
};

#endif // DLGSONGDETAIL_H

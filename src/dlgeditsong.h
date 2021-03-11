#ifndef DLGEDITSONG_H
#define DLGEDITSONG_H

#include <QDialog>

namespace Ui {
class DlgEditSong;
}

class DlgEditSong : public QDialog
{
    Q_OBJECT

public:
    explicit DlgEditSong(QString artist, QString title, QString songId, bool showSongId = true, bool allowRename = true, QWidget *parent = 0);
    ~DlgEditSong();
    QString artist();
    QString title();
    QString songId();
    bool renameFile();

private slots:
    void on_btnSave_clicked();

    void on_btnCancel_clicked();

private:
    Ui::DlgEditSong *ui;
};

#endif // DLGEDITSONG_H

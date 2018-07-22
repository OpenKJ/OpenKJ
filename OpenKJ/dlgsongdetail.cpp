#include "dlgsongdetail.h"
#include "ui_dlgsongdetail.h"

DlgSongDetail::DlgSongDetail(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgSongDetail)
{
    ui->setupUi(this);

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

DlgSongDetail::~DlgSongDetail()
{
    delete ui;
}

void DlgSongDetail::accept()
{
    QString artist, title, discID;
    artist = ui->lineEditArtist->text();
    discID = ui->lineEditDiscID->text();
    title = ui->lineEditSongTitle->text();

    m_dbModel->updateSong(m_song_id,artist, title, discID);
    QDialog::accept();

}

void DlgSongDetail::editSong(int song_id, DbTableModel *dbModel)
{
    QString artist, title, discID, filename;
    if (dbModel->fetchSongDetail(song_id, artist, title, discID, filename))
    {
        m_song_id = song_id;
        m_dbModel = dbModel;
        ui->lineEditArtist->setText(artist);
        ui->lineEditDiscID->setText(discID);
        ui->lineEditSongTitle->setText(title);
        ui->lineEditFilename->setText(filename);

        setModal(true);
        show();
    }
}

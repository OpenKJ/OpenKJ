#include "dlgeditsong.h"
#include "ui_dlgeditsong.h"
#include <QRegExpValidator>
#include <QRegExp>

DlgEditSong::DlgEditSong(QString artist, QString title, QString songId, bool showSongId, bool allowRename, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgEditSong)
{
    ui->setupUi(this);
    ui->lineEditArtist->setText(artist);
    ui->lineEditTitle->setText(title);
    ui->lineEditSongId->setText(songId);
    QRegExp exp("[^\\*\\:\\/\\\\]+");
    exp.setCaseSensitivity(Qt::CaseInsensitive);
    QRegExpValidator *v = new QRegExpValidator(exp, this);
    ui->lineEditArtist->setValidator(v);
    ui->lineEditTitle->setValidator(v);
    ui->lineEditSongId->setValidator(v);
    if (!showSongId)
    {
        ui->lineEditSongId->setEnabled(false);
        ui->lineEditSongId->setToolTip("SongId not supported using the current naming pattern");
    }
    if (!allowRename)
    {
        ui->cbxRenameFile->setChecked(false);
        ui->cbxRenameFile->setEnabled(false);
        ui->cbxRenameFile->setToolTip("Can't rename files in source directories using CUSTOM or METADATA patterns.");
    }
}

DlgEditSong::~DlgEditSong()
{
    delete ui;
}

QString DlgEditSong::artist()
{
    return ui->lineEditArtist->text();
}

QString DlgEditSong::title()
{
    return ui->lineEditTitle->text();
}

QString DlgEditSong::songId()
{
    return ui->lineEditSongId->text();
}

bool DlgEditSong::renameFile()
{
    return ui->cbxRenameFile->isChecked();
}

void DlgEditSong::on_btnSave_clicked()
{
    accept();
}

void DlgEditSong::on_btnCancel_clicked()
{
    reject();
}

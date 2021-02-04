#include "dlgcustompatterns.h"
#include "ui_dlgcustompatterns.h"
#include "settings.h"
#include <QRegularExpression>
#include <QInputDialog>
#include <QSqlQuery>
#include "karaokefileinfo.h"

extern Settings settings;

void DlgCustomPatterns::evaluateRegEx()
{
    KaraokeFileInfo parser;
    ui->labelArtistExample->setText(parser.testPattern(ui->lineEditArtistRegEx->text(), ui->lineEditFilenameExample->text(), ui->spinBoxArtistCaptureGrp->value()));
    ui->labelTitleExample->setText(parser.testPattern(ui->lineEditTitleRegEx->text(), ui->lineEditFilenameExample->text(), ui->spinBoxTitleCaptureGrp->value()));
    ui->labelDiscIdExample->setText(parser.testPattern(ui->lineEditDiscIdRegEx->text(), ui->lineEditFilenameExample->text(), ui->spinBoxDiscIdCaptureGrp->value()));
}

DlgCustomPatterns::DlgCustomPatterns(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgCustomPatterns)
{
    ui->setupUi(this);
    patternsModel = new CustomPatternsModel(this);
    ui->tableViewPatterns->setModel(patternsModel);
    settings.restoreColumnWidths(ui->tableViewPatterns);
    settings.restoreWindowState(this);
    selectedRow = -1;
}

DlgCustomPatterns::~DlgCustomPatterns()
{
    delete ui;
}

void DlgCustomPatterns::on_btnClose_clicked()
{
    settings.saveColumnWidths(ui->tableViewPatterns);
    settings.saveWindowState(this);
    hide();
}

void DlgCustomPatterns::on_tableViewPatterns_clicked(const QModelIndex &index)
{
    selectedRow = index.row();
    Pattern pattern = patternsModel->getPattern(selectedRow);
    selectedPattern = pattern;
    ui->lineEditDiscIdRegEx->setText(pattern.getSongIdRegex());
    ui->spinBoxDiscIdCaptureGrp->setValue(pattern.getSongIdCaptureGrp());
    ui->lineEditArtistRegEx->setText(pattern.getArtistRegex());
    ui->spinBoxArtistCaptureGrp->setValue(pattern.getArtistCaptureGrp());
    ui->lineEditTitleRegEx->setText(pattern.getTitleRegex());
    ui->spinBoxTitleCaptureGrp->setValue(pattern.getTitleCaptureGrp());
}

void DlgCustomPatterns::on_lineEditDiscIdRegEx_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    evaluateRegEx();
}

void DlgCustomPatterns::on_lineEditArtistRegEx_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    evaluateRegEx();
}

void DlgCustomPatterns::on_lineEditTitleRegEx_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    evaluateRegEx();
}

void DlgCustomPatterns::on_lineEditFilenameExample_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    evaluateRegEx();
}

void DlgCustomPatterns::on_spinBoxDiscIdCaptureGrp_valueChanged(int arg1)
{
    Q_UNUSED(arg1);
    evaluateRegEx();
}

void DlgCustomPatterns::on_spinBoxArtistCaptureGrp_valueChanged(int arg1)
{
    Q_UNUSED(arg1);
    evaluateRegEx();
}

void DlgCustomPatterns::on_spinBoxTitleCaptureGrp_valueChanged(int arg1)
{
    Q_UNUSED(arg1);
    evaluateRegEx();
}

void DlgCustomPatterns::on_btnAdd_clicked()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("New Custom Pattern"), tr("Pattern name:"), QLineEdit::Normal, tr("New Pattern"), &ok);
    if (ok && !name.isEmpty())
    {
        QSqlQuery query;
        query.exec("INSERT INTO custompatterns (name) VALUES(\"" + name + "\")");
        patternsModel->loadFromDB();
        //bmAddPlaylist(title);
    }
}

void DlgCustomPatterns::on_btnDelete_clicked()
{
    if (ui->tableViewPatterns->selectionModel()->selectedIndexes().count() > 0)
    {
        QSqlQuery query;
        query.exec("DELETE FROM custompatterns WHERE name == \"" + selectedPattern.getName() + "\"");
        patternsModel->loadFromDB();
    }
}

void DlgCustomPatterns::on_btnApplyChanges_clicked()
{
    if (ui->tableViewPatterns->selectionModel()->selectedIndexes().count() > 0)
    {
        QSqlQuery query;
        QString arx, trx, drx, acg, tcg, dcg, name;
        arx = ui->lineEditArtistRegEx->text();
        trx = ui->lineEditTitleRegEx->text();
        drx = ui->lineEditDiscIdRegEx->text();
        acg = QString::number(ui->spinBoxArtistCaptureGrp->value());
        tcg = QString::number(ui->spinBoxTitleCaptureGrp->value());
        dcg = QString::number(ui->spinBoxDiscIdCaptureGrp->value());
        name = selectedPattern.getName();
        query.exec("UPDATE custompatterns SET artistregex = \"" + arx + "\", titleregex = \"" + trx + "\", discidregex = \"" + drx + \
                   "\", artistcapturegrp = " + acg + ", titlecapturegrp = " + tcg + ", discidcapturegrp = " + dcg + " WHERE name = \"" + name + "\"");
        patternsModel->loadFromDB();
    }
}

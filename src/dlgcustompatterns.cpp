#include "dlgcustompatterns.h"
#include "ui_dlgcustompatterns.h"
#include "settings.h"
#include <QInputDialog>
#include <QSqlQuery>
#include "karaokefileinfo.h"

extern Settings settings;

void DlgCustomPatterns::evaluateRegEx() {
    KaraokeFileInfo parser;
    ui->labelArtistExample->setText(
            parser.testPattern(ui->lineEditArtistRegEx->text(), ui->lineEditFilenameExample->text(),
                               ui->spinBoxArtistCaptureGrp->value()));
    ui->labelTitleExample->setText(
            parser.testPattern(ui->lineEditTitleRegEx->text(), ui->lineEditFilenameExample->text(),
                               ui->spinBoxTitleCaptureGrp->value()));
    ui->labelDiscIdExample->setText(
            parser.testPattern(ui->lineEditDiscIdRegEx->text(), ui->lineEditFilenameExample->text(),
                               ui->spinBoxDiscIdCaptureGrp->value()));
}

DlgCustomPatterns::DlgCustomPatterns(QWidget *parent) :
        QDialog(parent),
        ui(new Ui::DlgCustomPatterns) {
    ui->setupUi(this);
    ui->tableViewPatterns->setModel(&m_patternsModel);
    settings.restoreColumnWidths(ui->tableViewPatterns);
    settings.restoreWindowState(this);
    connect(ui->lineEditArtistRegEx, &QLineEdit::textChanged, this, &DlgCustomPatterns::evaluateRegEx);
    connect(ui->lineEditTitleRegEx, &QLineEdit::textChanged, this, &DlgCustomPatterns::evaluateRegEx);
    connect(ui->lineEditDiscIdRegEx, &QLineEdit::textChanged, this, &DlgCustomPatterns::evaluateRegEx);
    connect(ui->lineEditFilenameExample, &QLineEdit::textChanged, this, &DlgCustomPatterns::evaluateRegEx);
    connect(ui->spinBoxArtistCaptureGrp, qOverload<int>(&QSpinBox::valueChanged), this,
            &DlgCustomPatterns::evaluateRegEx);
    connect(ui->spinBoxTitleCaptureGrp, qOverload<int>(&QSpinBox::valueChanged), this,
            &DlgCustomPatterns::evaluateRegEx);
    connect(ui->spinBoxDiscIdCaptureGrp, qOverload<int>(&QSpinBox::valueChanged), this,
            &DlgCustomPatterns::evaluateRegEx);
    connect(ui->tableViewPatterns, &QTableView::clicked, this, &DlgCustomPatterns::tableViewPatternsClicked);
    connect(ui->btnClose, &QPushButton::clicked, this, &DlgCustomPatterns::btnCloseClicked);
    connect(ui->btnAdd, &QPushButton::clicked, this, &DlgCustomPatterns::btnAddClicked);
    connect(ui->btnDelete, &QPushButton::clicked, this, &DlgCustomPatterns::btnDeleteClicked);
    connect(ui->btnApplyChanges, &QPushButton::clicked, this, &DlgCustomPatterns::btnApplyChangesClicked);
}

DlgCustomPatterns::~DlgCustomPatterns() = default;

void DlgCustomPatterns::btnCloseClicked() {
    settings.saveColumnWidths(ui->tableViewPatterns);
    settings.saveWindowState(this);
    hide();
}

void DlgCustomPatterns::tableViewPatternsClicked(const QModelIndex &index) {
    Pattern pattern = m_patternsModel.getPattern(index.row());
    m_selectedPattern = pattern;
    ui->lineEditDiscIdRegEx->setText(pattern.getSongIdRegex());
    ui->spinBoxDiscIdCaptureGrp->setValue(pattern.getSongIdCaptureGrp());
    ui->lineEditArtistRegEx->setText(pattern.getArtistRegex());
    ui->spinBoxArtistCaptureGrp->setValue(pattern.getArtistCaptureGrp());
    ui->lineEditTitleRegEx->setText(pattern.getTitleRegex());
    ui->spinBoxTitleCaptureGrp->setValue(pattern.getTitleCaptureGrp());
}

void DlgCustomPatterns::btnAddClicked() {
    bool ok;
    QString name = QInputDialog::getText(this, tr("New Custom Pattern"), tr("Pattern name:"), QLineEdit::Normal,
                                         tr("New Pattern"), &ok);
    if (ok && !name.isEmpty()) {
        QSqlQuery query;
        query.exec("INSERT INTO custompatterns (name) VALUES(\"" + name + "\")");
        m_patternsModel.loadFromDB();
    }
}

void DlgCustomPatterns::btnDeleteClicked() {
    if (ui->tableViewPatterns->selectionModel()->selectedIndexes().count() > 0) {
        QSqlQuery query;
        query.exec("DELETE FROM custompatterns WHERE name == \"" + m_selectedPattern.getName() + "\"");
        m_patternsModel.loadFromDB();
    }
}

void DlgCustomPatterns::btnApplyChangesClicked() {
    if (ui->tableViewPatterns->selectionModel()->selectedIndexes().count() > 0) {
        QSqlQuery query;
        QString arx, trx, drx, acg, tcg, dcg, name;
        arx = ui->lineEditArtistRegEx->text();
        trx = ui->lineEditTitleRegEx->text();
        drx = ui->lineEditDiscIdRegEx->text();
        acg = QString::number(ui->spinBoxArtistCaptureGrp->value());
        tcg = QString::number(ui->spinBoxTitleCaptureGrp->value());
        dcg = QString::number(ui->spinBoxDiscIdCaptureGrp->value());
        name = m_selectedPattern.getName();
        query.exec("UPDATE custompatterns SET artistregex = \"" + arx + "\", titleregex = \"" + trx +
                   "\", discidregex = \"" + drx + \
                   "\", artistcapturegrp = " + acg + ", titlecapturegrp = " + tcg + ", discidcapturegrp = " + dcg +
                   " WHERE name = \"" + name + "\"");
        m_patternsModel.loadFromDB();
    }
}

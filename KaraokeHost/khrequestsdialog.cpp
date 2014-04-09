#include "khrequestsdialog.h"
#include "ui_khrequestsdialog.h"

KhRequestsDialog::KhRequestsDialog(KhSongs *fullData, KhRotationSingers *singers, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KhRequestsDialog)
{
    ui->setupUi(this);
    requestsModel = new RequestsTableModel(this);
    ui->treeViewRequests->setModel(requestsModel);
    ui->treeViewRequests->hideColumn(0);
//    ui->treeViewRequests->header()->setSectionResizeMode(1,QHeaderView::ResizeToContents);
//    ui->treeViewRequests->header()->setSectionResizeMode(2,QHeaderView::Stretch);
//    ui->treeViewRequests->header()->setSectionResizeMode(3,QHeaderView::Stretch);
//    ui->treeViewRequests->header()->setSectionResizeMode(4,QHeaderView::ResizeToContents);
//    ui->treeViewRequests->header()->setSectionResizeMode(1,QHeaderView::Interactive);
//    ui->treeViewRequests->header()->setSectionResizeMode(2,QHeaderView::Interactive);
//    ui->treeViewRequests->header()->setSectionResizeMode(3,QHeaderView::Interactive);
    connect(requestsModel, SIGNAL(layoutChanged()), this, SLOT(requestsModified()));
    songDbModel = new SongDBTableModel(this);
    songDbModel->setFullData(fullData);
    ui->treeViewSearch->setModel(songDbModel);
    connect(ui->treeViewRequests->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(requestSelectionChanged(QModelIndex,QModelIndex)));
    connect(ui->treeViewSearch->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(songSelectionChanged(QModelIndex,QModelIndex)));
    rotSingers = singers;
    ui->comboBoxAddPosition->setEnabled(false);
    ui->comboBoxSingers->setEnabled(true);
    ui->lineEditSingerName->setEnabled(false);
    ui->labelAddPos->setEnabled(false);
}

KhRequestsDialog::~KhRequestsDialog()
{
    delete ui;
}

void KhRequestsDialog::on_pushButtonClose_clicked()
{
    close();
}

void KhRequestsDialog::requestsModified()
{
    this->show();
    ui->treeViewRequests->header()->resizeSections(QHeaderView::Stretch);
}

void KhRequestsDialog::on_pushButtonSearch_clicked()
{
    songDbModel->applyFilter(ui->lineEditSearch->text());
}

void KhRequestsDialog::on_lineEditSearch_returnPressed()
{
    songDbModel->applyFilter(ui->lineEditSearch->text());

}

void KhRequestsDialog::requestSelectionChanged(const QModelIndex &current, const QModelIndex &previous)
{
    if (current.isValid())
    {
        ui->comboBoxSingers->clear();
        ui->comboBoxSingers->addItems(rotSingers->getSingerList());
        QString filterStr = current.sibling(current.row(),2).data().toString() + " " + current.sibling(current.row(),3).data().toString();
        songDbModel->applyFilter(filterStr);
        ui->lineEditSearch->setText(filterStr);
    }

}

void KhRequestsDialog::songSelectionChanged(const QModelIndex &current, const QModelIndex &previous)
{
    if (current.isValid())
    {
//        if (ui->treeViewSearch->selectionModel()->selectedIndexes().size() > 0)
//            ui->groupBoxAddSong->setEnabled(true);
//        else
//            ui->groupBoxAddSong->setEnabled(false);
    }
}

void KhRequestsDialog::on_radioButtonExistingSinger_toggled(bool checked)
{
    ui->comboBoxAddPosition->setEnabled(!checked);
    ui->comboBoxSingers->setEnabled(checked);
    ui->lineEditSingerName->setEnabled(!checked);
    ui->labelAddPos->setEnabled(!checked);
}

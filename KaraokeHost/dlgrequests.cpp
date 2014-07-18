#include "dlgrequests.h"
#include "ui_dlgrequests.h"
#include <QMenu>
#include <QMessageBox>
#include "khsettings.h"

extern KhSettings *settings;

DlgRequests::DlgRequests(RotationModel *rotationModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgRequests)
{
    ui->setupUi(this);
    requestsModel = new RequestsTableModel(this);
    dbModel = new DbTableModel(this);
    dbDelegate = new DbItemDelegate(this);
    ui->treeViewRequests->setModel(requestsModel);
    ui->treeViewRequests->header()->setSectionResizeMode(5,QHeaderView::Fixed);
    ui->treeViewRequests->header()->resizeSection(5,22);
    ui->treeViewRequests->hideColumn(0);
    connect(requestsModel, SIGNAL(layoutChanged()), this, SLOT(requestsModified()));
    ui->tableViewSearch->setModel(dbModel);
    ui->tableViewSearch->setItemDelegate(dbDelegate);
    cdgPreviewDialog = new DlgCdgPreview(this);
    connect(ui->treeViewRequests->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(requestSelectionChanged(QModelIndex,QModelIndex)));
    connect(ui->tableViewSearch->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(songSelectionChanged(QModelIndex,QModelIndex)));
    connect(requestsModel, SIGNAL(updateReceived(QTime)), this, SLOT(updateReceived(QTime)));
    connect(requestsModel, SIGNAL(authenticationError()), this, SLOT(authError()));
    connect(requestsModel, SIGNAL(sslError()), this, SLOT(sslError()));
    connect(requestsModel, SIGNAL(delayError(int)), this, SLOT(delayError(int)));
    rotModel = rotationModel;
    ui->comboBoxAddPosition->setEnabled(false);
    ui->comboBoxSingers->setEnabled(true);
    ui->lineEditSingerName->setEnabled(false);
    ui->labelAddPos->setEnabled(false);
    QStringList posOptions;
    posOptions << "After current singer";
    posOptions << "Fair (full rotation)";
    posOptions << "Bottom of rotation";
    ui->comboBoxAddPosition->addItems(posOptions);
    ui->comboBoxAddPosition->setCurrentIndex(1);
    settings->restoreColumnWidths(ui->treeViewRequests);
    settings->restoreColumnWidths(ui->tableViewSearch);
    ui->tableViewSearch->hideColumn(0);
    ui->tableViewSearch->hideColumn(5);
    ui->tableViewSearch->hideColumn(6);
}

DlgRequests::~DlgRequests()
{

    delete ui;
}

void DlgRequests::on_pushButtonClose_clicked()
{
    settings->saveColumnWidths(ui->treeViewRequests);
    settings->saveColumnWidths(ui->tableViewSearch);
    close();
}

void DlgRequests::requestsModified()
{
    if (requestsModel->count() > 0)
    {
        this->show();
    }
}

void DlgRequests::on_pushButtonSearch_clicked()
{
    dbModel->search(ui->lineEditSearch->text());
}

void DlgRequests::on_lineEditSearch_returnPressed()
{
    dbModel->search(ui->lineEditSearch->text());
}

void DlgRequests::requestSelectionChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);
    qDebug() << "Current selection " << current.row();
    if ((current.isValid()) && (ui->treeViewRequests->selectionModel()->selectedIndexes().size() > 0))
    {
//        ui->comboBoxSingers->clear();
//        ui->comboBoxSingers->addItems(rotSingers->getSingerList());
//        QString filterStr = current.sibling(current.row(),2).data().toString() + " " + current.sibling(current.row(),3).data().toString();
//        songDbModel->applyFilter(filterStr);
//        ui->lineEditSearch->setText(filterStr);
//        ui->treeViewSearch->header()->resizeSections(QHeaderView::Stretch);
    }

}

void DlgRequests::songSelectionChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);
    if (current.isValid())
    {
//        if (ui->treeViewSearch->selectionModel()->selectedIndexes().size() > 0)
//            ui->groupBoxAddSong->setEnabled(true);
//        else
//            ui->groupBoxAddSong->setEnabled(false);
    }
}

void DlgRequests::on_radioButtonExistingSinger_toggled(bool checked)
{
    ui->comboBoxAddPosition->setEnabled(!checked);
    ui->comboBoxSingers->setEnabled(checked);
    ui->lineEditSingerName->setEnabled(!checked);
    ui->labelAddPos->setEnabled(!checked);
}

void DlgRequests::on_pushButtonClearReqs_clicked()
{
    QMessageBox msgBox;
    msgBox.setText("Are you sure?");
    msgBox.setInformativeText("This action will clear all received requests. This operation can not be undone.");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.addButton(QMessageBox::Cancel);
    QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
    msgBox.exec();
    if (msgBox.clickedButton() == yesButton)
    {
        requestsModel->deleteAll();
    }
}

void DlgRequests::on_treeViewRequests_clicked(const QModelIndex &index)
{
    if (index.column() == 5)
    {
        requestsModel->deleteRequestId(index.sibling(index.row(),0).data().toInt());
    }
    else
    {
        ui->comboBoxSingers->clear();
        QString singerName = index.sibling(index.row(),1).data().toString();
        QStringList singers = rotModel->singers();
        ui->comboBoxSingers->addItems(singers);

        QString filterStr = index.sibling(index.row(),2).data().toString() + " " + index.sibling(index.row(),3).data().toString();
        dbModel->search(filterStr);
        ui->lineEditSearch->setText(filterStr);
        ui->lineEditSingerName->setText(singerName);

        int s = -1;
        for (int i=0; i < singers.size(); i++)
        {
            if (singers.at(i).toLower() == singerName.toLower())
            {
                s = i;
                break;
            }
        }
        if (s != -1)
        {
            ui->comboBoxSingers->setCurrentIndex(s);
        }
    }
}

void DlgRequests::on_pushButtonAddSong_clicked()
{
    if (ui->treeViewRequests->selectionModel()->selectedIndexes().size() < 1)
        return;
    if (ui->tableViewSearch->selectionModel()->selectedIndexes().size() < 1)
        return;

    QModelIndex index = ui->tableViewSearch->selectionModel()->selectedIndexes().at(0);
    int songid = index.sibling(index.row(),0).data().toInt();
    if (ui->radioButtonNewSinger->isChecked())
    {
        if (ui->lineEditSingerName->text() == "")
            return;
        else if (rotModel->singerExists(ui->lineEditSingerName->text()))
            return;
        else
        {
            rotModel->singerAdd(ui->lineEditSingerName->text());
            if (rotModel->currentSinger() != -1)
            {
                int curSingerPos = rotModel->getSingerPosition(rotModel->currentSinger());
                if (ui->comboBoxAddPosition->currentText() == "After current singer")
                    rotModel->singerMove(rotModel->rowCount() -1, curSingerPos + 1);
                else if ((ui->comboBoxAddPosition->currentText() == "Fair (full rotation)") && (curSingerPos != 0))
                    rotModel->singerMove(rotModel->rowCount() -1, curSingerPos);
            }
            emit addRequestSong(songid, rotModel->getSingerId(ui->lineEditSingerName->text()));
        }
    }
    else if (ui->radioButtonExistingSinger->isChecked())
    {
        emit addRequestSong(songid, rotModel->getSingerId(ui->comboBoxSingers->currentText()));
    }
}

void DlgRequests::on_treeViewSearch_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->tableViewSearch->indexAt(pos);
    if (index.isValid())
    {
        QString zipPath = index.sibling(index.row(),5).data().toString();
        cdgPreviewDialog->setZipFile(zipPath);
        QMenu contextMenu(this);
        contextMenu.addAction("Preview", cdgPreviewDialog, SLOT(preview()));
        contextMenu.exec(QCursor::pos());
    }
}

void DlgRequests::updateReceived(QTime updateTime)
{
    ui->labelLastUpdate->setText(updateTime.toString("hh:mm:ss AP"));
}

void DlgRequests::on_buttonRefresh_clicked()
{
    requestsModel->forceFullUpdate();
}

void DlgRequests::authError()
{
    QMessageBox::warning(this, "Authentication Error", "An authentication error was encountered while trying to connect to the requests server.  Please verify that your username, password, and the requests server URL are correct.");
}

void DlgRequests::sslError()
{
    QMessageBox::warning(this, "SSL Handshake Error", "An error was encountered while establishing a secure connection to the requests server.  This is usually caused by an invalid or self-signed cert on the server.  You can set the requests client to ignore SSL errors in the network settings dialog.");
}

void DlgRequests::delayError(int seconds)
{
    QMessageBox::warning(this, "Possible Connectivity Issue", "It has been " + QString::number(seconds) + " seconds since we last received a response from the requests server.  You may be missing new submitted requests.  Please ensure that your network connection is up and working.");
}

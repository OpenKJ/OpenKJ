#include "dlgsongshop.h"
#include "ui_dlgsongshop.h"


DlgSongShop::DlgSongShop(SongShop *songShop, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgSongShop)
{
    ui->setupUi(this);
    shop = songShop;
    modelSongs = new SongShopModel(shop, this);
    sortFilterModel = new ShopSortFilterProxyModel(this);
    sortFilterModel->setSourceModel(modelSongs);
    sortFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    sortFilterModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    sortFilterModel->sort(0);
    sortFilterModel->setFilterKeyColumn(-1);
    ui->tableViewSongs->setModel(sortFilterModel);
    dlgPurchase = new DlgSongShopPurchase(shop, this);
    dlgPurchase->setModal(false);
    settings->restoreColumnWidths(ui->tableViewSongs);
    connect(modelSongs->getShop(), SIGNAL(karaokeSongDownloaded(QString)), this, SIGNAL(karaokeSongDownloaded(QString)));
    connect(modelSongs->getShop(), SIGNAL(songsUpdated()), this, SLOT(autoSizeView()));
}

DlgSongShop::~DlgSongShop()
{
    delete ui;
}

void DlgSongShop::on_btnClose_clicked()
{
    settings->saveColumnWidths(ui->tableViewSongs);
    close();
}

void DlgSongShop::on_lineEditSearch_textChanged(const QString &arg1)
{
    sortFilterModel->setSearchTerms(arg1);
}

void DlgSongShop::on_btnPurchase_clicked()
{
    if (ui->tableViewSongs->selectionModel()->selectedIndexes().size() < 1)
        return;
    QModelIndex idx = ui->tableViewSongs->selectionModel()->selectedIndexes().at(0);
    dlgPurchase->setSongId(idx.data(Qt::UserRole).toString());
    dlgPurchase->setArtist(idx.sibling(idx.row(), 0).data().toString());
    dlgPurchase->setTitle(idx.sibling(idx.row(), 1).data().toString());
    dlgPurchase->setPrice(idx.sibling(idx.row(), 4).data().toString());
    dlgPurchase->doAuth();
    dlgPurchase->show();
}

void DlgSongShop::setVisible(bool visible)
{
    settings->restoreColumnWidths(ui->tableViewSongs);
    QDialog::setVisible(visible);
}

void DlgSongShop::autoSizeView()
{
    int priceColSize = QFontMetrics(settings->applicationFont()).width("__$0.00__");
    int songidColSize = QFontMetrics(settings->applicationFont()).width("__PY000000__");
    int vendorColSize = QFontMetrics(settings->applicationFont()).width("__Party Tyme Karaoke__");
    int mediaColSize = QFontMetrics(settings->applicationFont()).width("__mp3+g__");
    int remainingSpace = ui->tableViewSongs->width() - priceColSize - songidColSize - vendorColSize - mediaColSize - 20;
    int artistColSize = (remainingSpace / 2) - 100;
    int titleColSize = (remainingSpace / 2) + 80;
    ui->tableViewSongs->horizontalHeader()->resizeSection(0, artistColSize);
    ui->tableViewSongs->horizontalHeader()->resizeSection(1, titleColSize);
    ui->tableViewSongs->horizontalHeader()->resizeSection(2, songidColSize);
    ui->tableViewSongs->horizontalHeader()->resizeSection(3, vendorColSize);
    ui->tableViewSongs->horizontalHeader()->resizeSection(4, mediaColSize);
    ui->tableViewSongs->horizontalHeader()->resizeSection(5, priceColSize);
}


void DlgSongShop::resizeEvent(QResizeEvent *event)
{
    autoSizeView();
}

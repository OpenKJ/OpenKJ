#include "dlgsongshoppurchase.h"
#include "ui_dlgsongshoppurchase.h"
#include "dlgsetpassword.h"
#include "dlgpassword.h"


DlgSongShopPurchase::DlgSongShopPurchase(SongShop *songShop, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgSongShopPurchase)
{
    setupDone = false;
    ui->setupUi(this);
    ui->lineEditCCN->setValidator(new QRegExpValidator(QRegExp("[0-9]*"), this));
    ui->lineEditCCM->setValidator(new QRegExpValidator(QRegExp("[0-9]*"), this));
    ui->lineEditCCY->setValidator(new QRegExpValidator(QRegExp("[0-9]*"), this));
    ui->lineEditCCV->setValidator(new QRegExpValidator(QRegExp("[0-9]*"), this));
    shop = songShop;
    knLoginTest = false;
    ui->cbxSaveAccount->setChecked(settings->saveKNAccount());
    ui->cbxSaveCard->setChecked(settings->saveCC());
    authenticated = false;
    setupDone = true;
    connect(shop, SIGNAL(paymentProcessingFailed()), this, SLOT(paymentProcessingFailed()));
    connect(shop, SIGNAL(karaokeSongDownloaded(QString)), this, SLOT(purchaseSuccess()));
    msgBoxInfo = new DlgPurchaseProgress;
    connect(shop, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));
}

DlgSongShopPurchase::~DlgSongShopPurchase()
{
    delete ui;
}

void DlgSongShopPurchase::setArtist(QString artist)
{
    ui->labelArtist->setText(artist);
    this->artist = artist;
}

void DlgSongShopPurchase::setTitle(QString title)
{
    ui->labelTitle->setText(title);
    this->title = title;
}

void DlgSongShopPurchase::setSongId(QString songId)
{
    ui->labelSongId->setText(songId);
    this->songId = songId;
}

void DlgSongShopPurchase::setPrice(QString price)
{
    ui->labelPrice->setText(price + " (additional taxes may apply)");
}

void DlgSongShopPurchase::doAuth()
{
    if (!setupDone)
        return;
    qWarning() << "running doAuth";
    if ((settings->saveCC() || settings->saveKNAccount()) && settings->passIsSet())
    {
        setupDone = false;
        if (!authenticated)
        {
            DlgPassword dlgPass;
            if(dlgPass.exec() == QDialog::Accepted)
            {
                password = dlgPass.getPassword();
                authenticated = true;
            }
            else
            {
                return;
            }
        }
        if (settings->saveKNAccount())
        {
            ui->lineEditKNUser->setText(settings->karoakeDotNetUser(password));
            ui->lineEditKNPass->setText(settings->karoakeDotNetPass(password));
        }
        if (settings->saveCC())
        {
            ui->lineEditCCN->setText(settings->getCCN(password));
            ui->lineEditCCM->setText(settings->getCCM(password));
            ui->lineEditCCY->setText(settings->getCCY(password));
            ui->lineEditCCV->setText(settings->getCCV(password));
        }
        setupDone = true;
    }
}

void DlgSongShopPurchase::on_pushButtonTestLogin_clicked()
{
    knLoginTest = true;
    shop->knLogin(ui->lineEditKNUser->text(), ui->lineEditKNPass->text());
}

void DlgSongShopPurchase::knLoginSuccess()
{
    if (knLoginTest)
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Login successful!");
        msgBox.setText("Your login to Karoake.NET was successful.");
        msgBox.exec();
        knLoginTest = false;

    }
}

void DlgSongShopPurchase::knLoginFailure()
{
    msgBoxInfo->hide();
    QMessageBox msgBox;
    msgBox.setWindowTitle("Login failed!");
    msgBox.setText("Login failed, incorrect username or password.");
    msgBox.exec();
    knLoginTest = false;
    authenticated = false;
}

void DlgSongShopPurchase::paymentProcessingFailed()
{
    msgBoxInfo->hide();
    QMessageBox msgBox;
    msgBox.setWindowTitle("Payment failed!");
    msgBox.setText("Payment processing failed. Please verify your credit card details or try another card.");
    msgBox.exec();
}

void DlgSongShopPurchase::purchaseSuccess()
{
    msgBoxInfo->hide();
    QMessageBox msgBox;
    msgBox.setWindowTitle("Purchase succeeded!");
    msgBox.setText("Your purchased track has been downloaded and added to your database.");
    msgBox.exec();
    close();
}

void DlgSongShopPurchase::on_pushButtonCancel_clicked()
{
    hide();
}

void DlgSongShopPurchase::on_btnPurchase_clicked()
{
    msgBoxInfo->setWindowTitle("Purchasing Song");
    msgBoxInfo->show();
    if (!shop->loggedIn())
    {
        msgBoxInfo->setText("Logging you in to Karoake.NET...");
        shop->knLogin(ui->lineEditKNUser->text(), ui->lineEditKNPass->text());
        while (!shop->loggedIn() && !shop->loginError())
            QApplication::processEvents();
    }
    if (shop->loginError())
    {
        msgBoxInfo->hide();
        return;
    }
    msgBoxInfo->setText("Purchasing song...");
    shop->setDlSongInfo(artist, title, songId);
    shop->knPurchase(songId, ui->lineEditCCN->text(), ui->lineEditCCM->text(), ui->lineEditCCY->text(), ui->lineEditCCV->text());
}

void DlgSongShopPurchase::on_cbxSaveAccount_stateChanged(int arg1)
{
    if (!setupDone)
        return;

    if ((arg1) && (!authenticated))
    {
        if (settings->passIsSet())
        {
            DlgPassword dlgPass;
            if (dlgPass.exec() == QDialog::Accepted)
            {
                password = dlgPass.getPassword();
                authenticated = true;
                settings->setKaroakeDotNetUser(ui->lineEditKNUser->text(), password);
                settings->setKaraokeDotNetPass(ui->lineEditKNPass->text(), password);
                settings->setSaveKNAccount(true);
            }
            else
            {
                password = "";
                authenticated = false;
                ui->cbxSaveAccount->setChecked(false);
                settings->setSaveKNAccount(false);
            }
        }
        else
        {
            DlgSetPassword dlgSetPw;
            if (dlgSetPw.exec() == QDialog::Accepted)
            {
                password = dlgSetPw.getPassword();
                authenticated = true;
                settings->setKaroakeDotNetUser(ui->lineEditKNUser->text(), password);
                settings->setKaraokeDotNetPass(ui->lineEditKNPass->text(), password);
                settings->setSaveKNAccount(true);
            }
            else
            {
                ui->cbxSaveAccount->setChecked(false);
                settings->setSaveKNAccount(false);
            }
        }
    }
    else if (arg1 && authenticated)
    {
        settings->setKaroakeDotNetUser(ui->lineEditKNUser->text(), password);
        settings->setKaraokeDotNetPass(ui->lineEditKNPass->text(), password);
        settings->setSaveKNAccount(true);
    }
    else
    {
        settings->clearKNAccount();
        settings->setSaveKNAccount(false);
    }
}

void DlgSongShopPurchase::on_cbxSaveCard_stateChanged(int arg1)
{
    if (!setupDone)
        return;
    if ((arg1) && (!authenticated))
    {
        if (settings->passIsSet())
        {
            DlgPassword dlgPass;
            if (dlgPass.exec() == QDialog::Accepted)
            {
                password = dlgPass.getPassword();
                authenticated = true;
                settings->setCC(ui->lineEditCCN->text(), ui->lineEditCCM->text(),ui->lineEditCCY->text(), ui->lineEditCCV->text(), password);
                settings->setSaveCC(true);
            }
            else
            {
                password = "";
                authenticated = false;
                ui->cbxSaveCard->setChecked(false);
                settings->setSaveCC(false);
            }
        }
        else
        {
            DlgSetPassword dlgSetPw;
            if (dlgSetPw.exec() == QDialog::Accepted)
            {
                password = dlgSetPw.getPassword();
                authenticated = true;
                settings->setCC(ui->lineEditCCN->text(), ui->lineEditCCM->text(),ui->lineEditCCY->text(), ui->lineEditCCV->text(), password);
                settings->setSaveCC(true);
            }
            else
            {
                ui->cbxSaveCard->setChecked(false);
                settings->setSaveCC(false);
            }
        }
    }
    else if (arg1 && authenticated)
    {
        settings->setCC(ui->lineEditCCN->text(), ui->lineEditCCM->text(),ui->lineEditCCY->text(), ui->lineEditCCV->text(), password);
        settings->setSaveCC(true);
    }
    else
    {
        settings->clearCC();
        settings->setSaveCC(false);
    }
}


void DlgSongShopPurchase::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
}


void DlgSongShopPurchase::setVisible(bool visible)
{
    QDialog::setVisible(visible);
}

void DlgSongShopPurchase::on_lineEditKNUser_textChanged(const QString &arg1)
{
    if (!setupDone)
        return;
    if (settings->saveKNAccount() && authenticated)
        settings->setKaroakeDotNetUser(arg1, password);
}

void DlgSongShopPurchase::on_lineEditKNPass_textChanged(const QString &arg1)
{
    if (!setupDone)
        return;
    if (settings->saveKNAccount() && authenticated)
        settings->setKaraokeDotNetPass(arg1,password);
}

void DlgSongShopPurchase::on_lineEditCCN_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1)
    if (!setupDone)
        return;
    ui->lineEditCCN->setEchoMode(QLineEdit::Normal);
    setupDone = false;
    if (settings->saveCC() && authenticated)
        settings->setCC(ui->lineEditCCN->text(), ui->lineEditCCM->text(),ui->lineEditCCY->text(), ui->lineEditCCV->text(), password);
    setupDone = true;
}

void DlgSongShopPurchase::on_lineEditCCM_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1)
    if (!setupDone)
        return;
    setupDone = false;
    if (settings->saveCC() && authenticated)
        settings->setCC(ui->lineEditCCN->text(), ui->lineEditCCM->text(),ui->lineEditCCY->text(), ui->lineEditCCV->text(), password);
    setupDone = true;
}

void DlgSongShopPurchase::on_lineEditCCY_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1)
    if (!setupDone)
        return;
    setupDone = false;
    if (settings->saveCC() && authenticated)
        settings->setCC(ui->lineEditCCN->text(), ui->lineEditCCM->text(),ui->lineEditCCY->text(), ui->lineEditCCV->text(), password);
    setupDone = true;
}

void DlgSongShopPurchase::on_lineEditCCV_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1)
    if (!setupDone)
        return;
    setupDone = false;
    if (settings->saveCC() && authenticated)
        settings->setCC(ui->lineEditCCN->text(), ui->lineEditCCM->text(),ui->lineEditCCY->text(), ui->lineEditCCV->text(), password);
    setupDone = true;
}

void DlgSongShopPurchase::on_lineEditCCN_cursorPositionChanged(int arg1, int arg2)
{
    Q_UNUSED(arg1)
    Q_UNUSED(arg2)
    if (!setupDone)
        return;
    ui->lineEditCCN->setEchoMode(QLineEdit::Normal);
}

void DlgSongShopPurchase::on_lineEditCCN_editingFinished()
{
    ui->lineEditCCN->setEchoMode(QLineEdit::Password);
}

void DlgSongShopPurchase::downloadProgress(qint64 received, qint64 total)
{
    msgBoxInfo->setText("Downloading song...");
    msgBoxInfo->setProgress(received, total);
}

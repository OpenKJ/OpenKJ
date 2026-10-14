#ifndef KHREQUESTSDIALOG_H
#define KHREQUESTSDIALOG_H

#include <QDialog>
#include <QItemSelection>
#include "src/models/tablemodelrequests.h"
#include "src/models/tablemodelkaraokesongs.h"
#include "src/models/tablemodelrotation.h"
#include "okjsongbookapi.h"
#include <spdlog/spdlog.h>
#include "settings.h"

namespace Ui {
class DlgRequests;
}

class DlgRequests : public QDialog
{
    Q_OBJECT

private:
    Ui::DlgRequests *ui;
    TableModelRequests *requestsModel;
    //SongDBTableModel *songDbModel;
    TableModelKaraokeSongs dbModel;
    TableModelRotation &rotModel;
    QString rtClickFile;
    int curRequestId;
    QString curSelReqSinger;
    QTimer testTimer;
    std::shared_ptr<spdlog::logger> m_reqLogger;
    std::vector<int> m_prevRequestList;
    OKJSongbookAPI &songbookApi;
    Settings m_settings;

public:
    bool isLibraryLoading() const { return dbModel.isLoading(); }
    explicit DlgRequests(TableModelRotation &rotationModel, OKJSongbookAPI &songbookAPI, QWidget *parent = nullptr);
    int numRequests();
    ~DlgRequests();

signals:
    void addRequestSong(int songId, int singerId, int keyChg);

public slots:
    void databaseAboutToUpdate();
    void databaseUpdateComplete();
    void databaseSongAdded();
    void refreshLibrarySong(int songId);
    void setSongDuration(const QString &path, unsigned int duration);
    void rotationChanged();
    void updateIcons();

private slots:
    void on_pushButtonClose_clicked();
    void requestsModified();
    void requestsChanged(OkjsRequests requests);
    void on_pushButtonSearch_clicked();
    void on_lineEditSearch_returnPressed();
    void requestSelectionChanged(const QItemSelection & current, const QItemSelection & previous);
    void songSelectionChanged(const QItemSelection & current, const QItemSelection & previous);
    void on_radioButtonExistingSinger_toggled(bool checked);
    void on_pushButtonClearReqs_clicked();
    void on_tableViewRequests_clicked(const QModelIndex &index);
    void on_pushButtonAddSong_clicked();
    void on_tableViewSearch_customContextMenuRequested(const QPoint &pos);
    void updateReceived(QTime updateTime);
    void on_buttonRefresh_clicked();
    void sslError();
    void delayError(int seconds);
    void on_checkBoxAccepting_clicked(bool checked);
    void venuesChanged(OkjsVenues venues);
    void on_pushButtonUpdateDb_clicked();
    void on_comboBoxVenue_activated(int index);
    void previewCdg(const std::shared_ptr<okj::KaraokeSong>& song);
    void on_lineEditSearch_textChanged(const QString &arg1);
    void lineEditSearchEscapePressed();
    void autoSizeViews();

    // QWidget interface
    void on_spinBoxKey_valueChanged(int arg1);

    void on_pushButtonWebSearch_clicked();

    void on_pushButtonRunTortureTest_clicked();

protected:
    void resizeEvent(QResizeEvent *event);

    // QWidget interface
protected:
    void showEvent(QShowEvent *event);

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event);
};

#endif // KHREQUESTSDIALOG_H

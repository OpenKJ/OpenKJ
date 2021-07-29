#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QNetworkAccessManager>
#include <QObject>
#include "okjversion.h"
#include "settings.h"

class UpdateChecker : public QObject
{
    Q_OBJECT
private:
    QNetworkAccessManager *manager;
    QString currentVer;
    QString availVersion;
    QString OS;
    QString channel;
    Settings m_settings;

public:
    explicit UpdateChecker(QObject *parent = 0);
    void checkForUpdates();

    QString getOS() const;
    void setOS(const QString &value);

    QString getChannel() const;
    void setChannel(const QString &value);

signals:
    void newVersionAvailable(QString version);

public slots:

private slots:
    void onNetworkReply(QNetworkReply* reply);
    void aOnNetworkReply(QNetworkReply* reply);
    void downloadInstaller();
};

#endif // UPDATECHECKER_H

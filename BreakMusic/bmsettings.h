#ifndef BMSETTINGS_H
#define BMSETTINGS_H

#include <QObject>
#include <QSettings>
#include <QWidget>

class BmSettings : public QObject
{
    Q_OBJECT
public:
    explicit BmSettings(QObject *parent = 0);
    void saveWindowState(QWidget *window);
    void restoreWindowState(QWidget *window);
    bool showFilenames();
    void setShowFilenames(bool show);
    bool showMetadata();
    void setShowMetadata(bool show);
    int volume();
    void setVolume(int volume);
    int playlistIndex();
    void setPlaylistIndex(int index);

signals:

public slots:

private:
    QSettings *settings;


};

#endif // BMSETTINGS_H

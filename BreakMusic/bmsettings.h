#ifndef BMSETTINGS_H
#define BMSETTINGS_H

#include <QObject>
#include <QSettings>
#include <QWidget>
#include <QTableView>
#include <QTreeView>
#include <QSplitter>

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
    void saveColumnWidths(QTreeView *treeView);
    void saveColumnWidths(QTableView *tableView);
    void restoreColumnWidths(QTreeView *treeView);
    void restoreColumnWidths(QTableView *tableView);
    void saveSplitterState(QSplitter *splitter);
    void restoreSplitterState(QSplitter *splitter);

signals:

public slots:

private:
    QSettings *settings;


};

#endif // BMSETTINGS_H

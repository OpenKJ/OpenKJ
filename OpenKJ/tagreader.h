#ifndef TAGREADER_H
#define TAGREADER_H

#include <QObject>
#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>


class TagReader : public QObject
{
    Q_OBJECT
private:
    QString m_artist;
    QString m_title;
    QString m_album;
    QString m_track;
    unsigned int m_duration;
    QString m_path;
    GstDiscoverer *discoverer;

public:
    explicit TagReader(QObject *parent = 0);
    QString getArtist();
    QString getTitle();
    QString getAlbum();
    QString getTrack();
    unsigned int getDuration();
    void setMedia(QString path);
    void taglibTags(QString path);

signals:

public slots:
};

#endif // TAGREADER_H

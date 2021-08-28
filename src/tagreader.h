#ifndef TAGREADER_H
#define TAGREADER_H

#include <QObject>
#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>
#include <spdlog/spdlog.h>
#include <spdlog/async_logger.h>
#include <spdlog/fmt/ostr.h>

std::ostream& operator<<(std::ostream& os, const QString& s);


class TagReader : public QObject
{
    Q_OBJECT
private:
    std::string m_loggingPrefix{"[TagReader]"};
    std::shared_ptr<spdlog::logger> m_logger;
    QString m_artist;
    QString m_title;
    QString m_album;
    QString m_track;
    unsigned int m_duration;
    QString m_path;
    GstDiscoverer *discoverer;

public:
    explicit TagReader(QObject *parent = 0);
    ~TagReader();
    QString getArtist();
    QString getTitle();
    QString getAlbum();
    QString getTrack();
    unsigned int getDuration() const;
    void setMedia(const QString& path);
    void taglibTags(const QString& path);

signals:

public slots:
};

#endif // TAGREADER_H

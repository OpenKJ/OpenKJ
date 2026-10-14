#ifndef OPENKJ_OKJTYPES_H
#define OPENKJ_OKJTYPES_H

#include <QDateTime>
#include <QString>
#include <qmetatype.h>
#include <spdlog/async_logger.h>
#include <spdlog/fmt/ostr.h>
#include "settings.h"

namespace okj {

    struct KaraokeSong {
        int id{0};
        QString artist;
        QString artistL;
        QString title;
        QString titleL;
        QString songid;
        QString songidL;
        int duration{0};
        QString filename;
        QString path;
        QString searchString;
        int plays;
        QDateTime lastPlay;
        bool bad{false};
        bool dropped{false};
    };

    struct HistorySinger {
        int historySingerId{-1};
        QString name;
        int songCount{0};
    };

    struct RotationSinger {
        int id{0};
        QString name;
        int position{0};
        bool regular{false};
        QDateTime addTs;
        bool valid{true};
        std::shared_ptr<spdlog::logger> m_logger;
        std::shared_ptr<Settings> m_settings;
        [[nodiscard]] std::string loggingPrefix() const { return "[RotationSinger] [" + name.toStdString() + "]"; }
        RotationSinger();
        RotationSinger(int id, QString name, int position, bool regular, QDateTime addTs, bool valid = true);
        RotationSinger(const RotationSinger &r1) = default;
        RotationSinger(RotationSinger &&other) = default;
        RotationSinger& operator=(RotationSinger&& other) = default;
        RotationSinger& operator=(const RotationSinger& other) = default;
        [[nodiscard]] bool isValid() const { return valid; }

        // Cached fields
        QString cachedNextSongPath;
        QString cachedNextSongArtist;
        QString cachedNextSongTitle;
        QString cachedNextSongSongId;
        int cachedNextSongDurationSecs{0};
        int cachedNextSongKeyChg{0};
        int cachedNextSongQueueId{-1};
        int cachedNumSongsSung{0};
        int cachedNumSongsUnsung{0};

        [[nodiscard]] QString nextSongPath() const { return cachedNextSongPath; }
        [[nodiscard]] QString nextSongArtist() const { return cachedNextSongArtist; }
        [[nodiscard]] QString nextSongTitle() const { return cachedNextSongTitle; }
        [[nodiscard]] QString nextSongArtistTitle() const {
            if (cachedNextSongArtist.isEmpty() && cachedNextSongTitle.isEmpty()) {
                return " - empty - ";
            }
            return cachedNextSongArtist + " - " + cachedNextSongTitle;
        }
        [[nodiscard]] QString nextSongSongId() const { return cachedNextSongSongId; }
        [[nodiscard]] int nextSongDurationSecs() const { return cachedNextSongDurationSecs; }
        [[nodiscard]] int nextSongKeyChg() const { return cachedNextSongKeyChg; }
        [[nodiscard]] int nextSongQueueId() const { return cachedNextSongQueueId; }
        [[nodiscard]] int numSongsSung() const { return cachedNumSongsSung; }
        [[nodiscard]] int numSongsUnsung() const { return cachedNumSongsUnsung; }
    };

    struct QueueSong {
        int id{0};
        int singerId{0};
        int dbSongId{0};
        bool played{false};
        int keyChange{0};
        int position{0};
        QString artist;
        QString title;
        QString songId;
        int duration{0};
        QString path;
    };

    struct HistorySong {
        unsigned int id{0};
        unsigned int historySinger{0};
        QString filePath;
        QString artist;
        QString title;
        QString songid;
        int keyChange{0};
        int plays{0};
        QDateTime lastPlayed; // unix time
    };
}

Q_DECLARE_METATYPE(okj::KaraokeSong)
Q_DECLARE_METATYPE(std::shared_ptr<okj::KaraokeSong>)
Q_DECLARE_METATYPE(okj::QueueSong)
Q_DECLARE_METATYPE(okj::HistorySong)

std::ostream& operator<<(std::ostream& os, const okj::RotationSinger& s);

#endif //OPENKJ_OKJTYPES_H

/*
 * Copyright (c) 2013-2021 Thomas Isaac Lightburn
 *
 *
 * This file is part of OpenKJ.
 *
 * OpenKJ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
        RotationSinger(const RotationSinger &r1);
        RotationSinger(RotationSinger &&other) = default;
        RotationSinger& operator=(RotationSinger&& other) = default;
        RotationSinger& operator=(const RotationSinger& other) = default;
        [[nodiscard]] bool isValid() const { return valid; }
        [[nodiscard]] QString nextSongPath() const;
        [[nodiscard]] QString nextSongArtist() const;
        [[nodiscard]] QString nextSongTitle() const;
        [[nodiscard]] QString nextSongArtistTitle() const;
        [[nodiscard]] QString nextSongSongId() const;
        [[nodiscard]] int nextSongDurationSecs() const;
        [[nodiscard]] int nextSongKeyChg() const;
        [[nodiscard]] int nextSongQueueId() const;
        [[nodiscard]] int numSongsSung() const;
        [[nodiscard]] int numSongsUnsung() const;
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

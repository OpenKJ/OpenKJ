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
#include <spdlog/async_logger.h>
#include "settings.h"

namespace okj {

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

}

#endif //OPENKJ_OKJTYPES_H

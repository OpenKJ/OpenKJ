#include "okjtypes.h"
#include <QSqlQuery>
#include <QSqlError>
#include <utility>
#include <spdlog/spdlog.h>

std::ostream & operator<<(std::ostream& os, const okj::RotationSinger& s)
{
    return os
        << " RotationSinger(id: " << s.id
        << " position: " << s.position
        << " name: " << s.name.toStdString()
        << " regular: " << s.regular
        << " addTS: " << s.addTs.toString().toStdString()
        << " valid: " << s.valid
        << ")";
}

std::ostream & operator<<(std::ostream& os, const QString& s)
{
    return os << s.toStdString();
}


namespace okj {



    RotationSinger::RotationSinger() {
        m_logger = spdlog::get("logger");
        m_settings = std::make_shared<Settings>();
    }

    RotationSinger::RotationSinger(int id, QString name, int position, bool regular, QDateTime addTs, bool valid)
            : id(id), name(std::move(name)), position(position), regular(regular), addTs(std::move(addTs)),
              valid(valid) {
        m_logger = spdlog::get("logger");
        m_settings = std::make_shared<Settings>();
    }




}

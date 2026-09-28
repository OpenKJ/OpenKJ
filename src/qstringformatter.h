#ifndef QSTRINGFORMATTER_H
#define QSTRINGFORMATTER_H

#include <QString>
#include <ostream>
#include <spdlog/fmt/fmt.h>

// Streaming and formatting support for QString, so it can be passed straight to
// spdlog. Both halves live here because they are one contract: the formatter
// below is implemented in terms of this operator, and a translation unit that
// has one without the other either fails to link or fails to compile.
//
// The operator is defined once, in src/models/tablemodelbreaksongs.cpp.
std::ostream &operator<<(std::ostream &os, const QString &s);

// QString values are passed to spdlog in ~275 places. That worked because fmt
// would implicitly fall back to a type's operator<<(std::ostream&). fmt removed
// that fallback in version 9, so from there on a formatter has to be declared
// explicitly.
//
// ostream_formatter reuses the operator above, so no call site changes. The
// guard keeps fmt 8 — what Debian 12's libspdlog-dev ships — building as before.
#if FMT_VERSION >= 90000
#include <spdlog/fmt/ostr.h>
template<>
struct fmt::formatter<QString> : fmt::ostream_formatter {
};
#endif

#endif // QSTRINGFORMATTER_H

#pragma once

#include <QtCore/QString>
#include <QtCore/QtGlobal>

#if defined(IIFILEPROVIDER_BUILDING_LIBRARY)
#    define IIFILEPROVIDER_EXPORT Q_DECL_EXPORT
#else
#    define IIFILEPROVIDER_EXPORT Q_DECL_IMPORT
#endif

namespace iiFileProvider {

[[nodiscard]] IIFILEPROVIDER_EXPORT QString helloWorld();

} // namespace iiFileProvider

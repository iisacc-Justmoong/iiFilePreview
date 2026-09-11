#pragma once

#include <QtCore/QString>
#include <QtCore/QtGlobal>

#if defined(IIFILEPREVIEW_BUILDING_LIBRARY)
#    define IIFILEPREVIEW_EXPORT Q_DECL_EXPORT
#else
#    define IIFILEPREVIEW_EXPORT Q_DECL_IMPORT
#endif

namespace iiFileProvider {

[[nodiscard]] IIFILEPREVIEW_EXPORT QString helloWorld();

} // namespace iiFileProvider

// Qt Static Plugin Imports for Lupine Editor
// This file is required for static Qt linking to import platform plugins

#include <QtPlugin>

// Import Qt platform plugins for Windows
#ifdef Q_OS_WIN
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
Q_IMPORT_PLUGIN(QMinimalIntegrationPlugin)
Q_IMPORT_PLUGIN(QOffscreenIntegrationPlugin)
#endif

// Import Qt image format plugins
Q_IMPORT_PLUGIN(QGifPlugin)
Q_IMPORT_PLUGIN(QICOPlugin)
Q_IMPORT_PLUGIN(QJpegPlugin)

// Import Qt generic plugins
Q_IMPORT_PLUGIN(QTuioTouchPlugin)

// Import Qt style plugins for Windows
#ifdef Q_OS_WIN
Q_IMPORT_PLUGIN(QModernWindowsStylePlugin)
#endif

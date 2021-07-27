# references:
# https://github.com/cspiegel/qmmp-adplug
# https://github.com/cspiegel/qmmp-openmpt

QT      += widgets

HEADERS += decodergmefactory.h \
           decoder_gme.h \
           gmehelper.h \
           settingsdialog.h
    
SOURCES += decodergmefactory.cpp \
           decoder_gme.cpp \
           gmehelper.cpp \
           settingsdialog.cpp

FORMS += settingsdialog.ui

CONFIG += warn_on plugin link_pkgconfig

TEMPLATE = lib

QMAKE_CLEAN += lib$${TARGET}.so

unix {
	CONFIG += link_pkgconfig
	PKGCONFIG += qmmp libgme
	
	QMMP_PREFIX = $$system(pkg-config qmmp --variable=prefix)
	PLUGIN_DIR = $$system(pkg-config qmmp --variable=plugindir)/Input
	LOCAL_INCLUDES = $${QMMP_PREFIX}/include
	LOCAL_INCLUDES -= $$QMAKE_DEFAULT_INCDIRS
	INCLUDEPATH += $$LOCAL_INCLUDES
	
	plugin.path = $${PLUGIN_DIR}
	plugin.files = lib$${TARGET}.so
	INSTALLS += plugin
}

win32 {
    LIBS += -L$$EXTRA_PREFIX/libgme/lib -lgme
}

# NOTICE:
#
# Application name defined in TARGET has a corresponding QML filename.
# If name defined in TARGET is changed, the following needs to be done
# to match new name:
#   - corresponding QML filename must be changed
#   - desktop icon filename must be changed
#   - desktop filename must be changed
#   - icon definition filename in desktop file must be changed
#   - translation filenames have to be changed

TARGET = harbour-dollfin

CONFIG += sailfishapp

# Release build: no debug symbols in the packaged binary.
QMAKE_CXXFLAGS_RELEASE += -g0
QMAKE_LFLAGS_RELEASE += -s

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

QT += dbus

INCLUDEPATH += $$PWD/src

SOURCES += \
    src/harbour-dollfin.cpp \
    src/app/appcontroller.cpp

HEADERS += src/app/appcontroller.h

include(src/core/core.pri)

DISTFILES += \
    qml/harbour-dollfin.qml \
    qml/dollfin.js \
    qml/cover/CoverPage.qml \
    qml/pages/TodayPage.qml \
    qml/pages/MedicationsPage.qml \
    qml/pages/MedicationEditPage.qml \
    qml/pages/StockPage.qml \
    qml/pages/RegimensPage.qml \
    qml/pages/RegimenEditPage.qml \
    qml/pages/ScheduleUpdateDialog.qml \
    qml/pages/HistoryPage.qml \
    qml/pages/LabsPage.qml \
    qml/pages/AnalyteResultsPage.qml \
    qml/pages/AnalyteGraphPage.qml \
    qml/pages/LabEditPage.qml \
    qml/pages/SettingsPage.qml \
    rpm/$${TARGET}.spec \
    translations/*.ts \
    $${TARGET}.desktop

# to disable building translations every time, comment out the
# following CONFIG line
CONFIG += sailfishapp_i18n
TRANSLATIONS += \
    translations/$${TARGET}-nl.ts \
    translations/$${TARGET}-nb.ts

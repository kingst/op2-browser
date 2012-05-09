TEMPLATE = app

TARGET = op-ui

DEFINES += QT_NO_CAST_FROM_ASCII

include(src.pri)

SOURCES += main.cpp

DESTDIR = 

isEmpty(QMAKE_LRELEASE) {
    win32:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\lrelease.exe
    else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}

TRANSLATIONS += \
    locale/cs_CZ.ts \
    locale/da.ts \
    locale/de.ts \
    locale/en.ts \
    locale/es.ts \
    locale/et.ts \
    locale/fr_FR.ts \
    locale/fr_CA.ts \
    locale/hu.ts \
    locale/it.ts \
    locale/pl.ts \
    locale/pt_BR.ts \
    locale/ru.ts \
    locale/tr.ts \
    locale/nl.ts \
    locale/zh_CN.ts

updateqm.input = TRANSLATIONS
updateqm.output = .qm/locale/${QMAKE_FILE_BASE}.qm
updateqm.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} -qm .qm/locale/${QMAKE_FILE_BASE}.qm
updateqm.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += updateqm

PRE_TARGETDEPS += compiler_updateqm_make_all

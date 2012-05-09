CONFIG += qt warn_on release
contains(QT_BUILD_PARTS, tools): CONFIG += uitools
else : DEFINES += QT_NO_UITOOLS

INCLUDEPATH += $$PWD

INCLUDEPATH += . \
	   ../kernel/

DEPENDPATH += $$PWD

RCC_DIR = .rcc
UI_DIR = .ui
MOC_DIR = .moc
OBJECTS_DIR = .obj

FORMS += \
    aboutdialog.ui \
    addbookmarkdialog.ui \
    bookmarks.ui \
    cookies.ui \
    cookiesexceptions.ui \
    downloaditem.ui \
    downloads.ui \
    history.ui \
    passworddialog.ui \
    proxy.ui \
    searchbanner.ui \
    settings.ui

HEADERS += \
    aboutdialog.h \
    bookmarks.h \
    browserapplication.h \
    browsermainwindow.h \
    clearprivatedata.h \
    downloadmanager.h \
    edittableview.h \
    edittreeview.h \
    history.h \
    locationbar.h \
    locationbar_p.h \
    modelmenu.h \
    searchlineedit.h \
    settings.h \
    tabbar.h \
    tabwidget.h \
    toolbarsearch.h \
    webactionmapper.h \
    webview.h \
    frameview.h \
    webviewsearch.h \
    xbel.h \
    ../kernel/Message.h

SOURCES += \
    aboutdialog.cpp \
    bookmarks.cpp \
    browserapplication.cpp \
    browsermainwindow.cpp \
    clearprivatedata.cpp \
    downloadmanager.cpp \
    edittableview.cpp \
    edittreeview.cpp \
    history.cpp \
    locationbar.cpp \
    modelmenu.cpp \
    searchlineedit.cpp \
    settings.cpp \
    tabbar.cpp \
    tabwidget.cpp \
    toolbarsearch.cpp \
    webactionmapper.cpp \
    webview.cpp \
    frameview.cpp \
    webviewsearch.cpp \
    xbel.cpp \
    ../kernel/Message.cpp

include(utils/utils.pri)

RESOURCES += data/data.qrc


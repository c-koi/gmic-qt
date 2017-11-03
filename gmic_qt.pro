#
# Set HOST variable to define target host software.
# Possible values are "none" or "gimp" (or hopefully some day "krita")
#
#

!defined(HOST,var) { HOST = gimp }

!defined(PRERELEASE, var) {

# calling 'date' directly crashes on MSYS2!

   PRERELEASE = $$system(bash pre_version.sh)
}

#
#
#
#

#
# Check Qt version (>= 5.2)
#
!greaterThan(QT_MAJOR_VERSION, 4):error("You need Qt 5.2 or greater to build this program.")
equals(QT_MAJOR_VERSION,5) {
  !greaterThan(QT_MINOR_VERSION, 1):error("You need Qt 5.2 or greater to build this program.")
}

#
# Check that pkg-config is installed (qmake error messages are misleading, if not)
#

!system(bash pkg-config-check.sh):error("pkg-config is not installed")

TEMPLATE = app
QT += widgets network
CONFIG	+= qt c++11
CONFIG	+= warn_on
QT_CONFIG -= no-pkg-config
CONFIG += link_pkgconfig
VERSION = 0.0.0

PKGCONFIG += fftw3 zlib libpng libcurl

equals( HOST, "gimp" ) {
  PKGCONFIG += gimp-2.0
}

DEFINES += cimg_use_fftw3 cimg_use_zlib
DEFINES += gmic_build cimg_use_abort gmic_is_parallel cimg_use_curl cimg_use_png cimg_use_rng
DEFINES += cimg_appname="\\\"gmic\\\""

!defined(GMIC_PATH,var) {
  GMIC_PATH = ../gmic/src
}

message( GMIC PATH $$GMIC_PATH )

!isEmpty(PRERELEASE) {
  message( Prerelease date is $$PRERELEASE )
  DEFINES += gmic_prerelease="\\\"$$PRERELEASE\\\""
}

!win32 {
 LIBS += -lfftw3_threads
 DEFINES += cimg_display=1
}

win32 {
 DEFINES += _IS_WINDOWS_
 LIBS += -mwindows -lpthread -DPSAPI_VERSION=1 -lpsapi -lgdi32
 DEFINES += cimg_display=2
}

linux {
  DEFINES += _IS_LINUX_
  PKGCONFIG += x11
  message( Linux platform )
}

equals( HOST, "gimp") {
 TARGET = gmic_gimp_qt
 SOURCES += src/host_gimp.cpp
 DEFINES += GMIC_HOST=gimp_qt
 DEFINES += GIMP_DISABLE_DEPRECATED
 message(Target host software is GIMP)
}


equals( HOST, "none") {
 TARGET = gmic_qt
 DEFINES += GMIC_HOST=standalone
 SOURCES += src/host_none.cpp
 SOURCES += src/standalone/ImageDialog.cpp
 HEADERS += include/standalone/ImageDialog.h
 message(Building standalone version)
}

equals( HOST, "krita") {
 TARGET = gmic_krita_qt
 SOURCES += src/host_krita.cpp
 DEFINES += GMIC_HOST=krita
 message(Target host software is Krita)
}

# enable OpenMP by default on with g++, except on OS X
!macx:*g++* {
    CONFIG += openmp
}

# use qmake CONFIG+=openmp ... to force using openmp
# For example, on OS X with GCC 4.8 installed:
# qmake -spec unsupported/macx-clang QMAKE_CXX=g++-4.8 QMAKE_LINK=g++-4.8 CONFIG+=openmp
# Notes:
#  - the compiler name is g++-4.8 on Homebrew and g++-mp-4.8 on MacPorts
#  - we use the unsupported/macx-clang config because macx-g++ uses arch flags that are not recognized by GNU GCC
openmp {
    message(OpenMP enabled)
    DEFINES += cimg_use_openmp
    QMAKE_CXXFLAGS_DEBUG += -fopenmp
    QMAKE_CXXFLAGS_RELEASE += -fopenmp
    QMAKE_LFLAGS_DEBUG += -fopenmp
    QMAKE_LFLAGS_RELEASE += -fopenmp
}

DEFINES += gmic_gui gmic_build gmic_is_parallel cimg_use_abort

INCLUDEPATH	+= $$PWD $$PWD/include $$GMIC_PATH

DEPENDPATH += $$PWD/include $$PWD/images

HEADERS +=  include/ProgressInfoWidget.h include/FilterThread.h include/MultilineTextParameterWidget.h include/MainWindow.h include/ProgressInfoWindow.h include/BoolParameter.h  include/FiltersTreeFilterItem.h include/ConstParameter.h include/FiltersTreeAbstractFilterItem.h include/LinkParameter.h include/Common.h include/PreviewWidget.h include/ButtonParameter.h include/ChoiceParameter.h include/IntParameter.h include/SearchFieldWidget.h include/FolderParameter.h include/ImageTools.h include/SeparatorParameter.h include/GmicStdlibParser.h include/gmic_qt.h include/FiltersTreeItemDelegate.h include/NoteParameter.h include/DialogSettings.h include/TextParameter.h include/host.h include/ParametersCache.h include/FiltersTreeAbstractItem.h include/AbstractParameter.h include/FloatParameter.h include/ImageConverter.h include/ColorParameter.h include/FiltersTreeFaveItem.h include/Updater.h include/FiltersTreeFolderItem.h include/FilterParamsWidget.h include/InOutPanel.h include/ClickableLabel.h include/FileParameter.h include/HeadlessProcessor.h include/FiltersVisibilityMap.h include/HtmlTranslator.h include/StoredFave.h include/ZoomLevelSelector.h \
    include/LanguageSelectionWidget.h \
    include/TreeView.h

HEADERS += $$GMIC_PATH/gmic.h

SOURCES +=  src/FolderParameter.cpp src/ParametersCache.cpp src/gmic_qt.cpp src/TextParameter.cpp src/ColorParameter.cpp  src/FilterParamsWidget.cpp src/FiltersTreeFaveItem.cpp src/FiltersTreeAbstractItem.cpp src/FileParameter.cpp src/GmicStdlibParser.cpp src/ImageTools.cpp src/FiltersTreeFolderItem.cpp src/ProgressInfoWindow.cpp src/IntParameter.cpp src/LayersExtentProxy.cpp src/FiltersTreeItemDelegate.cpp src/FilterThread.cpp src/SeparatorParameter.cpp src/NoteParameter.cpp src/MainWindow.cpp  src/ConstParameter.cpp src/ImageConverter.cpp src/BoolParameter.cpp src/DialogSettings.cpp src/ButtonParameter.cpp src/FloatParameter.cpp src/ProgressInfoWidget.cpp src/AbstractParameter.cpp src/PreviewWidget.cpp src/ClickableLabel.cpp src/FiltersTreeAbstractFilterItem.cpp src/InOutPanel.cpp src/LinkParameter.cpp src/ChoiceParameter.cpp src/FiltersTreeFilterItem.cpp  src/MultilineTextParameterWidget.cpp src/SearchFieldWidget.cpp src/Updater.cpp src/HeadlessProcessor.cpp src/FiltersVisibilityMap.cpp src/HtmlTranslator.cpp src/StoredFave.cpp src/ZoomLevelSelector.cpp \
    src/LanguageSelectionWidget.cpp \
    src/TreeView.cpp

SOURCES += $$GMIC_PATH/gmic.cpp

# ALL_FORMS
FORMS +=  ui/inoutpanel.ui ui/multilinetextparameterwidget.ui ui/progressinfowindow.ui ui/dialogsettings.ui ui/progressinfowidget.ui ui/mainwindow.ui ui/SearchFieldWidget.ui ui/headlessprogressdialog.ui ui/zoomlevelselector.ui ui/languageselectionwidget.ui

RESOURCES = gmic_qt.qrc translations.qrc

TRANSLATIONS = \
translations/cs.ts \
translations/de.ts \
translations/es.ts \
translations/fr.ts \
translations/id.ts \
translations/it.ts \
translations/nl.ts \
translations/pl.ts \
translations/pt.ts \
translations/ru.ts \
translations/ua.ts \
translations/ja.ts \
translations/zh.ts \
translations/zh_tw.ts

# PRE_TARGETDEPS +=

QMAKE_CXXFLAGS_RELEASE += -O3 -s
QMAKE_LFLAGS_RELEASE += -s
QMAKE_CXXFLAGS_DEBUG += -Dcimg_verbosity=3

unix { DEFINES += cimg_use_vt100 }

CONFIG(release, debug|release) {
    message(Release build)
    DEFINES += QT_NO_DEBUG_OUTPUT
}

CONFIG(debug, debug|release) {
    message(Debug build)
    DEFINES += _GMIC_QT_DEBUG_
    QMAKE_CXXFLAGS_DEBUG += -fsanitize=address
    QMAKE_LFLAGS_DEBUG += -fsanitize=address
}

UI_DIR = .ui
MOC_DIR = .moc
RCC_DIR = .qrc
OBJECTS_DIR = .obj

unix:!macx { DEFINES += _IS_UNIX_ }
macx {  DEFINES += _IS_MACOS_ }

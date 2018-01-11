#
# Set HOST variable to define target host software.
# Possible values are "none" or "gimp" (or hopefully some day "krita")
#
#

!defined(HOST,var) { HOST = krita }

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

!exists( $$GMIC_PATH/gmic.cpp ) {
 error("G'MIC repository is missing (" $$GMIC_PATH ")")
}

#
# Make sure CImg.h is in G'MIC source tree
#
!exists( $$GMIC_PATH/CImg.h ) {
  message( "CImg.h is missing. Trying to get it..." )
  !system(make -C $$GMIC_PATH CImg.h) {
    error("Could not get CImg.h from G'MIC repository")
  }
  !exists($$GMIC_PATH/CImg.h) {
    error("Could not get CImg.h from G'MIC repository")
  }
  message("CImg.h found")
}

#
# Make sure gmic_stdlib.h is in G'MIC source tree
#
!exists( $$GMIC_PATH/gmic_stdlib.h ) {
  message( "gmic_stdlib.h is missing. Trying to get it..." )
  !system(make -C $$GMIC_PATH gmic_stdlib.h) {
    error("Could not get gmic_stdlib.h from G'MIC repository")
  }
  !exists($$GMIC_PATH/gmic_stdlib.h) {
    error("Could not get gmic_stdlib.h from G'MIC repository")
  }
  message("gmic_stdlib.h found")
}

# Make sure CImg and gmic are the same version
GMIC_VERSION = $$system(bash check_versions.sh $$GMIC_PATH gmic)
CIMG_VERSION = $$system(bash check_versions.sh $$GMIC_PATH CImg)
message("G'MIC version is" $$GMIC_VERSION)
message("CImg version is" $$CIMG_VERSION)
!system(bash check_versions.sh $$GMIC_PATH check):{
   error("Version numbers of files 'gmic.h' (" $$GMIC_VERSION ") and 'CImg.h' (" $$CIMG_VERSION ") mismatch")
}



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
 SOURCES += src/Host/Gimp/host_gimp.cpp
 DEFINES += GMIC_HOST=gimp_qt
 DEFINES += GIMP_DISABLE_DEPRECATED
 DEPENDPATH += $$PWD/src/Host/Gimp
 message(Target host software is GIMP)
}


equals( HOST, "none") {
 TARGET = gmic_qt
 DEFINES += GMIC_HOST=standalone
 SOURCES += src/Host/None/host_none.cpp
 SOURCES += src/Host/None/ImageDialog.cpp
 HEADERS += src/Host/None/ImageDialog.h
 DEPENDPATH += $$PWD/src/Host/None
 message(Building standalone version)
}

equals( HOST, "krita") {
 TARGET = gmic_krita_qt
 SOURCES += src/Host/Krita/host_krita.cpp
 DEFINES += GMIC_HOST=krita
 DEPENDPATH += $$PWD/src/Host/Krita
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

INCLUDEPATH	+= $$PWD $$PWD/src $$GMIC_PATH
DEPENDPATH += $$PWD/src \
              $$PWD/src/Host \
              $$PWD/src/FilterParameters \
              $$PWD/src/FilterSelector \
              $$PWD/src/FilterSelector/FiltersView \

HEADERS +=  \
  src/ClickableLabel.h \
  src/Common.h \
  src/DialogSettings.h \
  src/FilterParameters/AbstractParameter.h \
  src/FilterParameters/BoolParameter.h \
  src/FilterParameters/ButtonParameter.h \
  src/FilterParameters/ChoiceParameter.h \
  src/FilterParameters/ColorParameter.h \
  src/FilterParameters/ConstParameter.h \
  src/FilterParameters/FileParameter.h \
  src/FilterParameters/FilterParametersWidget.h \
  src/FilterParameters/FloatParameter.h \
  src/FilterParameters/FolderParameter.h \
  src/FilterParameters/IntParameter.h \
  src/FilterParameters/LinkParameter.h \
  src/FilterParameters/MultilineTextParameterWidget.h \
  src/FilterParameters/NoteParameter.h \
  src/FilterParameters/SeparatorParameter.h \
  src/FilterParameters/TextParameter.h \
  src/FilterSelector/FiltersModel.h \
  src/FilterSelector/FiltersModelReader.h \
  src/FilterSelector/FiltersPresenter.h \
  src/FilterSelector/FiltersView/FiltersView.h \
  src/FilterSelector/FiltersView/TreeView.h \
  src/FiltersVisibilityMap.h \
  src/FilterThread.h \
  src/gmic_qt.h \
  src/GmicStdlibParser.h \
  src/HeadlessProcessor.h \
  src/Host/host.h \
  src/HtmlTranslator.h \
  src/ImageConverter.h \
  src/ImageTools.h \
  src/InOutPanel.h \
  src/LanguageSelectionWidget.h \
  src/LayersExtentProxy.h \
  src/MainWindow.h \
  src/ParametersCache.h \
  src/PreviewWidget.h \
  src/ProgressInfoWidget.h \
  src/ProgressInfoWindow.h \
  src/SearchFieldWidget.h \
  src/Updater.h \
  src/ZoomLevelSelector.h \
  src/FilterSelector/FiltersView/FilterTreeFolder.h \
  src/FilterSelector/FiltersView/FilterTreeItem.h \
  src/FilterSelector/FavesModel.h \
  src/FilterSelector/FavesModelReader.h \
  src/FilterSelector/FiltersView/FilterTreeAbstractItem.h \
  src/FilterSelector/FiltersView/FilterTreeItemDelegate.h \
  src/FilterSelector/FavesModelWriter.h

HEADERS += $$GMIC_PATH/gmic.h
HEADERS += $$GMIC_PATH/CImg.h
HEADERS += $$GMIC_PATH/gmic_stdlib.h

SOURCES += \
  src/ClickableLabel.cpp \
  src/DialogSettings.cpp \
  src/FilterParameters/AbstractParameter.cpp \
  src/FilterParameters/BoolParameter.cpp \
  src/FilterParameters/ButtonParameter.cpp \
  src/FilterParameters/ChoiceParameter.cpp \
  src/FilterParameters/ColorParameter.cpp \
  src/FilterParameters/ConstParameter.cpp \
  src/FilterParameters/FileParameter.cpp \
  src/FilterParameters/FilterParametersWidget.cpp \
  src/FilterParameters/FloatParameter.cpp \
  src/FilterParameters/FolderParameter.cpp \
  src/FilterParameters/IntParameter.cpp \
  src/FilterParameters/LinkParameter.cpp \
  src/FilterParameters/MultilineTextParameterWidget.cpp \
  src/FilterParameters/NoteParameter.cpp \
  src/FilterParameters/SeparatorParameter.cpp \
  src/FilterParameters/TextParameter.cpp \
  src/FilterSelector/FiltersModel.cpp \
  src/FilterSelector/FiltersModelReader.cpp \
  src/FilterSelector/FiltersPresenter.cpp \
  src/FilterSelector/FiltersView/FiltersView.cpp \
  src/FilterSelector/FiltersView/TreeView.cpp \
  src/FiltersVisibilityMap.cpp \
  src/FilterThread.cpp \
  src/gmic_qt.cpp \
  src/GmicStdlibParser.cpp \
  src/HeadlessProcessor.cpp \
  src/HtmlTranslator.cpp \
  src/ImageConverter.cpp \
  src/ImageTools.cpp \
  src/InOutPanel.cpp \
  src/LanguageSelectionWidget.cpp \
  src/LayersExtentProxy.cpp \
  src/MainWindow.cpp \
  src/ParametersCache.cpp \
  src/PreviewWidget.cpp \
  src/ProgressInfoWidget.cpp \
  src/ProgressInfoWindow.cpp \
  src/SearchFieldWidget.cpp \
  src/Updater.cpp \
  src/ZoomLevelSelector.cpp \
  src/FilterSelector/FiltersView/FilterTreeItem.cpp \
  src/FilterSelector/FiltersView/FilterTreeFolder.cpp \
  src/FilterSelector/FavesModel.cpp \
  src/FilterSelector/FavesModelReader.cpp \
  src/FilterSelector/FiltersView/FilterTreeAbstractItem.cpp \
  src/FilterSelector/FiltersView/FilterTreeItemDelegate.cpp \
  src/FilterSelector/FavesModelWriter.cpp

SOURCES += $$GMIC_PATH/gmic.cpp

# ALL_FORMS
FORMS +=  ui/inoutpanel.ui ui/multilinetextparameterwidget.ui ui/progressinfowindow.ui ui/dialogsettings.ui ui/progressinfowidget.ui ui/mainwindow.ui ui/SearchFieldWidget.ui ui/headlessprogressdialog.ui ui/zoomlevelselector.ui ui/languageselectionwidget.ui \
    ui/filtersview.ui

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
#SEB    QMAKE_CXXFLAGS_DEBUG += -fsanitize=address
#SEB    QMAKE_LFLAGS_DEBUG += -fsanitize=address
}

UI_DIR = .ui
MOC_DIR = .moc
RCC_DIR = .qrc
OBJECTS_DIR = .obj

unix:!macx { DEFINES += _IS_UNIX_ }
macx {  DEFINES += _IS_MACOS_ }

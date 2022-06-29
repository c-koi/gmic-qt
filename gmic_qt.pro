#
# Set HOST variable to define target host software.
# Possible values are "none", "gimp", "gimp3" (experimental) "krita" and "paintdotnet"
#
#

!defined(HOST,var) { HOST = gimp }

!defined(GMIC_DYNAMIC_LINKING,var) { GMIC_DYNAMIC_LINKING = off }

!defined(ASAN,var) { ASAN = off }

!defined(PRERELEASE, var) {
# calling 'date' directly crashes on MSYS2!
   PRERELEASE = $$system(bash pre_version.sh)
}

# Possible values are "gcc" or "clang"
!defined(COMPILER,var) { COMPILER = gcc }

# Possible values are "on" or "off"
!defined(LTO,var) { LTO=off }

#
#
#
#

# For debugging purpose
!defined(TIMING,var) { TIMING = off }

#
# Check Qt version (>= 5.2)
#
!greaterThan(QT_MAJOR_VERSION, 4):error("You need Qt 5.2 or greater to build this program.")
equals(QT_MAJOR_VERSION,5) {
  !greaterThan(QT_MINOR_VERSION, 1):error("You need Qt 5.2 or greater to build this program.")
}

DEFINES += QT_DEPRECATED_WARNINGS

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

PKGCONFIG += fftw3 zlib libpng libjpeg libcurl

equals( HOST, "gimp" ) {
  PKGCONFIG += gimp-2.0
}

equals( HOST, "gimp3" ) {
  PKGCONFIG += gimp-3.0
}

DEFINES += cimg_use_cpp11=1
DEFINES += cimg_use_fftw3 cimg_use_zlib
DEFINES += cimg_use_abort gmic_is_parallel cimg_use_curl cimg_use_png cimg_use_jpeg
DEFINES += cimg_appname="\\\"gmic\\\""

equals(TIMING, "on") {
DEFINES += _TIMING_ENABLED_
message(Timing is enabled)
}

defined(GMIC_PATH, var) {
  message("GMIC_PATH is set ("$$GMIC_PATH")")
}
!defined(GMIC_PATH, var):exists(../src/gmic.cpp) {
  message(GMIC_PATH was not set: Found gmic sources in ../src)
  GMIC_PATH = ../src
}
!defined(GMIC_PATH, var):exists(../gmic/src/gmic.cpp) {
  message(GMIC_PATH was not set: Found gmic sources in ../gmic/src)
  GMIC_PATH = ../gmic/src
}
defined(GMIC_PATH, var):!exists( $$GMIC_PATH/gmic.cpp ) {
 error("G'MIC repository was not found ("$$GMIC_PATH")")
}

message("G'MIC repository was found ("$$GMIC_PATH")")

equals( COMPILER, "clang" ) {
 message("Compiler is clang++")
 QMAKE_CXX = clang++
 QMAKE_LINK = clang++
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
# Make sure gmic_stdlib_community.h is in G'MIC source tree
#
!exists( $$GMIC_PATH/gmic_stdlib_community.h ) {
  message( "gmic_stdlib_community.h is missing. Trying to get it..." )
  !system(make -C $$GMIC_PATH gmic_stdlib_community.h) {
    error("Could not get gmic_stdlib_community.h from G'MIC repository")
  }
  !exists($$GMIC_PATH/gmic_stdlib_community.h) {
    error("Could not get gmic_stdlib_community.h from G'MIC repository")
  }
  message("gmic_stdlib_community.h found")
}

# Make sure CImg, gmic and gmic_stdlib_community.h are the same version
GMIC_VERSION = $$system(bash check_versions.sh $$GMIC_PATH gmic)
STDLIB_VERSION = $$system(bash check_versions.sh $$GMIC_PATH stdlib)
CIMG_VERSION = $$system(bash check_versions.sh $$GMIC_PATH CImg)
message("G'MIC version is ................." $$GMIC_VERSION)
message("gmic_stdlib_community.h version is" $$STDLIB_VERSION)
message("CImg version is .................." $$CIMG_VERSION)
!equals(GMIC_VERSION, $$CIMG_VERSION):{
   error("Version numbers of files 'gmic.h' (" $$GMIC_VERSION ") and 'CImg.h' (" $$CIMG_VERSION ") mismatch")
}
!equals(GMIC_VERSION, $$STDLIB_VERSION):{
   error("Version numbers of files 'gmic.h' (" $$GMIC_VERSION ") and 'gmic_stdlib_community.h' (" $$STDLIB_VERSION ") mismatch")
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

equals( HOST, "gimp")|equals( HOST, "gimp3") {
 TARGET = gmic_gimp_qt
 SOURCES += src/Host/Gimp/host_gimp.cpp
 DEFINES += GMIC_HOST=gimp
 DEFINES += GIMP_DISABLE_DEPRECATED
 DEPENDPATH += $$PWD/src/Host/Gimp
 message(Target host software is GIMP)
}

equals( HOST, "none") {
 TARGET = gmic_qt
 DEFINES += GMIC_HOST=standalone
 HEADERS += src/Host/None/ImageDialog.h \
            src/Host/None/JpegQualityDialog.h
 SOURCES += src/Host/None/host_none.cpp \
            src/Host/None/ImageDialog.cpp \
            src/Host/None/JpegQualityDialog.cpp
 FORMS += src/Host/None/jpegqualitydialog.ui
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

equals( HOST, "paintdotnet") {
 TARGET = gmic_paintdotnet_qt
 SOURCES += src/Host/PaintDotNet/host_paintdotnet.cpp
 DEFINES += GMIC_HOST=paintdotnet
 DEPENDPATH += $$PWD/src/Host/PaintDotNet
 message(Target host software is Paint.NET)
}

equals( HOST, "8bf") {
 TARGET = gmic_8bf_qt
 SOURCES += src/Host/8bf/host_8bf.cpp
 DEFINES += GMIC_HOST=plugin8bf
 DEPENDPATH += $$PWD/src/Host/8bf
 message(Target host software is 8bf filter)
}

# enable OpenMP by default on with g++, except on OS X
!macx:*g++* {
    CONFIG += openmp
}
!macx:equals(COMPILER,"clang") {
    CONFIG += openmp
}


# use qmake CONFIG+=openmp ... to force using openmp
# For example, on OS X with GCC 4.8 installed:
# qmake -spec unsupported/macx-clang QMAKE_CXX=g++-4.8 QMAKE_LINK=g++-4.8 CONFIG+=openmp
# Notes:
#  - the compiler name is g++-4.8 on Homebrew and g++-mp-4.8 on MacPorts
#  - we use the unsupported/macx-clang config because macx-g++ uses arch flags that are not recognized by GNU GCC
openmp:equals(COMPILER,"gcc") {
    message("OpenMP enabled, with g++")
    DEFINES += cimg_use_openmp
    QMAKE_CXXFLAGS_DEBUG += -fopenmp
    QMAKE_CXXFLAGS_RELEASE += -fopenmp
    QMAKE_LFLAGS_DEBUG += -fopenmp
    QMAKE_LFLAGS_RELEASE += -fopenmp
}

openmp:equals(COMPILER,"clang") {
    message("OpenMP enabled, with clang++")
    DEFINES += cimg_use_openmp
    QMAKE_CXXFLAGS_DEBUG += -fopenmp=libomp -I/usr/lib/gcc/x86_64-redhat-linux/7/include/
    QMAKE_CXXFLAGS_RELEASE += -fopenmp=libomp  -I/usr/lib/gcc/x86_64-redhat-linux/7/include/
    QMAKE_LFLAGS_DEBUG += -fopenmp=libomp
    QMAKE_LFLAGS_RELEASE += -fopenmp=libomp
}

CONFIG(release, debug|release):gcc|clang:equals(LTO,"on") {
    message("Link Time Optimizer enabled")
    QMAKE_CXXFLAGS_RELEASE += -flto
    QMAKE_LFLAGS_RELEASE += -flto
}

DEFINES += gmic_gui gmic_core gmic_is_parallel gmic_community cimg_use_abort

INCLUDEPATH	+= $$PWD $$PWD/src $$GMIC_PATH
DEPENDPATH += $$PWD/src \
              $$PWD/src/Host \
              $$PWD/src/FilterParameters \
              $$PWD/src/FilterSelector \
              $$PWD/src/FilterSelector/FiltersView \

HEADERS +=  \
  src/ClickableLabel.h \
  src/Common.h \
  src/FilterParameters/CustomSpinBox.h \
  src/GmicQt.h \
  src/Host/GmicQtHost.h \
  src/OverrideCursor.h \
  src/DialogSettings.h \
  src/FilterParameters/AbstractParameter.h \
  src/FilterParameters/BoolParameter.h \
  src/FilterParameters/ButtonParameter.h \
  src/FilterParameters/ChoiceParameter.h \
  src/FilterParameters/ColorParameter.h \
  src/FilterParameters/ConstParameter.h \
  src/FilterParameters/CustomDoubleSpinBox.h \
  src/FilterParameters/FileParameter.h \
  src/FilterParameters/FilterParametersWidget.h \
  src/FilterParameters/FloatParameter.h \
  src/FilterParameters/FolderParameter.h \
  src/FilterParameters/IntParameter.h \
  src/FilterParameters/LinkParameter.h \
  src/FilterParameters/MultilineTextParameterWidget.h \
  src/FilterParameters/NoteParameter.h \
  src/FilterParameters/PointParameter.h \
  src/FilterParameters/SeparatorParameter.h \
  src/FilterParameters/TextParameter.h \
  src/FilterSelector/FiltersModel.h \
  src/FilterSelector/FiltersModelReader.h \
  src/FilterSelector/FiltersPresenter.h \
  src/FilterSelector/FiltersView/FiltersView.h \
  src/FilterSelector/FiltersView/TreeView.h \
  src/FilterSelector/FiltersVisibilityMap.h \
  src/FilterSelector/FilterTagMap.h \
  src/CroppedImageListProxy.h \
  src/CroppedActiveLayerProxy.h \
  src/FilterSyncRunner.h \
  src/FilterThread.h \
  src/FilterTextTranslator.h \
  src/Globals.h \
  src/GmicStdlib.h \
  src/GmicProcessor.h \
  src/HeadlessProcessor.h \
  src/HtmlTranslator.h \
  src/IconLoader.h \
  src/ImageTools.h \
  src/InputOutputState.h \
  src/KeypointList.h \
  src/LayersExtentProxy.h \
  src/Logger.h \
  src/LanguageSettings.h \
  src/MainWindow.h \
  src/Misc.h \
  src/ParametersCache.h \
  src/PersistentMemory.h \
  src/Settings.h \
  src/Tags.h \
  src/TimeLogger.h \
  src/Updater.h \
  src/Utils.h \
  src/Widgets/VisibleTagSelector.h \
  src/ZoomConstraint.h \
  src/FilterSelector/FiltersView/FilterTreeFolder.h \
  src/FilterSelector/FiltersView/FilterTreeItem.h \
  src/FilterSelector/FavesModel.h \
  src/FilterSelector/FavesModelReader.h \
  src/FilterSelector/FiltersView/FilterTreeAbstractItem.h \
  src/FilterSelector/FiltersView/FilterTreeItemDelegate.h \
  src/FilterSelector/FavesModelWriter.h \
  src/Widgets/PreviewWidget.h \
  src/Widgets/ProgressInfoWidget.h \
  src/Widgets/InOutPanel.h \
  src/Widgets/ZoomLevelSelector.h \
  src/Widgets/SearchFieldWidget.h \
  src/Widgets/LanguageSelectionWidget.h \
  src/Widgets/ProgressInfoWindow.h


HEADERS += $$GMIC_PATH/gmic.h
HEADERS += $$GMIC_PATH/CImg.h
HEADERS += $$GMIC_PATH/gmic_stdlib_community.h

SOURCES += \
  src/ClickableLabel.cpp \
  src/Common.cpp \
  src/FilterParameters/CustomSpinBox.cpp \
  src/GmicQt.cpp \
  src/OverrideCursor.cpp \
  src/DialogSettings.cpp \
  src/FilterParameters/AbstractParameter.cpp \
  src/FilterParameters/BoolParameter.cpp \
  src/FilterParameters/ButtonParameter.cpp \
  src/FilterParameters/ChoiceParameter.cpp \
  src/FilterParameters/ColorParameter.cpp \
  src/FilterParameters/ConstParameter.cpp \
  src/FilterParameters/CustomDoubleSpinBox.cpp \
  src/FilterParameters/FileParameter.cpp \
  src/FilterParameters/FilterParametersWidget.cpp \
  src/FilterParameters/FloatParameter.cpp \
  src/FilterParameters/FolderParameter.cpp \
  src/FilterParameters/IntParameter.cpp \
  src/FilterParameters/LinkParameter.cpp \
  src/FilterParameters/MultilineTextParameterWidget.cpp \
  src/FilterParameters/NoteParameter.cpp \
  src/FilterParameters/PointParameter.cpp \
  src/FilterParameters/SeparatorParameter.cpp \
  src/FilterParameters/TextParameter.cpp \
  src/FilterSelector/FiltersModel.cpp \
  src/FilterSelector/FiltersModelReader.cpp \
  src/FilterSelector/FiltersPresenter.cpp \
  src/FilterSelector/FiltersView/FiltersView.cpp \
  src/FilterSelector/FiltersView/TreeView.cpp \
  src/FilterSelector/FiltersVisibilityMap.cpp \
  src/FilterSelector/FilterTagMap.cpp \
  src/CroppedImageListProxy.cpp \
  src/CroppedActiveLayerProxy.cpp \
  src/FilterSyncRunner.cpp \
  src/FilterThread.cpp \
  src/FilterTextTranslator.cpp \
  src/Globals.cpp \
  src/GmicStdlib.cpp \
  src/GmicProcessor.cpp \
  src/HeadlessProcessor.cpp \
  src/HtmlTranslator.cpp \
  src/IconLoader.cpp \
  src/ImageTools.cpp \
  src/InputOutputState.cpp \
  src/KeypointList.cpp \
  src/LayersExtentProxy.cpp \
  src/LanguageSettings.cpp \
  src/Logger.cpp \
  src/MainWindow.cpp \
  src/ParametersCache.cpp \
  src/PersistentMemory.cpp \
  src/Settings.cpp \
  src/Tags.cpp \
  src/TimeLogger.cpp \
  src/Updater.cpp \
  src/Utils.cpp \
  src/Misc.cpp \
  src/FilterSelector/FiltersView/FilterTreeItem.cpp \
  src/FilterSelector/FiltersView/FilterTreeFolder.cpp \
  src/FilterSelector/FavesModel.cpp \
  src/FilterSelector/FavesModelReader.cpp \
  src/FilterSelector/FiltersView/FilterTreeAbstractItem.cpp \
  src/FilterSelector/FiltersView/FilterTreeItemDelegate.cpp \
  src/FilterSelector/FavesModelWriter.cpp \
  src/Widgets/PreviewWidget.cpp \
  src/Widgets/ProgressInfoWidget.cpp \
  src/Widgets/InOutPanel.cpp \
  src/Widgets/VisibleTagSelector.cpp \
  src/Widgets/ZoomLevelSelector.cpp \
  src/Widgets/SearchFieldWidget.cpp \
  src/Widgets/LanguageSelectionWidget.cpp \
  src/Widgets/ProgressInfoWindow.cpp

equals(GMIC_DYNAMIC_LINKING, "on" ) {
  message(Dynamic linking with libgmic)
  LIBS += -Wl,-rpath,. $$GMIC_PATH/libgmic.so
}

equals(GMIC_DYNAMIC_LINKING, "off" ) {
   SOURCES += $$GMIC_PATH/gmic.cpp
   DEFINES += gmic_core
}

# ALL_FORMS
FORMS +=  ui/inoutpanel.ui \
          ui/multilinetextparameterwidget.ui \
          ui/progressinfowindow.ui \
          ui/dialogsettings.ui \
          ui/progressinfowidget.ui \
          ui/mainwindow.ui \
          ui/SearchFieldWidget.ui \
          ui/headlessprogressdialog.ui \
          ui/zoomlevelselector.ui \
          ui/languageselectionwidget.ui \
          ui/filtersview.ui

RESOURCES += gmic_qt.qrc translations.qrc
equals(HOST, "none") {
 RESOURCES += standalone.qrc
}

TRANSLATIONS = \
translations/cs.ts \
translations/de.ts \
translations/es.ts \
translations/fr.ts \
translations/id.ts \
translations/it.ts \
translations/ja.ts \
translations/nl.ts \
translations/pl.ts \
translations/pt.ts \
translations/ru.ts \
translations/sv.ts \
translations/uk.ts \
translations/zh.ts \
translations/zh_tw.ts

RESOURCES += wip_translations.qrc

# message(Build QM translation files)
# system(make -C translations)
# system(make -C translations/filters)

qm_files.commands += make -C translations
qm_filter_files.commands += make -C translations/filters
QMAKE_EXTRA_TARGETS += qm_files qm_filter_files
PRE_TARGETDEPS += qm_files qm_filter_files

QMAKE_DISTCLEAN = \
  translations/*.qm \
  translations/filters/*.ts \
  translations/filters/*.qm

# Prevent overwriting of these files by lupdate
# TRANSLATIONS += translations/filters/fr.ts

# PRE_TARGETDEPS +=

QMAKE_CXXFLAGS_RELEASE += -Ofast # -O3 -s
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
#    QMAKE_CXXFLAGS_DEBUG += -Wfatal-errors
}

equals(ASAN, "on" ) {
    message(Address sanitizer enabled)
    QMAKE_CXXFLAGS_DEBUG += -fsanitize=address
    QMAKE_LFLAGS_DEBUG += -fsanitize=address
}

UI_DIR = .ui
MOC_DIR = .moc
RCC_DIR = .qrc
OBJECTS_DIR = .obj

unix:!macx { DEFINES += _IS_UNIX_ }
macx {  DEFINES += _IS_MACOS_ }

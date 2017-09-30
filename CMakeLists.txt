project(gmic-qt)

message(STATUS "Using CMake version: ${CMAKE_VERSION}")

cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
LIST (APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
include(FeatureSummary)

set(CMAKE_CXX_STANDARD 11)
set(MIN_QT_VERSION 5.2.0)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set (GMIC_QT_HOST "gimp" CACHE STRING "Define for which host qmic-qt will be built: gimp, krita or none.")
message("Building version:" ${GMIC_QT_HOST})

set (GMIC_PATH "../gmic/src" CACHE STRING "Define the path to the gmic headers")
message("G'Mic path: " ${GMIC_PATH})

option(PRERELEASE "Set to ON makes this a prelease build")
if (${PRERELEASE})
    string(TIMESTAMP PRERELEASE_DATE %y%m%d)
    message("Prelease date is " ${PRERELEASE_DATE})
    add_definitions(-Dgmic_prelease=${PRERELEASE_DATE})
endif()

option(DRMINGW "Set to ON enables the drmingw debugger.")
if (${DRMINGW})
    add_definitions(-DDRMINGW)
endif()



# Required packages

#
# Qt5
#
find_package(Qt5 ${MIN_QT_VERSION}
        REQUIRED COMPONENTS
        Core
        Gui
        Widgets
        Network
)

#
# For the translations
#
find_package(Qt5LinguistTools REQUIRED)

#
# PNG
#
find_package(PNG REQUIRED)
add_definitions(${PNG_DEFINITIONS})
add_definitions(-Dcimg_use_png)
include_directories(SYSTEM ${PNG_INCLUDE_DIR})
if (APPLE)
    # this is not added correctly on OSX -- see http://forum.kde.org/viewtopic.php?f=139&t=101867&p=221242#p221242
    include_directories(SYSTEM ${PNG_INCLUDE_DIR})
endif()

#
# ZLIB
#
find_package(ZLIB REQUIRED)
add_definitions(-Dcimg_use_zlib)
include_directories(SYSTEM ${ZLIB_INCLUDE_DIRS} )

#
# FFTW3
#
find_package(FFTW3 REQUIRED)
add_definitions(-Dcimg_use_fftw3 )
add_definitions(-Dcimg_use_fftw3_singlethread )
include_directories(${FFTW3_INCLUDE_DIR})

#
# CURL
#
find_package(CURL)
if (CURL_FOUND)
    add_definitions(-Dcimg_use_curl)
    include_directories(SYSTEM ${CURL_INCLUDE_DIRS} )
endif()

#
# Test for OpenMP
#
find_package(OpenMP)
set_package_properties(OpenMP PROPERTIES
    DESCRIPTION "A low-level parallel execution library"
    URL "http://openmp.org/wp/"
    TYPE OPTIONAL
    PURPOSE "Optionally used by gmic-qt")

if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.8.3 AND OPENMP_FOUND)
    message("G'Mic: using OpenMP")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    add_definitions(-Dcimg_use_openmp)
    add_definitions(-fopenmp)
endif()


#
# add all defines
#

set(gmic_qt_LIBRARIES
        Qt5::Core
        Qt5::Widgets
        Qt5::Gui
        Qt5::Network
        ${PNG_LIBRARIES}
        ${FFTW3_LIBRARIES}
        ${ZLIB_LIBRARIES}
)

if (CURL_FOUND)
    set(gmic_qt_LIBRARIES
        ${gmic_qt_LIBRARIES}
        ${CURL_LIBRARIES}
    )
endif()

add_definitions(-Dgmic_build)
add_definitions(-Dcimg_use_abort)
add_definitions(-Dgmic_is_parallel)
add_definitions(-Dcimg_use_rng)
add_definitions(-Dgmic_gui)
add_definitions(-Dcimg_use_abort)
add_definitions(-Dcimg_appname=\"gmic\")

if (UNIX AND NOT APPLE)
    add_definitions(-Dcimg_display=1)
    add_definitions(-D_IS_LINUX_)
    add_definitions(-Dcimg_use_vt100)
    add_definitions(-D_IS_UNIX_)
    find_package(X11)
    set(gmic_qt_LIBRARIES
        ${gmic_qt_LIBRARIES}
        ${X11_LIBRARIES} # XXX: Search for X11: Wayland is coming!
    )

endif()

if (APPLE)
    add_definitions(-Dcimg_display=0)
    add_definitions(-D_IS_MACOS_)
    set(CMAKE_MACOSX_RPATH 1)
    set(BUILD_WITH_INSTALL_RPATH 1)
    add_definitions(-mmacosx-version-min=10.9 -Wno-macro-redefined -Wno-deprecated-register)
endif()

if (WIN32)
    add_definitions(-Dcimg_display=2)
    add_definitions(-DPSAPI_VERSION=1)
    set(gmic_qt_LIBRARIES
        ${gmic_qt_LIBRARIES}
        pthread psapi gdi32
    )
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("Debug build")
    add_definitions(-D_GMIC_QT_DEBUG_)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -fsanitize=address")
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    message("Release build")
    add_definitions(-DQT_NO_DEBUG_OUTPUT)
    string(REPLACE "-O2" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "-O3" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message("Release build with debug info")
    add_definitions(-DQT_NO_DEBUG_OUTPUT)
    string(REPLACE "-O2" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "-O3" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    set_source_files_properties(${GMIC_PATH}/gmic.cpp PROPERTIES COMPILE_FLAGS "-O3")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2")
else()
    message(FATAL_ERROR "Build type not recognized (${CMAKE_BUILD_TYPE})")
endif()

include_directories(${CMAKE_SOURCE_DIR}/include ${GMIC_PATH})

set (gmic_qt_SRCS

    include/ProgressInfoWidget.h
    include/FilterThread.h
    include/MultilineTextParameterWidget.h
    include/MainWindow.h
    include/ProgressInfoWindow.h
    include/BoolParameter.h
    include/FiltersTreeFilterItem.h
    include/ConstParameter.h
    include/FiltersTreeAbstractFilterItem.h
    include/LinkParameter.h
    include/Common.h
    include/PreviewWidget.h
    include/ButtonParameter.h
    include/ChoiceParameter.h
    include/IntParameter.h
    include/SearchFieldWidget.h
    include/FolderParameter.h
    include/ImageTools.h
    include/SeparatorParameter.h
    include/GmicStdlibParser.h
    include/gmic_qt.h
    include/FiltersTreeItemDelegate.h
    include/NoteParameter.h
    include/DialogSettings.h
    include/TextParameter.h
    include/host.h
    include/ParametersCache.h
    include/FiltersTreeAbstractItem.h
    include/AbstractParameter.h
    include/FloatParameter.h
    include/ImageConverter.h
    include/ColorParameter.h
    include/FiltersTreeFaveItem.h
    include/Updater.h
    include/FiltersTreeFolderItem.h
    include/FilterParamsWidget.h
    include/InOutPanel.h
    include/ClickableLabel.h
    include/FileParameter.h
    include/HeadlessProcessor.h
    include/FiltersVisibilityMap.h
    include/HtmlTranslator.h
    include/StoredFave.h
    include/ZoomLevelSelector.h
    include/LanguageSelectionWidget.h
    ${GMIC_PATH}/gmic.h

    src/FolderParameter.cpp
    src/ParametersCache.cpp
    src/gmic_qt.cpp
    src/TextParameter.cpp
    src/ColorParameter.cpp
    src/FilterParamsWidget.cpp
    src/FiltersTreeFaveItem.cpp
    src/FiltersTreeAbstractItem.cpp
    src/FileParameter.cpp
    src/GmicStdlibParser.cpp
    src/ImageTools.cpp
    src/FiltersTreeFolderItem.cpp
    src/ProgressInfoWindow.cpp
    src/IntParameter.cpp
    src/LayersExtentProxy.cpp
    src/FiltersTreeItemDelegate.cpp
    src/FilterThread.cpp
    src/SeparatorParameter.cpp
    src/NoteParameter.cpp
    src/MainWindow.cpp
    src/ConstParameter.cpp
    src/ImageConverter.cpp
    src/BoolParameter.cpp
    src/DialogSettings.cpp
    src/ButtonParameter.cpp
    src/FloatParameter.cpp
    src/ProgressInfoWidget.cpp
    src/AbstractParameter.cpp
    src/PreviewWidget.cpp
    src/ClickableLabel.cpp
    src/FiltersTreeAbstractFilterItem.cpp
    src/InOutPanel.cpp
    src/LinkParameter.cpp
    src/ChoiceParameter.cpp
    src/FiltersTreeFilterItem.cpp
    src/MultilineTextParameterWidget.cpp
    src/SearchFieldWidget.cpp
    src/Updater.cpp
    src/HeadlessProcessor.cpp
    src/FiltersVisibilityMap.cpp
    src/HtmlTranslator.cpp
    src/StoredFave.cpp
    src/ZoomLevelSelector.cpp
    src/LanguageSelectionWidget.cpp
    ${GMIC_PATH}/gmic.cpp
)

qt5_wrap_ui(gmic_qt_SRCS
    ui/inoutpanel.ui
    ui/multilinetextparameterwidget.ui
    ui/progressinfowindow.ui
    ui/dialogsettings.ui
    ui/progressinfowidget.ui
    ui/mainwindow.ui
    ui/SearchFieldWidget.ui
    ui/headlessprogressdialog.ui
    ui/zoomlevelselector.ui
    ui/languageselectionwidget.ui
)

qt5_create_translation(
    qmic_qt_QM

    ${CMAKE_SOURCE_DIR}/translations

    ${gmic_qt_SRCS}

    translations/cs.ts
    translations/de.ts
    translations/es.ts
    translations/fr.ts
    translations/id.ts
    translations/it.ts
    translations/nl.ts
    translations/pl.ts
    translations/pt.ts
    translations/ru.ts
    translations/ua.ts
    translations/ja.ts
    translations/zh.ts
)

install(FILES ${gmic_qt_QM} DESTINATION ${CMAKE_SOURCE_DIR/translations})

set(gmic_qt_QRC
    gmic_qt.qrc
    translations.qrc
)

if (${GMIC_QT_HOST} STREQUAL "gimp")

    find_package(Gimp2 REQUIRED)
    find_package(Cairo REQUIRED)
    find_package(GTK2 REQUIRED)
    include_directories(${GIMP2_INCLUDE_DIRS} ${CAIRO_INCLUDE_DIRS} ${GTK2_INCLUDE_DIRS})

    set (gmic_qt_SRCS ${gmic_qt_SRCS} src/host_gimp.cpp)
    add_definitions(-DGMIC_HOST=gimp_qt -DGIMP_DISABLE_DEPRECATED)
    add_executable(gmic_gimp_qt ${gmic_qt_SRCS} ${gmic_qt_QRC} ${qmic_qt_QM})
    target_link_libraries(
        gmic_gimp_qt
    PRIVATE
        ${GIMP2_LIBRARIES}
        ${gmic_qt_LIBRARIES}
    )

elseif (${GMIC_QT_HOST} STREQUAL "krita")

    set (gmic_qt_SRCS ${gmic_qt_SRCS} src/host_krita.cpp)
    add_definitions(-DGMIC_HOST=krita)
    add_executable(gmic_krita_qt ${gmic_qt_SRCS} ${gmic_qt_QRC} ${qmic_qt_QM})
    target_link_libraries(
        gmic_krita_qt
    PRIVATE
        ${gmic_qt_LIBRARIES}
    )

elseif (${GMIC_QT_HOST} STREQUAL "none")

    set (gmic_qt_SRCS ${gmic_qt_SRCS} src/host_none.cpp include/standalone/ImageDialog.h src/standalone/ImageDialog.cpp)
    add_definitions(-DGMIC_HOST=stantalone)
    add_executable(gmic_qt ${gmic_qt_SRCS} ${gmic_qt_QRC}  ${qmic_qt_QM})
        target_link_libraries(
        gmic_qt
    PRIVATE
        ${gmic_qt_LIBRARIES}
    )

else()
    message(FATAL_ERROR "GMIC_QT_HOST is not defined as gimp, krita or none")
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

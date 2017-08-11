# - Try to find the libgimp-2.0 Library
# Once done this will define
#
#  GIMP2_FOUND - system has gimp
#  GIMP2_INCLUDE_DIRS - the gimp include directories
#  GIMP2_LIBRARIES - the libraries needed to use gimp
#
# Redistribution and use is allowed according to the terms of the BSD license.

if (NOT WIN32)
    include(LibFindMacros)
    libfind_pkg_check_modules(GIMP2_PKGCONF gimp-2.0)

    find_path(GIMP2_INCLUDE_DIR
        NAMES libgimp/gimp.h
        HINTS ${GIMP2_PKGCONF_INCLUDE_DIRS} ${GIMP2_PKGCONF_INCLUDEDIR}
    )

    # we want the toplevel include directory
    string(REPLACE "libgimp" "" GIMP2_INCLUDE_DIR_STRIPPED ${GIMP2_INCLUDE_DIR})
    
    find_library(GIMP2_LIBRARY
        NAMES gimp-2.0
        HINTS ${GIMP2_PKGCONF_LIBRARY_DIRS} ${GIMP2_PKGCONF_LIBDIR}
    )

    set(GIMP2_PROCESS_LIBS GIMP2_LIBRARY)
    set(GIMP2_PROCESS_INCLUDES GIMP2_INCLUDE_DIR_STRIPPED)
    libfind_process(GIMP2)

    if(GIMP2_FOUND)
        message(STATUS "GIMP Found Version: " ${GIMP2_VERSION})
    endif()

else()

    find_path(GIMP2_INCLUDE_DIR
        NAMES gimp.h
    )


    find_library(
        GIMP2_LIBRARY
        NAMES libgimp-2.0
        DOC "Libraries to link against for Gimp Support")

    if (GIMP2_LIBRARY)
        set(GIMP2_LIBRARY_DIR ${GIMP2_LIBRARY})
    endif()

    set (GIMP2_LIBRARIES ${GIMP2_LIBRARY})

    if(GIMP2_INCLUDE_DIR AND GIMP2_LIBRARY_DIR)
        set (GIMP2_FOUND true)
        message(STATUS "Correctly found GIMP2")
    else()
        message(STATUS "Could not find GIMP2")
    endif()
endif()

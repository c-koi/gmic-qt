/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file Common.h
 *
 *  Copyright 2017 Sebastien Fourey
 *
 *  This file is part of G'MIC-Qt, a generic plug-in for raster graphics
 *  editors, offering hundreds of filters thanks to the underlying G'MIC
 *  image processing framework.
 *
 *  gmic_qt is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  gmic_qt is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with gmic_qt.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef _GMIC_QT_COMMON_H_
#define _GMIC_QT_COMMON_H_

#include <iostream>
#include "gmic_qt.h"

#define GMIC_QT_STRINGIFY(X) #X
#define GMIC_QT_XSTRINGIFY(X) GMIC_QT_STRINGIFY(X)

#ifdef _GMIC_QT_DEBUG_
#define ENTERING qWarning() << "[" << __PRETTY_FUNCTION__ << "] <<Entering>>"
#define TRACE qWarning() << "[" << __PRETTY_FUNCTION__ << "]"
#define TSHOW(V) qWarning() << "[" << __PRETTY_FUNCTION__ << "]" << #V << "=" << (V)
#define SHOW(V) qWarning() << #V << "=" << (V)
#else
#define ENTERING while (false)
#define TRACE                                                                                                                                                                                          \
  while (false)                                                                                                                                                                                        \
  qWarning() << ""
#define TSHOW(V)                                                                                                                                                                                       \
  while (false)                                                                                                                                                                                        \
  qWarning() << ""
#define SHOW(V)                                                                                                                                                                                        \
  while (false)                                                                                                                                                                                        \
  qWarning() << ""
#endif

#ifndef gmic_pixel_type
#define gmic_pixel_type float
#endif

#define gmic_pixel_type_str GMIC_QT_XSTRINGIFY(gmic_pixel_type)

template <typename T> inline void unused(const T &, ...)
{
}

#define SLIDER_MIN_WIDTH 60
#define PARAMETERS_CACHE_FILENAME "gmic_qt_params.dat"
#define FILTERS_VISIBILITY_FILENAME "gmic_qt_visibility.dat"

#define FAVE_FOLDER_TEXT "<b>Faves</b>"
#define FAVES_IMPORT_KEY "Faves/ImportedGTK179"

#define REFRESH_USING_INTERNET_KEY "Config/RefreshInternetUpdate"
#define INTERNET_UPDATE_PERIODICITY_KEY "Config/UpdatesPeriodicityValue"
#define INTERNET_NEVER_UPDATE_PERIODICITY std::numeric_limits<int>::max()

#define PREVIEW_MAX_ZOOM_FACTOR 40.0

//#define LOAD_ICON( NAME ) ( GmicQt::DarkThemeEnabled ? QIcon(":/icons/dark/" NAME ".png") : QIcon::fromTheme( NAME , QIcon(":/icons/" NAME ".png") ) )
#define LOAD_ICON(NAME) (DialogSettings::darkThemeEnabled() ? QIcon(":/icons/dark/" NAME ".png") : QIcon(":/icons/" NAME ".png"))

#endif // _GMIC_QT_COMMON_H

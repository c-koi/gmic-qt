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
#include "TimeLogger.h"

#ifdef _GMIC_QT_DEBUG_
#define ENTERING qWarning() << "[" << __PRETTY_FUNCTION__ << "] <<Entering>>"
#define LEAVING qWarning() << "[" << __PRETTY_FUNCTION__ << "] <<Leaving>>"
#define TRACE qWarning() << "[" << __PRETTY_FUNCTION__ << "]"
#define TSHOW(V) qWarning() << "[" << __PRETTY_FUNCTION__ << "]" << #V << "=" << (V)
#define SHOW(V) qWarning() << #V << "=" << (V)
#else
#define ENTERING while (false)
#define LEAVING while (false)
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

template <typename T> inline void unused(const T &, ...) {}

#ifdef _TIMING_ENABLED_
#define TIMING TimeLogger::getInstance()->step(__PRETTY_FUNCTION__, __LINE__, __FILE__);
#else
#define TIMING                                                                                                                                                                                         \
  if (false)                                                                                                                                                                                           \
  std::cout << ""
#define TIMING_CLOSE                                                                                                                                                                                   \
  if (false)                                                                                                                                                                                           \
  std::cout << ""
#endif

#endif // _GMIC_QT_COMMON_H

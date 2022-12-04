/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file ImageTools.h
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
#ifndef GMIC_QT_IMAGETOOLS_H
#define GMIC_QT_IMAGETOOLS_H

#include "Common.h"
#include "GmicQt.h"

namespace gmic_library
{
template <typename T> struct gmic_image;
template <typename T> struct gmic_list;
} // namespace gmic_library

namespace GmicQt
{

bool checkImageSpectrumAtMost4(const gmic_library::gmic_list<float> & images, unsigned int & index);
void buildPreviewImage(const gmic_library::gmic_list<float> & images, gmic_library::gmic_image<float> & result);

template <typename T> bool hasAlphaChannel(const gmic_library::gmic_image<T> & image);

} // namespace GmicQt

#endif // GMIC_QT_IMAGETOOLS_H

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
#include "gmic_qt.h"

namespace cimg_library
{
template <typename T> struct CImg;
template <typename T> struct CImgList;
} // namespace cimg_library

namespace GmicQt
{
template <typename T> void image2uchar(cimg_library::CImg<T> & img);
template <typename T> void calibrate_image(cimg_library::CImg<T> & img, const int spectrum, const bool is_preview);
bool checkImageSpectrumAtMost4(const cimg_library::CImgList<float> & images, unsigned int & index);
void buildPreviewImage(const cimg_library::CImgList<float> & images, cimg_library::CImg<float> & result);

} // namespace GmicQt

template <typename T> bool hasAlphaChannel(const cimg_library::CImg<T> & image);

#endif // GMIC_QT_IMAGETOOLS_H

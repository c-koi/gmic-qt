/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file CroppedActiveLayerProxy.h
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
#ifndef GMIC_QT_CROPPEDACTIVELAYERPROXY_H
#define GMIC_QT_CROPPEDACTIVELAYERPROXY_H

#include <QSize>
#include <memory>
#include "GmicQt.h"

namespace cimg_library
{
template <typename T> struct CImg;
template <typename T> struct CImgList;
} // namespace cimg_library

namespace GmicQt
{
class CroppedActiveLayerProxy {
public:
  CroppedActiveLayerProxy() = delete;

  static void get(cimg_library::CImg<gmic_pixel_type> & image, double x, double y, double width, double height);
  static QSize getSize(double x, double y, double width, double height);
  static void clear();

private:
  static void update(double x, double y, double width, double height);
  static std::unique_ptr<cimg_library::CImg<float>> _cachedImage;
  static double _x;
  static double _y;
  static double _width;
  static double _height;
};

} // namespace GmicQt

#endif // GMIC_QT_CROPPEDACTIVELAYERPROXY_H

/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file CroppedImageListProxy.h
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
#ifndef GMIC_QT_CROPPEDIMAGELISTPROXY_H
#define GMIC_QT_CROPPEDIMAGELISTPROXY_H

#include <memory>
#include "GmicQt.h"

namespace gmic_library
{
template <typename T> struct gmic_image;
template <typename T> struct gmic_list;
} // namespace gmic_library

namespace GmicQt
{

class CroppedImageListProxy {
public:
  CroppedImageListProxy() = delete;

  static void get(gmic_library::gmic_list<gmic_pixel_type> & images, gmic_library::gmic_list<char> & imageNames, double x, double y, double width, double height, InputMode mode, double zoom);
  static void update(double x, double y, double width, double height, InputMode mode, double zoom);
  static void clear();

private:
  static std::unique_ptr<gmic_library::gmic_list<float>> _cachedImageList;
  static std::unique_ptr<gmic_library::gmic_list<char>> _cachedImageNames;
  static double _x;
  static double _y;
  static double _width;
  static double _height;
  static InputMode _inputMode;
  static double _zoom;
};

} // namespace GmicQt

#endif // GMIC_QT_CROPPEDIMAGELISTPROXY_H

/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file CroppedImageListProxy.cpp
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

#include "CroppedImageListProxy.h"
#include <QDebug>
#include <cmath>
#include "Common.h"
#include "Host/host.h"
#include "gmic.h"

namespace GmicQt
{

double CroppedImageListProxy::_x = -1.0;
double CroppedImageListProxy::_y = -1.0;
double CroppedImageListProxy::_width = -1.0;
double CroppedImageListProxy::_height = -1.0;
double CroppedImageListProxy::_zoom = 0.0;
InputMode CroppedImageListProxy::_inputMode = InputMode::Unspecified;
std::unique_ptr<cimg_library::CImgList<gmic_pixel_type>> CroppedImageListProxy::_cachedImageList(new cimg_library::CImgList<gmic_pixel_type>);
std::unique_ptr<cimg_library::CImgList<char>> CroppedImageListProxy::_cachedImageNames(new cimg_library::CImgList<char>);

void CroppedImageListProxy::get(cimg_library::CImgList<gmic_pixel_type> & images, cimg_library::CImgList<char> & imageNames, double x, double y, double width, double height, InputMode mode,
                                double zoom)
{
  if ((x != _x) || (y != _y) || (width != _width) || (height != _height) || (mode != _inputMode) || (zoom != _zoom)) {
    update(x, y, width, height, mode, zoom);
  }
  images = *_cachedImageList;
  imageNames = *_cachedImageNames;
}

void CroppedImageListProxy::update(double x, double y, double width, double height, InputMode mode, double zoom)
{
  _x = x;
  _y = y;
  _width = width;
  _height = height;
  _inputMode = mode;
  _zoom = zoom;
  GmicQtHost::get_cropped_images(*_cachedImageList, *_cachedImageNames, _x, _y, _width, _height, _inputMode);
  if (zoom < 1.0) {
    for (unsigned int i = 0; i < _cachedImageList->size(); ++i) {
      gmic_image<float> & image = (*_cachedImageList)[i];
      image.resize(std::round(image.width() * zoom), std::round(image.height() * zoom), 1, -100, 1);
    }
  }
}

void CroppedImageListProxy::clear()
{
  _cachedImageList->assign();
  _cachedImageNames->assign();
  _x = _y = _width = _height = -1.0;
  _inputMode = InputMode::Unspecified;
  _zoom = 0.0;
}

} // namespace GmicQt

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
#include "Common.h"
#include "Host/host.h"
#include "gmic.h"

double CroppedImageListProxy::_x = -1.0;
double CroppedImageListProxy::_y = -1.0;
double CroppedImageListProxy::_width = -1.0;
double CroppedImageListProxy::_height = -1.0;
GmicQt::InputMode CroppedImageListProxy::_inputMode = GmicQt::UnspecifiedInputMode;
std::unique_ptr<cimg_library::CImgList<gmic_pixel_type>> CroppedImageListProxy::_cachedImageList(new cimg_library::CImgList<gmic_pixel_type>);
std::unique_ptr<cimg_library::CImgList<char>> CroppedImageListProxy::_cachedImageNames(new cimg_library::CImgList<char>);

void CroppedImageListProxy::get(cimg_library::CImgList<gmic_pixel_type> & images, cimg_library::CImgList<char> & imageNames, double x, double y, double width, double height, GmicQt::InputMode mode)
{
  if ((x != _x) || (y != _y) || (width != _width) || (height != _height) || (mode != _inputMode)) {
    _x = x;
    _y = y;
    _width = width;
    _height = height;
    _inputMode = mode;
    gmic_qt_get_cropped_images(*_cachedImageList, *_cachedImageNames, _x, _y, _width, _height, mode);
  }
  images = *_cachedImageList;
  imageNames = *_cachedImageNames;
}

void CroppedImageListProxy::clear()
{
  _cachedImageList->assign();
  _cachedImageNames->assign();
  _x = _y = _width = _height = -1.0;
  _inputMode = GmicQt::UnspecifiedInputMode;
}

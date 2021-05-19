/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file LayersExtentProxy.cpp
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
#include "LayersExtentProxy.h"
#include <QDebug>
#include "Common.h"
#include "Host/host.h"

namespace GmicQt
{

int LayersExtentProxy::_width = -1;
int LayersExtentProxy::_height = -1;
InputMode LayersExtentProxy::_inputMode = InputMode::All;

QSize LayersExtentProxy::getExtent(InputMode mode)
{
  QSize size;
  getExtent(mode, size.rwidth(), size.rheight());
  return size;
}

void LayersExtentProxy::getExtent(InputMode mode, int & width, int & height)
{
  if (mode != _inputMode || _width == -1 || _height == -1) {
    gmic_qt_get_layers_extent(&_width, &_height, mode);
    width = _width;
    height = _height;
  } else {
    width = _width;
    height = _height;
  }
  _inputMode = mode;
}

void LayersExtentProxy::clear()
{
  _width = _height = -1;
}

} // namespace GmicQt

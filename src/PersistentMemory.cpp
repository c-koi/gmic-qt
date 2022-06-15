/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file PersistentMemory.cpp
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
#include "PersistentMemory.h"
#include "gmic.h"

namespace GmicQt
{

cimg_library::CImg<char> * PersistentMemory::_image = nullptr;

cimg_library::CImg<char> & PersistentMemory::image()
{
  if (!_image) {
    _image = new cimg_library::CImg<char>;
  }
  return *_image;
}

void PersistentMemory::clear()
{
  image().assign();
}

void PersistentMemory::move_from(cimg_library::CImg<char> & list)
{
  list.move_to(image());
}

} // namespace GmicQt

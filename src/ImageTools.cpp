/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file ImageTools.cpp
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
#include "ImageTools.h"
#include <QDebug>
#include <QImage>
#include <QPainter>
#include "GmicStdlib.h"
#include "Gmic.h"

/*
 * Part of this code is much inspired by the original source code
 * of the GTK version of the gmic plug-in for GIMP by David Tschumperl\'e.
 */

namespace GmicQt
{

void buildPreviewImage(const gmic_library::gmic_list<float> & images, gmic_library::gmic_image<float> & result)
{
  gmic_library::gmic_list<gmic_pixel_type> preview_input_images;
  if (images.size() > 0) {
    preview_input_images.push_back(images[0]);
    int spectrum = 0;
    cimglist_for(preview_input_images, l) { spectrum = std::max(spectrum, preview_input_images[l].spectrum()); }
    spectrum += (spectrum == 1 || spectrum == 3);
    cimglist_for(preview_input_images, l) { calibrateImage(preview_input_images[l], spectrum, true); }
    result.swap(preview_input_images.front());
  } else {
    result.assign();
  }
}

bool checkImageSpectrumAtMost4(const gmic_library::gmic_list<float> & images, unsigned int & index)
{
  for (unsigned int i = 0; i < images.size(); ++i) {
    if (images[i].spectrum() > 4) {
      index = i;
      return false;
    }
  }
  return true;
}

template <typename T> bool hasAlphaChannel(const gmic_library::gmic_image<T> & image)
{
  return image.spectrum() == 2 || image.spectrum() == 4;
}

template bool hasAlphaChannel(const gmic_library::gmic_image<float> &);
template bool hasAlphaChannel(const gmic_library::gmic_image<unsigned char> &);

} // namespace GmicQt

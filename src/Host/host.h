/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file host.h
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
#ifndef GMIC_QT_HOST_H
#define GMIC_QT_HOST_H
#include <QString>
#include "gmic_qt.h"

namespace cimg_library
{
template <typename T> struct CImg;
template <typename T> struct CImgList;
} // namespace cimg_library

namespace GmicQt
{
extern const QString HostApplicationName;
extern const char * HostApplicationShortname;
extern const bool DarkThemeIsDefault;
} // namespace GmicQt

/**
 * @brief gmic_qt_get_image_size
 *
 * Already deprecated ! gmic_qt_get_layers_extends is the one actually used.
 *
 * @param[out] width
 * @param[out] height
 */
void gmic_qt_get_image_size(int * width, int * height);

/**
 * @brief Get the largest width and largest height among all the layers
 *        according to the input mode (\see gmic_qt.h).
 *
 * @param[out] width
 * @param[out] height
 */
void gmic_qt_get_layers_extent(int * width, int * height, GmicQt::InputMode);

/**
 * @brief Get a list of (cropped) image layers from host software.
 *
 *  Caution: returned images should contain "entire pixels" with respect to
 *  to the normalized coordinates. Hence, integer coordinates should be computed
 *  as (x,y,w,h) with :
 *   x = static_cast<int>(std::floor(x * input_image_width));
 *   w = std::min(input_image_width - x,static_cast<int>(1+std::ceil(width * input_image_width)));
 *
 * @param[out] images list
 * @param[out] imageNames Per layer description strings (position, opacity, etc.)
 * @param x Top-left corner normalized x coordinate w.r.t. image/extends width (i.e., in [0,1])
 * @param y Top-left corner normalized y coordinate w.r.t. image/extends width (i.e., in [0,1])
 * @param width Normalized width of the layers w.r.t. image/extends width
 * @param height Normalized height of the layers w.r.t. image/extends height
 * @param mode Input mode
 */
void gmic_qt_get_cropped_images(cimg_library::CImgList<gmic_pixel_type> & images, cimg_library::CImgList<char> & imageNames, double x, double y, double width, double height, GmicQt::InputMode mode);

/**
 * @brief Send a list of new image layers to the host application according to
 *        an output mode (\see gmic_qt.h)
 *
 * @param images List of layers to be sent to the host application. May be modified.
 * @param imageNames Layers labels
 * @param mode Output mode (\see gmic_qt.h)
 */
void gmic_qt_output_images(cimg_library::CImgList<gmic_pixel_type> & images, const cimg_library::CImgList<char> & imageNames, GmicQt::OutputMode mode);

/**
 * @brief Apply a color profile to a given image
 *
 * @param [in,out] images An image
 */
void gmic_qt_apply_color_profile(cimg_library::CImg<gmic_pixel_type> & images);

/**
 * @brief Display a message in the host application.
 *        This function is only used if the plugin is launched using
 *        launchPluginHeadless(). If a given plugin implementation never
 *        calls the latter function, show_message() can do nothing!
 *
 * @param message A message to be displayed by the host application
 */
void gmic_qt_show_message(const char * message);

#endif // GMIC_QT_HOST_H

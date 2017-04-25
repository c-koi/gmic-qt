/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file host_gimp.cpp
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
#include <libgimp/gimp.h>
#include <QDebug>
#include <QFileInfo>
#include <QRegExp>
#include <algorithm>
#include <limits>
#include "host.h"
#include "gmic_qt.h"
#include "ImageTools.h"
#include "Common.h"
#include "gmic.h"

/*
 * Part of this code is much inspired by the original source code
 * of the GTK version of the gmic plug-in for GIMP by David Tschumperl\'e.
 */

#define _gimp_image_get_item_position gimp_image_get_item_position

#if (GIMP_MAJOR_VERSION==2) && (GIMP_MINOR_VERSION<=7) && (GIMP_MICRO_VERSION<=14)
#define _gimp_item_get_visible gimp_drawable_get_visible
#else
#define _gimp_item_get_visible gimp_item_get_visible
#endif

namespace GmicQt {
const QString HostApplicationName = QString("GIMP %1.%2").arg(GIMP_MAJOR_VERSION).arg(GIMP_MINOR_VERSION);
const char * HostApplicationShortname = GMIC_QT_XSTRINGIFY(GMIC_HOST);
}

namespace {

int gmic_qt_gimp_image_id;
cimg_library::CImg<int> inputLayerDimensions;
std::vector<int> inputLayers;

#if (GIMP_MAJOR_VERSION>=3 || GIMP_MINOR_VERSION>8) && !defined(GIMP_NORMAL_MODE)
  typedef GimpLayerMode GimpLayerModeEffects;
#define GIMP_NORMAL_MODE        GIMP_LAYER_MODE_NORMAL
QMap<QString,GimpLayerModeEffects> BlendModesMap = {
  { QString("alpha"), GIMP_LAYER_MODE_NORMAL },
  { QString("normal"), GIMP_LAYER_MODE_NORMAL },
  { QString("replace"), GIMP_LAYER_MODE_REPLACE },
  { QString("dissolve"), GIMP_LAYER_MODE_DISSOLVE },
  { QString("behind"), GIMP_LAYER_MODE_BEHIND },
  { QString("colorerase"), GIMP_LAYER_MODE_COLOR_ERASE },
  { QString("erase"), GIMP_LAYER_MODE_ERASE },
  { QString("antierase"), GIMP_LAYER_MODE_ANTI_ERASE },
  { QString("merge"), GIMP_LAYER_MODE_MERGE },
  { QString("split"), GIMP_LAYER_MODE_SPLIT },
  { QString("lighten"), GIMP_LAYER_MODE_LIGHTEN_ONLY },
  { QString("lumalighten"), GIMP_LAYER_MODE_LUMA_LIGHTEN_ONLY },
  { QString("screen"), GIMP_LAYER_MODE_SCREEN },
  { QString("dodge"), GIMP_LAYER_MODE_DODGE },
  { QString("addition"), GIMP_LAYER_MODE_ADDITION },
  { QString("darken"), GIMP_LAYER_MODE_DARKEN_ONLY },
  { QString("lumadarken"), GIMP_LAYER_MODE_LUMA_DARKEN_ONLY },
  { QString("multiply"), GIMP_LAYER_MODE_MULTIPLY },
  { QString("burn"), GIMP_LAYER_MODE_BURN },
  { QString("overlay"), GIMP_LAYER_MODE_OVERLAY },
  { QString("softlight"), GIMP_LAYER_MODE_SOFTLIGHT },
  { QString("hardlight"), GIMP_LAYER_MODE_HARDLIGHT },
  { QString("vividlight"), GIMP_LAYER_MODE_VIVID_LIGHT },
  { QString("pinlight"), GIMP_LAYER_MODE_PIN_LIGHT },
  { QString("linearlight"), GIMP_LAYER_MODE_LINEAR_LIGHT },
  { QString("hardmix"), GIMP_LAYER_MODE_HARD_MIX },
  { QString("difference"), GIMP_LAYER_MODE_DIFFERENCE },
  { QString("subtract"), GIMP_LAYER_MODE_SUBTRACT },
  { QString("grainextract"), GIMP_LAYER_MODE_GRAIN_EXTRACT },
  { QString("grainmerge"), GIMP_LAYER_MODE_GRAIN_MERGE },
  { QString("divide"), GIMP_LAYER_MODE_DIVIDE },
  { QString("hue"), GIMP_LAYER_MODE_HSV_HUE },
  { QString("saturation"), GIMP_LAYER_MODE_HSV_SATURATION },
  { QString("color"), GIMP_LAYER_MODE_HSL_COLOR },
  { QString("value"), GIMP_LAYER_MODE_HSV_VALUE },
  { QString("lchhue"), GIMP_LAYER_MODE_LCH_HUE },
  { QString("lchchroma"), GIMP_LAYER_MODE_LCH_CHROMA },
  { QString("lchcolor"), GIMP_LAYER_MODE_LCH_COLOR },
  { QString("lchlightness"), GIMP_LAYER_MODE_LCH_LIGHTNESS },
  { QString("luminance"), GIMP_LAYER_MODE_LUMINANCE },
  { QString("exclusion"), GIMP_LAYER_MODE_EXCLUSION }
};
#else
QMap<QString,GimpLayerModeEffects> BlendModesMap = {
  { QString("alpha"), GIMP_NORMAL_MODE },
  { QString("normal"), GIMP_NORMAL_MODE },
  { QString("dissolve"), GIMP_DISSOLVE_MODE },
  { QString("lighten"), GIMP_LIGHTEN_ONLY_MODE },
  { QString("screen"), GIMP_SCREEN_MODE },
  { QString("dodge"), GIMP_DODGE_MODE },
  { QString("addition"), GIMP_ADDITION_MODE },
  { QString("darken"), GIMP_DARKEN_ONLY_MODE },
  { QString("multiply"), GIMP_MULTIPLY_MODE },
  { QString("burn"), GIMP_BURN_MODE },
  { QString("overlay"), GIMP_OVERLAY_MODE },
  { QString("softlight"), GIMP_SOFTLIGHT_MODE },
  { QString("hardlight"), GIMP_HARDLIGHT_MODE },
  { QString("difference"), GIMP_DIFFERENCE_MODE },
  { QString("subtract"), GIMP_SUBTRACT_MODE },
  { QString("grainextract"), GIMP_GRAIN_EXTRACT_MODE },
  { QString("grainmerge"), GIMP_GRAIN_MERGE_MODE },
  { QString("divide"), GIMP_DIVIDE_MODE },
  { QString("hue"), GIMP_HUE_MODE },
  { QString("saturation"), GIMP_SATURATION_MODE },
  { QString("color"), GIMP_COLOR_MODE },
  { QString("value"), GIMP_VALUE_MODE }
};
#endif

const char * BlendModeStrings(const GimpLayerModeEffects &blendmode) {
#if GIMP_MAJOR_VERSION<=2 && GIMP_MINOR_VERSION<=8
  switch (blendmode) {
  case GIMP_NORMAL_MODE : return "alpha";
  case GIMP_DISSOLVE_MODE : return "dissolve";
  case GIMP_BEHIND_MODE : return "behind";
  case GIMP_MULTIPLY_MODE : return "multiply";
  case GIMP_SCREEN_MODE : return "screen";
  case GIMP_OVERLAY_MODE : return "overlay";
  case GIMP_DIFFERENCE_MODE : return "difference";
  case GIMP_ADDITION_MODE : return "addition";
  case GIMP_SUBTRACT_MODE : return "subtract";
  case GIMP_DARKEN_ONLY_MODE : return "darken";
  case GIMP_LIGHTEN_ONLY_MODE : return "lighten";
  case GIMP_HUE_MODE : return "hue";
  case GIMP_SATURATION_MODE : return "saturation";
  case GIMP_COLOR_MODE : return "color";
  case GIMP_VALUE_MODE : return "value";
  case GIMP_DIVIDE_MODE : return "divide";
  case GIMP_DODGE_MODE : return "dodge";
  case GIMP_BURN_MODE : return "burn";
  case GIMP_HARDLIGHT_MODE : return "hardlight";
  case GIMP_SOFTLIGHT_MODE : return "softlight";
  case GIMP_GRAIN_EXTRACT_MODE : return "grainextract";
  case GIMP_GRAIN_MERGE_MODE : return "grainmerge";
  case GIMP_COLOR_ERASE_MODE : return "colorerase";
  default : return "alpha";
  }
#else
  switch (blendmode) {
  case GIMP_LAYER_MODE_NORMAL : return "alpha";
  case GIMP_LAYER_MODE_REPLACE : return "replace";
  case GIMP_LAYER_MODE_DISSOLVE : return "dissolve";
  case GIMP_LAYER_MODE_BEHIND : return "behind";
  case GIMP_LAYER_MODE_COLOR_ERASE : return "colorerase";
  case GIMP_LAYER_MODE_ERASE : return "erase";
  case GIMP_LAYER_MODE_ANTI_ERASE : return "antierase";
  case GIMP_LAYER_MODE_MERGE : return "merge";
  case GIMP_LAYER_MODE_SPLIT : return "split";
  case GIMP_LAYER_MODE_LIGHTEN_ONLY : return "lighten";
  case GIMP_LAYER_MODE_LUMA_LIGHTEN_ONLY : return "lumalighten";
  case GIMP_LAYER_MODE_SCREEN : return "screen";
  case GIMP_LAYER_MODE_DODGE : return "dodge";
  case GIMP_LAYER_MODE_ADDITION : return "addition";
  case GIMP_LAYER_MODE_DARKEN_ONLY : return "darken";
  case GIMP_LAYER_MODE_LUMA_DARKEN_ONLY : return "lumadarken";
  case GIMP_LAYER_MODE_MULTIPLY : return "multiply";
  case GIMP_LAYER_MODE_BURN : return "burn";
  case GIMP_LAYER_MODE_OVERLAY : return "overlay";
  case GIMP_LAYER_MODE_SOFTLIGHT : return "softlight";
  case GIMP_LAYER_MODE_HARDLIGHT : return "hardlight";
  case GIMP_LAYER_MODE_VIVID_LIGHT : return "vividlight";
  case GIMP_LAYER_MODE_PIN_LIGHT : return "pinlight";
  case GIMP_LAYER_MODE_LINEAR_LIGHT : return "linearlight";
  case GIMP_LAYER_MODE_HARD_MIX : return "hardmix";
  case GIMP_LAYER_MODE_DIFFERENCE : return "difference";
  case GIMP_LAYER_MODE_SUBTRACT : return "subtract";
  case GIMP_LAYER_MODE_GRAIN_EXTRACT : return "grainextract";
  case GIMP_LAYER_MODE_GRAIN_MERGE : return "grainmerge";
  case GIMP_LAYER_MODE_DIVIDE : return "divide";
  case GIMP_LAYER_MODE_HSV_HUE : return "hue";
  case GIMP_LAYER_MODE_HSV_SATURATION : return "saturation";
  case GIMP_LAYER_MODE_HSL_COLOR : return "color";
  case GIMP_LAYER_MODE_HSV_VALUE : return "value";
  case GIMP_LAYER_MODE_LCH_HUE : return "lchhue";
  case GIMP_LAYER_MODE_LCH_CHROMA : return "lchchroma";
  case GIMP_LAYER_MODE_LCH_COLOR : return "lchcolor";
  case GIMP_LAYER_MODE_LCH_LIGHTNESS : return "lchlightness";
  case GIMP_LAYER_MODE_LUMINANCE : return "luminance";
  case GIMP_LAYER_MODE_EXCLUSION : return "exclusion";
  default : return "alpha";
  }
#endif
  return "alpha";
}

// Get layer blending mode from string.
//-------------------------------------
void get_output_layer_props(const char * const s,
                            GimpLayerModeEffects & blendmode,
                            double &opacity,
                            int & posx, int & posy,
                            cimg_library::CImg<char> & name) {
  if (!s || !*s) return;
  QString str(s);

  // Read output blending mode.
  QRegExp modeRe("mode\\(\\s*([^)]*)\\s*\\)");
  if ( modeRe.indexIn(str) != -1 ) {
    QString modeStr = modeRe.cap(1).trimmed();
    if ( BlendModesMap.find(modeStr) != BlendModesMap.end() ) {
      blendmode = BlendModesMap[modeStr];
    }
  }

  // Read output opacity.
  QRegExp opacityRe("opacity\\(\\s*([^)]*)\\s*\\)");
  if ( opacityRe.indexIn(str) != -1 ) {
    QString opacityStr = opacityRe.cap(1).trimmed();
    bool ok = false;
    double x = opacityStr.toDouble(&ok);
    if ( ok ) {
      opacity = x;
      if (opacity < 0) {
        opacity = 0;
      } else if (opacity > 100) {
        opacity = 100;
      }
    }
  }

  // Read output positions.
  QRegExp posRe("pos\\(\\s*(\\d*)[^)](\\d*)\\s*\\)");
  if ( posRe.indexIn(str) != -1 ) {
    QString xStr = posRe.cap(1);
    QString yStr = posRe.cap(2);
    bool okX = false;
    bool okY= false;
    int x = xStr.toInt(&okX);
    int y = yStr.toInt(&okY);
    if ( okX && okY ) {
      posx = x;
      posy = y;
    }
  }

  // Read output name.
  const char * S = std::strstr(s,"name(");
  if (S) {
    const char *ps = S + 5;
    unsigned int level = 1;
    while (*ps && level) {
      if (*ps=='(') {
        ++level;
      } else if (*ps==')') {
        --level;
      }
      ++ps;
    }
    if (!level || *(ps - 1)==')') {
      name.assign(S + 5,(unsigned int)(ps - S - 5)).back() = 0;
      cimg_for(name,pn,char) {
        if (*pn==21) {
          *pn = '(';
        } else if (*pn==22) {
          *pn = ')';
        }
      }
    }
  }
}
}

void gmic_qt_show_message(const char * message)
{
  static bool first = true;

  if (first) {
    gimp_progress_init(message);
    first = false;
  } else {
    gimp_progress_set_text_printf("%s",message);
  }
}

void gmic_qt_apply_color_profile(cimg_library::CImg<float> & image)
{
#if (GIMP_MAJOR_VERSION<2) || ((GIMP_MAJOR_VERSION==2) && (GIMP_MINOR_VERSION<=8))
  unused(image);
  // SWAP RED<->GREEN CHANNELS : FOR TESTING PURPOSE ONLY!
  //  cimg_forXY(image,x,y) {
  //    std::swap(image(x,y,0,0),image(x,y,0,1));
  //  }
#else
  unused(image);
  //  GimpColorProfile * const img_profile = gimp_image_get_effective_color_profile(gmic_qt_gimp_image_id);
  //  GimpColorConfig * const color_config = gimp_get_color_configuration();
  //  if (!img_profile || !color_config) {
  //    return;
  //  }

  //    if (!image || image.spectrum() < 3 || image.spectrum() > 4 ) {
  //      continue;
  //    }
  //    const Babl *const fmt = babl_format(image.spectrum()==3?"R'G'B' float":"R'G'B'A float");
  //    GimpColorTransform *const transform = gimp_widget_get_color_transform(gui_preview,
  //                                                                          color_config,
  //                                                                          img_profile,
  //                                                                          fmt,
  //                                                                          fmt);
  //    if (!transform) {
  //      continue;
  //    }
  //    cimg_library::CImg<float> corrected;
  //    image.get_permute_axes("cxyz").move_to(corrected) /= 255;
  //    gimp_color_transform_process_pixels(transform,fmt,corrected,fmt,corrected,
  //                                        corrected.height()*corrected.depth());
  //    (corrected.permute_axes("yzcx")*=255).cut(0,255).move_to(image);
  //    g_object_unref(transform);
#endif
}

void gmic_qt_get_layers_extent(int * width, int * height, GmicQt::InputMode mode)
{
  int layersCount = 0;
  const int * begLayers = gimp_image_get_layers(gmic_qt_gimp_image_id, &layersCount);
  const int * endLayers = begLayers + layersCount;
  int activeLayerID = gimp_image_get_active_layer(gmic_qt_gimp_image_id);
  if (gimp_item_is_group(activeLayerID)) {
    return;
  }
  // Buil list of input layers IDs
  std::vector<int> layers;
  switch (mode) {
  case GmicQt::NoInput:
    break;
  case GmicQt::Active:
    if (activeLayerID >= 0) {
      layers.push_back(activeLayerID);
    }
    break;
  case GmicQt::All:
  case GmicQt::AllDesc:
    layers.assign(begLayers,endLayers);
    break;
  case GmicQt::ActiveAndBelow:
    if (activeLayerID >= 0) {
      layers.push_back(activeLayerID);
      const int * p = std::find(begLayers,endLayers,activeLayerID);
      if ( p < endLayers - 1) {
        layers.push_back(*(p+1));
      }
    }
    break;
  case GmicQt::ActiveAndAbove:
    if (activeLayerID >= 0) {
      layers.push_back(activeLayerID);
      const int * p = std::find(begLayers,endLayers,activeLayerID);
      if ( p > begLayers ) {
        layers.push_back(*(p-1));
      }
    }
    break;
  case GmicQt::AllVisibles:
  case GmicQt::AllVisiblesDesc:
  case GmicQt::AllInvisibles:
  case GmicQt::AllInvisiblesDesc:
  {
    bool visibility = (mode == GmicQt::AllVisibles || mode == GmicQt::AllVisiblesDesc );
    for ( int i = 0; i < layersCount; ++i ) {
      if ( _gimp_item_get_visible(begLayers[i]) == visibility ) {
        layers.push_back(begLayers[i]);
      }
    }
  }
    break;
  default:
    break;
  }

  gint rgn_x, rgn_y, rgn_width, rgn_height;
  *width = 0;
  *height = 0;
  for ( int layer : layers ) {
    if ( !gimp_item_is_valid(layer) ) {
      continue;
    }
    if ( ! gimp_drawable_mask_intersect(layer,&rgn_x,&rgn_y,&rgn_width,&rgn_height)) {
      continue;
    }
    *width = std::max(*width,rgn_width);
    *height = std::max(*height,rgn_height);
  }
}

void gmic_qt_get_image_size(int * width, int * height)
{
  *width = 0;
  *height = 0;
  int layersCount = 0;
  gimp_image_get_layers(gmic_qt_gimp_image_id, &layersCount);
  if ( layersCount > 0 ) {
    int active_layer_id = gimp_image_get_active_layer(gmic_qt_gimp_image_id);
    if (active_layer_id >= 0) {
      if ( gimp_item_is_valid(active_layer_id) ) {
        gint32 id= gimp_item_get_image(active_layer_id);
        *width = gimp_image_width(id);
        *height = gimp_image_height(id);
      }
    }
  }
}

void gmic_qt_get_cropped_images( gmic_list<float> & images,
                                 gmic_list<char> & imageNames,
                                 double x,
                                 double y,
                                 double width,
                                 double height,
                                 GmicQt::InputMode mode )
{
  using cimg_library::CImg;
  using cimg_library::CImgList;
  int layersCount = 0;
  const int * layers = gimp_image_get_layers(gmic_qt_gimp_image_id, &layersCount);
  const int * end_layers = layers + layersCount;
  int active_layer_id = gimp_image_get_active_layer(gmic_qt_gimp_image_id);

  if (gimp_item_is_group(active_layer_id)) {
    images.assign();
    imageNames.assign();
    return;
  }

  const bool entireImage = (x < 0 && y < 0 && width < 0 && height < 0) ||
      (x == 0.0 && y == 0 && width == 1 && height == 0);
  if (entireImage) {
    x = 0.0;
    y = 0.0;
    width = 1.0;
    height = 1.0;
  }

  // Buil list of input layers IDs
  inputLayers.clear();
  switch (mode) {
  case GmicQt::NoInput:
    break;
  case GmicQt::Active:
    if (active_layer_id >= 0) {
      inputLayers.push_back(active_layer_id);
    }
    break;
  case GmicQt::All:
  case GmicQt::AllDesc:
    inputLayers.assign(layers,end_layers);
    if ( mode == GmicQt::AllDesc ) {
      std::reverse(inputLayers.begin(),inputLayers.end());
    }
    break;
  case GmicQt::ActiveAndBelow:
    if (active_layer_id >= 0) {
      inputLayers.push_back(active_layer_id);
      const int * p = std::find(layers,end_layers,active_layer_id);
      if ( p < end_layers - 1) {
        inputLayers.push_back(*(p+1));
      }
    }
    break;
  case GmicQt::ActiveAndAbove:
    if (active_layer_id >= 0) {
      inputLayers.push_back(active_layer_id);
      const int * p = std::find(layers,end_layers,active_layer_id);
      if ( p > layers ) {
        inputLayers.push_back(*(p-1));
      }
    }
    break;
  case GmicQt::AllVisibles:
  case GmicQt::AllVisiblesDesc:
  case GmicQt::AllInvisibles:
  case GmicQt::AllInvisiblesDesc:
  {
    bool visibility = (mode == GmicQt::AllVisibles || mode == GmicQt::AllVisiblesDesc );
    for ( int i = 0; i < layersCount; ++i ) {
      if ( _gimp_item_get_visible(layers[i]) == visibility ) {
        inputLayers.push_back(layers[i]);
      }
    }
    if ( mode == GmicQt::AllVisiblesDesc || mode == GmicQt::AllInvisiblesDesc ) {
      std::reverse(inputLayers.begin(),inputLayers.end());
    }
  }
    break;
  default:
    break;
  }

  // Retrieve image list
  images.assign(inputLayers.size());
  imageNames.assign(inputLayers.size());
  inputLayerDimensions.assign(inputLayers.size(),4);
  gint rgn_x, rgn_y, rgn_width, rgn_height;

  gboolean isSelection  = 0;
  gint selX1=0, selY1=0, selX2=0, selY2=0;
  if ( ! gimp_selection_bounds(gmic_qt_gimp_image_id,
                               &isSelection,
                               &selX1,&selY1,&selX2,&selY2) ) {
    isSelection = 0;
    selX1 = 0;
    selY1 = 0;
  }

  cimglist_for(images,l) {
    if ( !gimp_item_is_valid(inputLayers[l]) ) {
      continue;
    }
    if ( ! gimp_drawable_mask_intersect(inputLayers[l],&rgn_x,&rgn_y,&rgn_width,&rgn_height)) {
      inputLayerDimensions(l,0) = 0;
      inputLayerDimensions(l,1) = 0;
      inputLayerDimensions(l,2) = 0;
      inputLayerDimensions(l,3) = 0;
      continue;
    }
    const int spectrum = (gimp_drawable_is_rgb(inputLayers[l])?3:1) +
        (gimp_drawable_has_alpha(inputLayers[l])?1:0);

    GimpDrawable *drawable = gimp_drawable_get(inputLayers[l]);
    const int ix = static_cast<int>(entireImage?rgn_x:(rgn_x+x*rgn_width));
    const int iy = static_cast<int>(entireImage?rgn_y:(rgn_y+y*rgn_height));
    const int iw = entireImage?rgn_width:std::min(static_cast<int>(drawable->width)-ix,static_cast<int>(1+std::ceil(rgn_width*width)));
    const int ih = entireImage?rgn_height:std::min(static_cast<int>(drawable->height)-iy,static_cast<int>(1+std::ceil(rgn_height*height)));

    if ( entireImage ) {
      inputLayerDimensions(l,0) = rgn_width;
      inputLayerDimensions(l,1) = rgn_height;
    } else {
      inputLayerDimensions(l,0) = iw;
      inputLayerDimensions(l,1) = ih;
    }
    inputLayerDimensions(l,2) = 1;
    inputLayerDimensions(l,3) = spectrum;

    const float opacity = gimp_layer_get_opacity(inputLayers[l]);
    const GimpLayerModeEffects blendMode = gimp_layer_get_mode(inputLayers[l]);
    int xPos = 0;
    int yPos = 0;
    if ( isSelection ) {
      xPos = selX1;
      yPos = selY1;
    } else {
      gimp_drawable_offsets(inputLayers[l],&xPos,&yPos);
    }

    QString name = QString("mode(%1),opacity(%2),pos(%3,%4),name(%5)")
        .arg(BlendModeStrings(blendMode))
        .arg(opacity)
        .arg(xPos)
        .arg(yPos)
        .arg(gimp_item_get_name(inputLayers[l]));
    QByteArray ba = name.toUtf8();
    gmic_image<char>::string(ba.constData()).move_to(imageNames[l]);
#if (GIMP_MAJOR_VERSION<2) || ((GIMP_MAJOR_VERSION==2) && (GIMP_MINOR_VERSION<=8))
    GimpPixelRgn region;
    gimp_pixel_rgn_init(&region,drawable,ix,iy,iw,ih,false,false);
    CImg<unsigned char> img(spectrum,iw,ih);
    gimp_pixel_rgn_get_rect(&region,img,ix,iy,iw,ih);
    gimp_drawable_detach(drawable);
    img.permute_axes("yzcx");
#else
    GeglRectangle rect;
    gegl_rectangle_set(&rect,ix,iy,iw,ih);
    GeglBuffer *buffer = gimp_drawable_get_buffer(inputLayers[l]);
    const char *const format = spectrum==1 ? "Y' " gmic_pixel_type_str :
                                             spectrum==2 ? "Y'A " gmic_pixel_type_str
                                                         : spectrum==3 ? "R'G'B' " gmic_pixel_type_str
                                                                       : "R'G'B'A " gmic_pixel_type_str;
    CImg<float> img(spectrum,iw,ih);
    gegl_buffer_get(buffer,&rect,1,babl_format(format),img.data(),0,GEGL_ABYSS_NONE);
    (img *= 255).permute_axes("yzcx");
    g_object_unref(buffer);
#endif
    img.move_to(images[l]);
  }
}

void gmic_qt_output_images( gmic_list<gmic_pixel_type> & images,
                            const gmic_list<char> & imageNames,
                            GmicQt::OutputMode outputMode,
                            const char * verboseLayersLabel )
{
  // Output modes in original gmic_gimp_gtk : 0/Replace 1/New layer 2/New active layer  3/New image

  // spectrum to GIMP image types conversion table
  GimpImageType spectrum2gimpImageTypes[5] = { GIMP_INDEXED_IMAGE, // (unused)
                                               GIMP_GRAY_IMAGE,
                                               GIMP_GRAYA_IMAGE,
                                               GIMP_RGB_IMAGE,
                                               GIMP_RGBA_IMAGE };

  // Get output layers dimensions and check if input/output layers have compatible dimensions.
  unsigned int max_width = 0, max_height = 0, max_spectrum = 0;
  cimglist_for(images,l) {
    if (images[l].is_empty()) {
      images.remove(l--);
      continue;
    } // Discard possible empty images.
    if (images[l]._width>max_width) {
      max_width = images[l]._width;
    }
    if (images[l]._height>max_height) {
      max_height = images[l]._height;
    }
    if (images[l]._spectrum>max_spectrum) {
      max_spectrum = images[l]._spectrum;
    }
  }

  int image_nb_layers = 0;
  gimp_image_get_layers(gmic_qt_gimp_image_id,&image_nb_layers);
  unsigned int image_width = 0, image_height = 0;
  if (inputLayers.size()) {
    image_width = gimp_image_width(gmic_qt_gimp_image_id);
    image_height = gimp_image_height(gmic_qt_gimp_image_id);
  }

  int is_selection = 0, sel_x0 = 0, sel_y0 = 0, sel_x1 = 0, sel_y1 = 0;
  if (!gimp_selection_bounds(gmic_qt_gimp_image_id,&is_selection,&sel_x0,&sel_y0,&sel_x1,&sel_y1)) {
    is_selection = 0;
  } else if (outputMode == GmicQt::InPlace || outputMode == GmicQt::NewImage ) {
    sel_x0 = sel_y0 = 0;
  }

  bool is_compatible_dimensions = (images.size()==inputLayers.size());
  for (unsigned int p = 0; p<images.size() && is_compatible_dimensions; ++p) {
    const cimg_library::CImg<gmic_pixel_type> & img = images[p];
    const bool source_is_alpha = (inputLayerDimensions(p,3)==2 || inputLayerDimensions(p,3)>=4);
    const bool dest_is_alpha = (img.spectrum()==2 || img.spectrum()>=4);
    if (dest_is_alpha && !source_is_alpha) {
      gimp_layer_add_alpha(inputLayers[p]);
      ++inputLayerDimensions(p,3);
    }
    if (img.width()!=inputLayerDimensions(p,0) || img.height()!=inputLayerDimensions(p,1)) {
      is_compatible_dimensions = false;
    }
  }

  // Transfer output layers back into GIMP.
  GimpLayerModeEffects layer_blendmode = GIMP_NORMAL_MODE;
  gint layer_posx = 0, layer_posy = 0;
  double layer_opacity = 100;
  cimg_library::CImg<char> layer_name;

  if ( outputMode == GmicQt::InPlace ) {
    gint rgn_x, rgn_y, rgn_width, rgn_height;
    gimp_image_undo_group_start(gmic_qt_gimp_image_id);
    if (is_compatible_dimensions) {                       // Direct replacement of the layer data.
      for (unsigned int p = 0; p < images.size(); ++p) {
        layer_blendmode = gimp_layer_get_mode(inputLayers[p]);
        layer_opacity = gimp_layer_get_opacity(inputLayers[p]);
        gimp_drawable_offsets(inputLayers[p],&layer_posx,&layer_posy);
        cimg_library::CImg<char>::string(gimp_item_get_name(inputLayers[p])).move_to(layer_name);
        get_output_layer_props(imageNames[p],layer_blendmode,layer_opacity,layer_posx,layer_posy,layer_name);
        if ( is_selection ) {
          layer_posx = 0;
          layer_posy = 0;
        }
        cimg_library::CImg<gmic_pixel_type> &img = images[p];
        GmicQt::calibrate_image(img,inputLayerDimensions(p,3),false);
        if (gimp_drawable_mask_intersect(inputLayers[p],&rgn_x,&rgn_y,&rgn_width,&rgn_height)) {
#if (GIMP_MAJOR_VERSION<2) || ((GIMP_MAJOR_VERSION==2) && (GIMP_MINOR_VERSION<=8))
          GimpDrawable *drawable = gimp_drawable_get(inputLayers[p]);
          GimpPixelRgn region;
          gimp_pixel_rgn_init(&region,drawable,rgn_x,rgn_y,rgn_width,rgn_height,true,true);
          GmicQt::image2uchar(img);
          gimp_pixel_rgn_set_rect(&region,(guchar*)img.data(),rgn_x,rgn_y,rgn_width,rgn_height);
          gimp_drawable_flush(drawable);
          gimp_drawable_merge_shadow(inputLayers[p],true);
          gimp_drawable_update(inputLayers[p],rgn_x,rgn_y,rgn_width,rgn_height);
          gimp_drawable_detach(drawable);
#else
          GeglRectangle rect;
          gegl_rectangle_set(&rect,rgn_x,rgn_y,rgn_width,rgn_height);
          GeglBuffer *buffer = gimp_drawable_get_shadow_buffer(inputLayers[p]);
          const char *const format = img.spectrum()==1?"Y' float":img.spectrum()==2?"Y'A float":
                                                                                    img.spectrum()==3?"R'G'B' float":"R'G'B'A float";
          (img/=255).permute_axes("cxyz");
          gegl_buffer_set(buffer,&rect,0,babl_format(format),img.data(),0);
          g_object_unref(buffer);
          gimp_drawable_merge_shadow(inputLayers[p],true);
          gimp_drawable_update(inputLayers[p],0,0,img.width(),img.height());
#endif
          gimp_layer_set_mode(inputLayers[p],layer_blendmode);
          gimp_layer_set_opacity(inputLayers[p],layer_opacity);
          gimp_layer_set_offsets(inputLayers[p],layer_posx,layer_posy);

          if (verboseLayersLabel) {   // Verbose (layer name)
            gimp_item_set_name(inputLayers[p],verboseLayersLabel);
          } else if (layer_name) {
            gimp_item_set_name(inputLayers[p],layer_name);
          }
        }
        img.assign();
      }
    } else { // Indirect replacement: create new layers.
      gimp_selection_none(gmic_qt_gimp_image_id);
      const int layer_pos = _gimp_image_get_item_position(gmic_qt_gimp_image_id,inputLayers[0]);
      max_width = max_height = 0;

      for (unsigned int p = 0; p < images.size(); ++p) {
        if (images[p]) {
          layer_posx = layer_posy = 0;
          if (p<inputLayers.size()) {
            layer_blendmode = gimp_layer_get_mode(inputLayers[p]);
            layer_opacity = gimp_layer_get_opacity(inputLayers[p]);
            if (!is_selection) {
              gimp_drawable_offsets(inputLayers[p],&layer_posx,&layer_posy);
            }
            cimg_library::CImg<char>::string(gimp_item_get_name(inputLayers[p])).move_to(layer_name);
            gimp_image_remove_layer(gmic_qt_gimp_image_id,inputLayers[p]);
          } else {
            layer_blendmode = GIMP_NORMAL_MODE;
            layer_opacity = 100;
            layer_name.assign();
          }
          get_output_layer_props(imageNames[p],
                                 layer_blendmode,
                                 layer_opacity,
                                 layer_posx,
                                 layer_posy,
                                 layer_name);
          if ( is_selection ) {
            layer_posx = 0;
            layer_posy = 0;
          }
          if (layer_posx + images[p]._width>max_width) {
            max_width = layer_posx + images[p]._width;
          }
          if (layer_posy + images[p]._height>max_height) {
            max_height = layer_posy + images[p]._height;
          }
          cimg_library::CImg<gmic_pixel_type> & img = images[p];
          if (gimp_image_base_type(gmic_qt_gimp_image_id)==GIMP_GRAY) {
            GmicQt::calibrate_image(img,(img.spectrum()==1 || img.spectrum()==3)?1:2,false);
          } else {
            GmicQt::calibrate_image(img,(img.spectrum()==1 || img.spectrum()==3)?3:4,false);
          }
          gint layer_id = gimp_layer_new(gmic_qt_gimp_image_id,verboseLayersLabel,img.width(),img.height(),
                                         spectrum2gimpImageTypes[std::min(img.spectrum(),4)],
              layer_opacity,layer_blendmode);
          gimp_layer_set_offsets(layer_id,layer_posx,layer_posy);
          if (verboseLayersLabel) {
            gimp_item_set_name(layer_id,verboseLayersLabel);
          } else if (layer_name) {
            gimp_item_set_name(layer_id,layer_name);
          }
          gimp_image_insert_layer(gmic_qt_gimp_image_id,layer_id,-1,layer_pos + p);

#if (GIMP_MAJOR_VERSION<2) || ((GIMP_MAJOR_VERSION==2) && (GIMP_MINOR_VERSION<=8))
          GimpDrawable *drawable = gimp_drawable_get(layer_id);
          GimpPixelRgn region;
          gimp_pixel_rgn_init(&region,drawable,0,0,drawable->width,drawable->height,true,true);
          GmicQt::image2uchar(img);
          gimp_pixel_rgn_set_rect(&region,(guchar*)img.data(),0,0,img.width(),img.height());
          gimp_drawable_flush(drawable);
          gimp_drawable_merge_shadow(layer_id,true);
          gimp_drawable_update(layer_id,0,0,drawable->width,drawable->height);
          gimp_drawable_detach(drawable);
#else
          GeglBuffer *buffer = gimp_drawable_get_shadow_buffer(layer_id);
          const char *const format = img.spectrum()==1?"Y' float":img.spectrum()==2?"Y'A float":
                                                                                    img.spectrum()==3?"R'G'B' float":"R'G'B'A float";
          (img/=255).permute_axes("cxyz");
          gegl_buffer_set(buffer,NULL,0,babl_format(format),img.data(),0);
          g_object_unref(buffer);
          gimp_drawable_merge_shadow(layer_id,true);
          gimp_drawable_update(layer_id,0,0,img.width(),img.height());
#endif
          img.assign();
        }
      }
      for (unsigned int p = images._width; p < inputLayers.size(); ++p) {
        gimp_image_remove_layer(gmic_qt_gimp_image_id,inputLayers[p]);
      }
      if ((unsigned int)image_nb_layers == inputLayers.size()) {
        gimp_image_resize(gmic_qt_gimp_image_id,max_width,max_height,0,0);
      } else {
        gimp_image_resize(gmic_qt_gimp_image_id,std::max(image_width,max_width),std::max(image_height,max_height),0,0);
      }
    }
    gimp_image_undo_group_end(gmic_qt_gimp_image_id);
  } else if ( outputMode == GmicQt::NewActiveLayers || outputMode == GmicQt::NewLayers ) {
    const gint active_layer_id = gimp_image_get_active_layer(gmic_qt_gimp_image_id);
    if (active_layer_id >= 0) {
      gimp_image_undo_group_start(gmic_qt_gimp_image_id);
      gint top_layer_id = 0, layer_id = 0;
      max_width = max_height = 0;
      for (unsigned int p = 0; p < images.size(); ++p) {
        if (images[p]) {
          layer_blendmode = GIMP_NORMAL_MODE;
          layer_opacity = 100;
          layer_posx = layer_posy = 0;
          if (inputLayers.size()==1) {
            if (!is_selection) {
              gimp_drawable_offsets(active_layer_id,&layer_posx,&layer_posy);
            }
            cimg_library::CImg<char>::string(gimp_item_get_name(active_layer_id)).move_to(layer_name);
          } else {
            layer_name.assign();
          }
          get_output_layer_props(imageNames[p],layer_blendmode,layer_opacity,layer_posx,layer_posy,layer_name);
          if (layer_posx + images[p]._width>max_width) {
            max_width = layer_posx + images[p]._width;
          }
          if (layer_posy + images[p]._height>max_height) {
            max_height = layer_posy + images[p]._height;
          }

          cimg_library::CImg<gmic_pixel_type> & img = images[p];
          if (gimp_image_base_type(gmic_qt_gimp_image_id)==GIMP_GRAY) {
            GmicQt::calibrate_image(img,!is_selection && (img.spectrum()==1 || img.spectrum()==3)?1:2,false);
          } else {
            GmicQt::calibrate_image(img,!is_selection && (img.spectrum()==1 || img.spectrum()==3)?3:4,false);
          }
          layer_id = gimp_layer_new(gmic_qt_gimp_image_id,verboseLayersLabel,img.width(),img.height(),
                                    spectrum2gimpImageTypes[std::min(img.spectrum(),4)],
              layer_opacity,layer_blendmode);
          if (!p) {
            top_layer_id = layer_id;
          }
          gimp_layer_set_offsets(layer_id,layer_posx,layer_posy);
          if (verboseLayersLabel) {
            gimp_item_set_name(layer_id,verboseLayersLabel);
          } else if (layer_name) {
            gimp_item_set_name(layer_id,layer_name);
          }
          gimp_image_insert_layer(gmic_qt_gimp_image_id,layer_id,-1,p);

#if (GIMP_MAJOR_VERSION<2) || ((GIMP_MAJOR_VERSION==2) && (GIMP_MINOR_VERSION<=8))
          GimpDrawable *drawable = gimp_drawable_get(layer_id);
          GimpPixelRgn region;
          gimp_pixel_rgn_init(&region,drawable,0,0,drawable->width,drawable->height,true,true);
          GmicQt::image2uchar(img);
          gimp_pixel_rgn_set_rect(&region,(guchar*)img.data(),0,0,img.width(),img.height());
          gimp_drawable_flush(drawable);
          gimp_drawable_merge_shadow(layer_id,true);
          gimp_drawable_update(layer_id,0,0,drawable->width,drawable->height);
          gimp_drawable_detach(drawable);
#else
          GeglBuffer *buffer = gimp_drawable_get_shadow_buffer(layer_id);
          const char *const format = img.spectrum()==1?"Y' float":img.spectrum()==2?"Y'A float": img.spectrum()==3?"R'G'B' float":"R'G'B'A float";
          (img/=255).permute_axes("cxyz");
          gegl_buffer_set(buffer,NULL,0,babl_format(format),img.data(),0);
          g_object_unref(buffer);
          gimp_drawable_merge_shadow(layer_id,true);
          gimp_drawable_update(layer_id,0,0,img.width(),img.height());
#endif
          img.assign();
        }
        const unsigned int Mw = std::max(image_width,max_width);
        const unsigned int Mh = std::max(image_height,max_height);
        if (Mw && Mh) {
          gimp_image_resize(gmic_qt_gimp_image_id,Mw,Mh,0,0);
        }
        if (outputMode == GmicQt::NewLayers) {
          gimp_image_set_active_layer(gmic_qt_gimp_image_id,active_layer_id);
        } else {
          gimp_image_set_active_layer(gmic_qt_gimp_image_id,top_layer_id);
        }
      }
      gimp_image_undo_group_end(gmic_qt_gimp_image_id);
    }
  } else if (outputMode == GmicQt::NewImage && images.size() ) {
    const gint active_layer_id = gimp_image_get_active_layer(gmic_qt_gimp_image_id);
    if (active_layer_id>=0) {
#if (GIMP_MAJOR_VERSION<2) || ((GIMP_MAJOR_VERSION==2) && (GIMP_MINOR_VERSION<=8))
      const int nimage_id = gimp_image_new(max_width,max_height,max_spectrum<=2?GIMP_GRAY:GIMP_RGB);
#else
      const int nimage_id = gimp_image_new_with_precision(max_width,max_height,max_spectrum<=2?GIMP_GRAY:GIMP_RGB,
                                                          gimp_image_get_precision(gmic_qt_gimp_image_id));
#endif
      gimp_image_undo_group_start(nimage_id);
      for (unsigned int p = 0; p < images.size(); ++p) {
        if (images[p]) {
          layer_blendmode = GIMP_NORMAL_MODE;
          layer_opacity = 100;
          layer_posx = layer_posy = 0;
          if (inputLayers.size() == 1) {
            if (!is_selection) {
              gimp_drawable_offsets(active_layer_id,&layer_posx,&layer_posy);
            }
            cimg_library::CImg<char>::string(gimp_item_get_name(active_layer_id)).move_to(layer_name);
          } else {
            layer_name.assign();
          }
          get_output_layer_props(imageNames[p],layer_blendmode,layer_opacity,layer_posx,layer_posy,layer_name);
          if ( is_selection ) {
            layer_posx = 0;
            layer_posy = 0;
          }
          cimg_library::CImg<gmic_pixel_type> & img = images[p];
          if (gimp_image_base_type(nimage_id)!=GIMP_GRAY) {
            GmicQt::calibrate_image(img,(img.spectrum()==1 || img.spectrum()==3)?3:4,false);
          }
          gint layer_id = gimp_layer_new(nimage_id,
                                         verboseLayersLabel,
                                         img.width(),
                                         img.height(),
                                         spectrum2gimpImageTypes[std::min(img.spectrum(),4)],
              layer_opacity,
              layer_blendmode);
          gimp_layer_set_offsets(layer_id,layer_posx,layer_posy);
          if (verboseLayersLabel) {
            gimp_item_set_name(layer_id,verboseLayersLabel);
          } else if (layer_name) {
            gimp_item_set_name(layer_id,layer_name);
          }
          gimp_image_insert_layer(nimage_id,layer_id,-1,p);

#if (GIMP_MAJOR_VERSION<2) || ((GIMP_MAJOR_VERSION==2) && (GIMP_MINOR_VERSION<=8))
          GimpDrawable *drawable = gimp_drawable_get(layer_id);
          GimpPixelRgn region;
          gimp_pixel_rgn_init(&region,drawable,0,0,drawable->width,drawable->height,true,true);
          GmicQt::image2uchar(img);
          gimp_pixel_rgn_set_rect(&region,(guchar*)img.data(),0,0,img.width(),img.height());
          gimp_drawable_flush(drawable);
          gimp_drawable_merge_shadow(layer_id,true);
          gimp_drawable_update(layer_id,0,0,drawable->width,drawable->height);
          gimp_drawable_detach(drawable);
#else
          GeglBuffer *buffer = gimp_drawable_get_shadow_buffer(layer_id);
          const char *const format = img.spectrum()==1?"Y' float":img.spectrum()==2?"Y'A float":
                                                                                    img.spectrum()==3?"R'G'B' float":"R'G'B'A float";
          (img/=255).permute_axes("cxyz");
          gegl_buffer_set(buffer,NULL,0,babl_format(format),img.data(),0);
          g_object_unref(buffer);
          gimp_drawable_merge_shadow(layer_id,true);
          gimp_drawable_update(layer_id,0,0,img.width(),img.height());
#endif
          img.assign();
        }
      }
      gimp_display_new(nimage_id);
      gimp_image_undo_group_end(nimage_id);
    }
  }

  gimp_displays_flush();
}

/*
 * 'Run' function, required by the GIMP plug-in API.
 */
void gmic_qt_run(const gchar * /* name */,
                 gint /* nparams */,
                 const GimpParam *param,
                 gint *nreturn_vals, GimpParam **return_vals) {
#if (GIMP_MAJOR_VERSION==2 && GIMP_MINOR_VERSION>8) || (GIMP_MAJOR_VERSION>=3)
  gegl_init(NULL,NULL);
  gimp_plugin_enable_precision();
#endif

  static GimpParam return_values[1];
  *return_vals  = return_values;
  *nreturn_vals = 1;
  return_values[0].type = GIMP_PDB_STATUS;
  int run_mode = (GimpRunMode)param[0].data.d_int32;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  switch (run_mode) {
  case GIMP_RUN_INTERACTIVE :
    gmic_qt_gimp_image_id = param[1].data.d_drawable;
    launchPlugin();
    break;
  case GIMP_RUN_WITH_LAST_VALS :
    gmic_qt_gimp_image_id = param[1].data.d_drawable;
    launchPluginHeadlessUsingLastParameters();
    break;
  case GIMP_RUN_NONINTERACTIVE :
    gmic_qt_gimp_image_id = param[1].data.d_drawable;
    launchPluginHeadless(param[5].data.d_string,
        (GmicQt::InputMode)(param[3].data.d_int32 + GmicQt::NoInput),
        GmicQt::OutputMode(param[4].data.d_int32 + GmicQt::InPlace));
    break;
  }
  return_values[0].data.d_status = status;
}

void gmic_qt_query() {
  static const GimpParamDef args[] = {
    {GIMP_PDB_INT32,    (gchar*)"run_mode", (gchar*)"Interactive, non-interactive"},
    {GIMP_PDB_IMAGE,    (gchar*)"image",    (gchar*)"Input image"},
    {GIMP_PDB_DRAWABLE, (gchar*)"drawable", (gchar*)"Input drawable (unused)"},
    {GIMP_PDB_INT32,    (gchar*)"input",
     (gchar*)
     "Input layers mode, when non-interactive"
     " (0=none, 1=active, 2=all, 3=active & below, 4=active & above, 5=all visibles, 6=all invisibles,"
     " 7=all visibles (decr.), 8=all invisibles (decr.), 9=all (decr.))"},
    {GIMP_PDB_INT32, (gchar*)"output",
     (gchar*)
     "Output mode, when non-interactive "
     "(0=in place,1=new layers,2=new active layers,3=new image)"},
    {GIMP_PDB_STRING, (gchar*)"command", (gchar*)"G'MIC command string, when non-interactive"}
  };

  const char name[] = "plug-in-gmic-qt";
  QByteArray blurb = QString("G'MIC - %1.%2.%3 (Qt)").arg(gmic_version/100).arg((gmic_version/10)%10).arg(gmic_version%10).toLatin1();
  QByteArray path = QString("G'MIC - %1.%2 (Qt)...").arg(gmic_version/100).arg((gmic_version/10)%10).toLatin1();
  path.prepend("_");
  gimp_install_procedure(name,                      // name
                         blurb.constData(),         // blurb
                         blurb.constData(),         // help
                         "S\303\251bastien Fourey", // author
                         "S\303\251bastien Fourey", // copyright
                         "2017",                    // date
                         path.constData(),          // menu_path
                         "RGB*, GRAY*",             // image_types
                         GIMP_PLUGIN,               // type
                         G_N_ELEMENTS(args),        // nparams
                         0,                         // nreturn_vals
                         args,                      // params
                         0);                        // return_vals
  gimp_plugin_menu_register(name, "<Image>/Filters");
}

GimpPlugInInfo PLUG_IN_INFO = { 0, 0, gmic_qt_query, gmic_qt_run };

MAIN()

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
#include <QRegularExpression>
#include <QString>
#include <algorithm>
#include <limits>
#include <stack>
#include <vector>
#ifdef _IS_WINDOWS_
#include <QTextCodec>
#endif
#include "Common.h"
#include "GmicQt.h"
#include "Host/GmicQtHost.h"
#include "ImageTools.h"
#include "gmic.h"

/*
 * Part of this code is much inspired by the original source code
 * of the GTK version of the gmic plug-in for GIMP by David Tschumperl\'e.
 */

#define GIMP_VERSION_LTE(MAJOR, MINOR) (GIMP_MAJOR_VERSION < MAJOR) || ((GIMP_MAJOR_VERSION == MAJOR) && (GIMP_MINOR_VERSION <= MINOR))

#define _gimp_image_get_item_position gimp_image_get_item_position

#if (GIMP_MAJOR_VERSION == 2) && (GIMP_MINOR_VERSION <= 7) && (GIMP_MICRO_VERSION <= 14)
#define _gimp_item_get_visible gimp_drawable_get_visible
#else
#define _gimp_item_get_visible gimp_item_get_visible
#endif

#if GIMP_CHECK_VERSION(2, 99, 6)
#define _gimp_image_get_width gimp_image_get_width
#define _gimp_image_get_height gimp_image_get_height
#define _gimp_image_get_base_type gimp_image_get_base_type
#define _gimp_drawable_get_width gimp_drawable_get_width
#define _gimp_drawable_get_height gimp_drawable_get_height
#define _gimp_drawable_get_offsets gimp_drawable_get_offsets
#else
#define _gimp_image_get_width gimp_image_width
#define _gimp_image_get_height gimp_image_height
#define _gimp_image_get_base_type gimp_image_base_type
#define _gimp_drawable_get_width gimp_drawable_width
#define _gimp_drawable_get_height gimp_drawable_height
#define _gimp_drawable_get_offsets gimp_drawable_offsets
#endif

#if GIMP_VERSION_LTE(2, 98)
#define _GimpImagePtr int
#define _GimpLayerPtr int
#define _GimpItemPtr int
#define _GIMP_ITEM(item) (item)
#define _GIMP_DRAWABLE(drawable) (drawable)
#define _GIMP_LAYER(layer) (layer)
#define _GIMP_NULL_LAYER -1
#define _GIMP_LAYER_IS_NOT_NULL(layer) ((layer) >= 0)
#define _gimp_top_layer 0

#else

#define _GimpImagePtr GimpImage *
#define _GimpLayerPtr GimpLayer *
#define _GimpItemPtr GimpItem *
#define _GIMP_ITEM(item) GIMP_ITEM(item)
#define _GIMP_DRAWABLE(drawable) GIMP_DRAWABLE(drawable)
#define _GIMP_LAYER(layer) GIMP_LAYER(layer)
#define _GIMP_NULL_LAYER NULL
#define _GIMP_LAYER_IS_NOT_NULL(layer) ((layer) != NULL)
#define _gimp_top_layer gimp_layer_get_by_id(0)

#define PLUG_IN_PROC "plug-in-gmic-qt"

typedef struct _GmicQtPlugin GmicQtPlugin;
typedef struct _GmicQtPluginClass GmicQtPluginClass;

struct _GmicQtPlugin {
  GimpPlugIn parent_instance;
};

struct _GmicQtPluginClass {
  GimpPlugInClass parent_class;
};

#define GMIC_QT_TYPE (gmic_qt_get_type())
// The object is called GmicQtPlugin to avoid name conflict with the namespace.
#define GMIC_QT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GMIC_QT_TYPE, GmicQtPlugin))

GType gmic_qt_get_type(void) G_GNUC_CONST;

static GList * gmic_qt_query(GimpPlugIn * plug_in);
static GimpProcedure * gmic_qt_create_procedure(GimpPlugIn * plug_in, const gchar * name);

#if (GIMP_MAJOR_VERSION <= 2) && (GIMP_MINOR_VERSION <= 99) && (GIMP_MICRO_VERSION < 6)
static GimpValueArray * gmic_qt_run(GimpProcedure * procedure, GimpRunMode run_mode, GimpImage * image, GimpDrawable * drawable, const GimpValueArray * args, gpointer run_data);
#else
static GimpValueArray * gmic_qt_run(GimpProcedure * procedure, GimpRunMode run_mode, GimpImage * image, gint n_drawables, GimpDrawable ** drawables, const GimpValueArray * args, gpointer run_data);
#endif

G_DEFINE_TYPE(GmicQtPlugin, gmic_qt, GIMP_TYPE_PLUG_IN)

GIMP_MAIN(GMIC_QT_TYPE)

static void gmic_qt_class_init(GmicQtPluginClass * klass)
{
  GimpPlugInClass * plug_in_class = GIMP_PLUG_IN_CLASS(klass);

  plug_in_class->query_procedures = gmic_qt_query;
  plug_in_class->create_procedure = gmic_qt_create_procedure;
}

static void gmic_qt_init(GmicQtPlugin * gmic_qt) {}

#endif

namespace GmicQtHost
{
const QString ApplicationName = QString("GIMP %1.%2").arg(GIMP_MAJOR_VERSION).arg(GIMP_MINOR_VERSION);
const char * const ApplicationShortname = GMIC_QT_XSTRINGIFY(GMIC_HOST);
#if GIMP_VERSION_LTE(2, 8)
const bool DarkThemeIsDefault = false;
#else
const bool DarkThemeIsDefault = true;
#endif

} // namespace GmicQtHost

namespace
{
_GimpImagePtr gmic_qt_gimp_image_id;

gmic_library::gmic_image<int> inputLayerDimensions;
std::vector<_GimpLayerPtr> inputLayers;

#if (GIMP_MAJOR_VERSION >= 3 || GIMP_MINOR_VERSION > 8) && !defined(GIMP_NORMAL_MODE)
typedef GimpLayerMode GimpLayerModeEffects;
#define GIMP_NORMAL_MODE GIMP_LAYER_MODE_NORMAL
const QMap<QString, GimpLayerModeEffects> BlendingModesMap = {{QString("alpha"), GIMP_LAYER_MODE_NORMAL},
                                                              {QString("normal"), GIMP_LAYER_MODE_NORMAL},
                                                              {QString("dissolve"), GIMP_LAYER_MODE_DISSOLVE},
                                                              {QString("behind"), GIMP_LAYER_MODE_BEHIND},
                                                              {QString("colorerase"), GIMP_LAYER_MODE_COLOR_ERASE},
                                                              {QString("erase"), GIMP_LAYER_MODE_ERASE},
                                                              {QString("merge"), GIMP_LAYER_MODE_MERGE},
                                                              {QString("split"), GIMP_LAYER_MODE_SPLIT},
                                                              {QString("lighten"), GIMP_LAYER_MODE_LIGHTEN_ONLY},
                                                              {QString("lumalighten"), GIMP_LAYER_MODE_LUMA_LIGHTEN_ONLY},
                                                              {QString("screen"), GIMP_LAYER_MODE_SCREEN},
                                                              {QString("dodge"), GIMP_LAYER_MODE_DODGE},
                                                              {QString("addition"), GIMP_LAYER_MODE_ADDITION},
                                                              {QString("darken"), GIMP_LAYER_MODE_DARKEN_ONLY},
                                                              {QString("lumadarken"), GIMP_LAYER_MODE_LUMA_DARKEN_ONLY},
                                                              {QString("multiply"), GIMP_LAYER_MODE_MULTIPLY},
                                                              {QString("burn"), GIMP_LAYER_MODE_BURN},
                                                              {QString("overlay"), GIMP_LAYER_MODE_OVERLAY},
                                                              {QString("softlight"), GIMP_LAYER_MODE_SOFTLIGHT},
                                                              {QString("hardlight"), GIMP_LAYER_MODE_HARDLIGHT},
                                                              {QString("vividlight"), GIMP_LAYER_MODE_VIVID_LIGHT},
                                                              {QString("pinlight"), GIMP_LAYER_MODE_PIN_LIGHT},
                                                              {QString("linearlight"), GIMP_LAYER_MODE_LINEAR_LIGHT},
                                                              {QString("hardmix"), GIMP_LAYER_MODE_HARD_MIX},
                                                              {QString("difference"), GIMP_LAYER_MODE_DIFFERENCE},
                                                              {QString("subtract"), GIMP_LAYER_MODE_SUBTRACT},
                                                              {QString("grainextract"), GIMP_LAYER_MODE_GRAIN_EXTRACT},
                                                              {QString("grainmerge"), GIMP_LAYER_MODE_GRAIN_MERGE},
                                                              {QString("divide"), GIMP_LAYER_MODE_DIVIDE},
                                                              {QString("hue"), GIMP_LAYER_MODE_HSV_HUE},
                                                              {QString("saturation"), GIMP_LAYER_MODE_HSV_SATURATION},
                                                              {QString("color"), GIMP_LAYER_MODE_HSL_COLOR},
                                                              {QString("value"), GIMP_LAYER_MODE_HSV_VALUE},
                                                              {QString("lchhue"), GIMP_LAYER_MODE_LCH_HUE},
                                                              {QString("lchchroma"), GIMP_LAYER_MODE_LCH_CHROMA},
                                                              {QString("lchcolor"), GIMP_LAYER_MODE_LCH_COLOR},
                                                              {QString("lchlightness"), GIMP_LAYER_MODE_LCH_LIGHTNESS},
                                                              {QString("luminance"), GIMP_LAYER_MODE_LUMINANCE},
                                                              {QString("exclusion"), GIMP_LAYER_MODE_EXCLUSION}};
#else
const QMap<QString, GimpLayerModeEffects> BlendingModesMap = {{QString("alpha"), GIMP_NORMAL_MODE},
                                                              {QString("normal"), GIMP_NORMAL_MODE},
                                                              {QString("dissolve"), GIMP_DISSOLVE_MODE},
                                                              {QString("lighten"), GIMP_LIGHTEN_ONLY_MODE},
                                                              {QString("screen"), GIMP_SCREEN_MODE},
                                                              {QString("dodge"), GIMP_DODGE_MODE},
                                                              {QString("addition"), GIMP_ADDITION_MODE},
                                                              {QString("darken"), GIMP_DARKEN_ONLY_MODE},
                                                              {QString("multiply"), GIMP_MULTIPLY_MODE},
                                                              {QString("burn"), GIMP_BURN_MODE},
                                                              {QString("overlay"), GIMP_OVERLAY_MODE},
                                                              {QString("softlight"), GIMP_SOFTLIGHT_MODE},
                                                              {QString("hardlight"), GIMP_HARDLIGHT_MODE},
                                                              {QString("difference"), GIMP_DIFFERENCE_MODE},
                                                              {QString("subtract"), GIMP_SUBTRACT_MODE},
                                                              {QString("grainextract"), GIMP_GRAIN_EXTRACT_MODE},
                                                              {QString("grainmerge"), GIMP_GRAIN_MERGE_MODE},
                                                              {QString("divide"), GIMP_DIVIDE_MODE},
                                                              {QString("hue"), GIMP_HUE_MODE},
                                                              {QString("saturation"), GIMP_SATURATION_MODE},
                                                              {QString("color"), GIMP_COLOR_MODE},
                                                              {QString("value"), GIMP_VALUE_MODE}};
#endif

QMap<GimpLayerModeEffects, QString> reverseBlendingModeMap(const QMap<QString, GimpLayerModeEffects> & string2mode)
{
  QMap<GimpLayerModeEffects, QString> result;
  QMap<QString, GimpLayerModeEffects>::const_iterator it = string2mode.cbegin();
  while (it != string2mode.cend()) {
    result[it.value()] = it.key();
    ++it;
  }
  result[GIMP_NORMAL_MODE] = QString("alpha");
  return result;
}

QString blendingMode2String(const GimpLayerModeEffects & blendingMode)
{
  static QMap<GimpLayerModeEffects, QString> mode2string = reverseBlendingModeMap(BlendingModesMap);
  QMap<GimpLayerModeEffects, QString>::const_iterator it = mode2string.find(blendingMode);
  if (it != mode2string.cend()) {
    return it.value();
  } else {
    return QString("alpha");
  }
}

#ifdef _IS_WINDOWS_
QByteArray mapToASCII(const char * str)
{
  static const QTextCodec * codec = QTextCodec::codecForName("ASCII");
  return codec->fromUnicode(QString::fromUtf8(str));
}
inline void _GIMP_ITEM_SET_NAME(_GimpItemPtr item_ID, const gchar * name)
{
  gimp_item_set_name(item_ID, mapToASCII(name).constData());
}
#else
inline void _GIMP_ITEM_SET_NAME(_GimpItemPtr item_ID, const gchar * name)
{
  gimp_item_set_name(item_ID, name);
}
#endif

// Get layer blending mode from string.
//-------------------------------------
void get_output_layer_props(const char * const s, GimpLayerModeEffects & blendmode, double & opacity, int & posx, int & posy, gmic_library::gmic_image<char> & name)
{
  if (!s || !*s)
    return;
  QString str(s);

  // Read output blending mode.
  QRegularExpression modeRe(R"_(mode\(\s*([^)]*)\s*\))_");
  QRegularExpressionMatch match = modeRe.match(str);
  if (match.hasMatch()) {
    QString modeStr = match.captured(1).trimmed();
    if (BlendingModesMap.find(modeStr) != BlendingModesMap.end()) {
      blendmode = BlendingModesMap[modeStr];
    }
  }

  // Read output opacity.
  QRegularExpression opacityRe(R"_(opacity\(\s*([^)]*)\s*\))_");
  match = opacityRe.match(str);
  if (match.hasMatch()) {
    QString opacityStr = match.captured(1).trimmed();
    bool ok = false;
    double x = opacityStr.toDouble(&ok);
    if (ok) {
      opacity = x;
      if (opacity < 0) {
        opacity = 0;
      } else if (opacity > 100) {
        opacity = 100;
      }
    }
  }

  // Read output positions.
  QRegularExpression posRe(R"_(pos\(\s*(-?\d*)[^)](-?\d*)\s*\))_"); // FIXME : Allow more spaces
  match = posRe.match(str);
  if (match.hasMatch()) {
    QString xStr = match.captured(1);
    QString yStr = match.captured(2);
    bool okX = false;
    bool okY = false;
    int x = xStr.toInt(&okX);
    int y = yStr.toInt(&okY);
    if (okX && okY) {
      posx = x;
      posy = y;
    }
  }

  // Read output name.
  const char * S = std::strstr(s, "name(");
  if (S) {
    const char * ps = S + 5;
    unsigned int level = 1;
    while (*ps && level) {
      if (*ps == '(') {
        ++level;
      } else if (*ps == ')') {
        --level;
      }
      ++ps;
    }
    if (!level || *(ps - 1) == ')') {
      name.assign(S + 5, (unsigned int)(ps - S - 5)).back() = 0;
      cimg_for(name, pn, char)
      {
        if (*pn == 21) {
          *pn = '(';
        } else if (*pn == 22) {
          *pn = ')';
        }
      }
    }
  }
}

_GimpLayerPtr * get_gimp_layers_flat_list(_GimpImagePtr imageId, int * count)
{
  static std::vector<_GimpLayerPtr> layersId;
  std::stack<_GimpLayerPtr> idStack;

  int layersCount = 0;
  _GimpLayerPtr * layers = gimp_image_get_layers(imageId, &layersCount);
  for (int i = layersCount - 1; i >= 0; --i) {
    idStack.push(layers[i]);
  }

  layersId.clear();
  while (!idStack.empty()) {
    if (gimp_item_is_group(_GIMP_ITEM(idStack.top()))) {
      int childCount = 0;
      _GimpItemPtr * children = gimp_item_get_children(_GIMP_ITEM(idStack.top()), &childCount);
      idStack.pop();
      for (int i = childCount - 1; i >= 0; --i) {
        idStack.push(_GIMP_LAYER(children[i])); // TODO: Check if layers can have non-layer children.
      }
    } else {
      layersId.push_back(idStack.top());
      idStack.pop();
    }
  }
  *count = layersId.size();
  return layersId.data();
}

template <typename T> void image2uchar(gmic_library::gmic_image<T> & img)
{
  unsigned int len = img.width() * img.height();
  auto dst = reinterpret_cast<unsigned char *>(img.data());
  switch (img.spectrum()) {
  case 1: {
    const T * src = img.data(0, 0, 0, 0);
    while (len--) {
      *dst++ = static_cast<unsigned char>(*src++);
    }
  } break;
  case 2: {
    const T * srcG = img.data(0, 0, 0, 0);
    const T * srcA = img.data(0, 0, 0, 1);
    while (len--) {
      dst[0] = static_cast<unsigned char>(*srcG++);
      dst[1] = static_cast<unsigned char>(*srcA++);
      dst += 2;
    }
  } break;
  case 3: {
    const T * srcR = img.data(0, 0, 0, 0);
    const T * srcG = img.data(0, 0, 0, 1);
    const T * srcB = img.data(0, 0, 0, 2);
    while (len--) {
      dst[0] = static_cast<unsigned char>(*srcR++);
      dst[1] = static_cast<unsigned char>(*srcG++);
      dst[2] = static_cast<unsigned char>(*srcB++);
      dst += 3;
    }
  } break;
  case 4: {
    const T * srcR = img.data(0, 0, 0, 0);
    const T * srcG = img.data(0, 0, 0, 1);
    const T * srcB = img.data(0, 0, 0, 2);
    const T * srcA = img.data(0, 0, 0, 3);
    while (len--) {
      dst[0] = static_cast<unsigned char>(*srcR++);
      dst[1] = static_cast<unsigned char>(*srcG++);
      dst[2] = static_cast<unsigned char>(*srcB++);
      dst[3] = static_cast<unsigned char>(*srcA++);
      dst += 4;
    }
  } break;
  default:
    return;
  }
}

} // namespace

namespace GmicQtHost
{

void showMessage(const char * message)
{
  static bool first = true;

  if (first) {
    gimp_progress_init(message);
    first = false;
  } else {
    gimp_progress_set_text_printf("%s", message);
  }
}

void applyColorProfile(gmic_library::gmic_image<float> & image)
{
#if GIMP_VERSION_LTE(2, 8)
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
//    gmic_library::gmic_image<float> corrected;
//    image.get_permute_axes("cxyz").move_to(corrected) /= 255;
//    gimp_color_transform_process_pixels(transform,fmt,corrected,fmt,corrected,
//                                        corrected.height()*corrected.depth());
//    (corrected.permute_axes("yzcx")*=255).cut(0,255).move_to(image);
//    g_object_unref(transform);
#endif
}

void getLayersExtent(int * width, int * height, GmicQt::InputMode mode)
{
  int layersCount = 0;
  // _GimpLayerPtr * begLayers = gimp_image_get_layers(gmic_qt_gimp_image_id, &layersCount);
  _GimpLayerPtr * begLayers = get_gimp_layers_flat_list(gmic_qt_gimp_image_id, &layersCount);
  _GimpLayerPtr * endLayers = begLayers + layersCount;
#if GIMP_CHECK_VERSION(2, 99, 12)
  GList * selected_layers = gimp_image_list_selected_layers(gmic_qt_gimp_image_id);
  GList * first_layer = g_list_first(selected_layers);
  _GimpLayerPtr activeLayerID = (_GimpLayerPtr)first_layer->data;
#else
  _GimpLayerPtr activeLayerID = gimp_image_get_active_layer(gmic_qt_gimp_image_id);
#endif

  // Build list of input layers IDs
  std::vector<_GimpLayerPtr> layers;
  switch (mode) {
  case GmicQt::InputMode::NoInput:
    break;
  case GmicQt::InputMode::Active:
    if (_GIMP_LAYER_IS_NOT_NULL(activeLayerID) && !gimp_item_is_group(_GIMP_ITEM(activeLayerID))) {
      layers.push_back(activeLayerID);
    }
    break;
  case GmicQt::InputMode::All:
    layers.assign(begLayers, endLayers);
    break;
  case GmicQt::InputMode::ActiveAndBelow:
    if (_GIMP_LAYER_IS_NOT_NULL(activeLayerID) && !gimp_item_is_group(_GIMP_ITEM(activeLayerID))) {
      layers.push_back(activeLayerID);
      _GimpLayerPtr * p = std::find(begLayers, endLayers, activeLayerID);
      if (p < endLayers - 1) {
        layers.push_back(*(p + 1));
      }
    }
    break;
  case GmicQt::InputMode::ActiveAndAbove:
    if (_GIMP_LAYER_IS_NOT_NULL(activeLayerID) && !gimp_item_is_group(_GIMP_ITEM(activeLayerID))) {
      _GimpLayerPtr * p = std::find(begLayers, endLayers, activeLayerID);
      if (p > begLayers) {
        layers.push_back(*(p - 1));
      }
      layers.push_back(activeLayerID);
    }
    break;
  case GmicQt::InputMode::AllVisible:
  case GmicQt::InputMode::AllInvisible: {
    bool visibility = (mode == GmicQt::InputMode::AllVisible);
    for (int i = 0; i < layersCount; ++i) {
      if (_gimp_item_get_visible(_GIMP_ITEM(begLayers[i])) == visibility) {
        layers.push_back(begLayers[i]);
      }
    }
  } break;
  default:
    break;
  }

  gint rgn_x, rgn_y, rgn_width, rgn_height;
  *width = 0;
  *height = 0;
  for (_GimpLayerPtr layer : layers) {
    if (!gimp_item_is_valid(_GIMP_ITEM(layer))) {
      continue;
    }
    if (!gimp_drawable_mask_intersect(_GIMP_DRAWABLE(layer), &rgn_x, &rgn_y, &rgn_width, &rgn_height)) {
      continue;
    }
    *width = std::max(*width, rgn_width);
    *height = std::max(*height, rgn_height);
  }
}

void getCroppedImages(gmic_list<float> & images, gmic_list<char> & imageNames, double x, double y, double width, double height, GmicQt::InputMode mode)
{
  using gmic_library::gmic_image;
  using gmic_library::gmic_list;
  int layersCount = 0;
  _GimpLayerPtr * layers = get_gimp_layers_flat_list(gmic_qt_gimp_image_id, &layersCount);
  _GimpLayerPtr * end_layers = layers + layersCount;
#if GIMP_CHECK_VERSION(2, 99, 12)
  GList * selected_layers = gimp_image_list_selected_layers(gmic_qt_gimp_image_id);
  GList * first_layer = g_list_first(selected_layers);
  _GimpLayerPtr active_layer_id = (_GimpLayerPtr)first_layer->data;
#else
  _GimpLayerPtr active_layer_id = gimp_image_get_active_layer(gmic_qt_gimp_image_id);
#endif

  const bool entireImage = (x < 0 && y < 0 && width < 0 && height < 0) || (x == 0.0 && y == 0 && width == 1 && height == 0);
  if (entireImage) {
    x = 0.0;
    y = 0.0;
    width = 1.0;
    height = 1.0;
  }

  // Build list of input layers IDs
  inputLayers.clear();
  switch (mode) {
  case GmicQt::InputMode::NoInput:
    break;
  case GmicQt::InputMode::Active:
    if (_GIMP_LAYER_IS_NOT_NULL(active_layer_id) && !gimp_item_is_group(_GIMP_ITEM(active_layer_id))) {
      inputLayers.push_back(active_layer_id);
    }
    break;
  case GmicQt::InputMode::All:
    inputLayers.assign(layers, end_layers);
    break;
  case GmicQt::InputMode::ActiveAndBelow:
    if (_GIMP_LAYER_IS_NOT_NULL(active_layer_id) && !gimp_item_is_group(_GIMP_ITEM(active_layer_id))) {
      inputLayers.push_back(active_layer_id);
      _GimpLayerPtr * p = std::find(layers, end_layers, active_layer_id);
      if (p < end_layers - 1) {
        inputLayers.push_back(*(p + 1));
      }
    }
    break;
  case GmicQt::InputMode::ActiveAndAbove:
    if (_GIMP_LAYER_IS_NOT_NULL(active_layer_id) && !gimp_item_is_group(_GIMP_ITEM(active_layer_id))) {
      _GimpLayerPtr * p = std::find(layers, end_layers, active_layer_id);
      if (p > layers) {
        inputLayers.push_back(*(p - 1));
      }
      inputLayers.push_back(active_layer_id);
    }
    break;
  case GmicQt::InputMode::AllVisible:
  case GmicQt::InputMode::AllInvisible: {
    bool visibility = (mode == GmicQt::InputMode::AllVisible);
    for (int i = 0; i < layersCount; ++i) {
      if (_gimp_item_get_visible(_GIMP_ITEM(layers[i])) == visibility) {
        inputLayers.push_back(layers[i]);
      }
    }
  } break;
  default:
    break;
  }

  // Retrieve image list
  images.assign(inputLayers.size());
  imageNames.assign(inputLayers.size());
  inputLayerDimensions.assign(inputLayers.size(), 4);
  gint rgn_x, rgn_y, rgn_width, rgn_height;

  gboolean isSelection = 0;
  gint selX1 = 0, selY1 = 0, selX2 = 0, selY2 = 0;
  if (!gimp_selection_bounds(gmic_qt_gimp_image_id, &isSelection, &selX1, &selY1, &selX2, &selY2)) {
    isSelection = 0;
    selX1 = 0;
    selY1 = 0;
  }

  cimglist_for(images, l)
  {
    if (!gimp_item_is_valid(_GIMP_ITEM(inputLayers[l]))) {
      continue;
    }
    if (!gimp_drawable_mask_intersect(_GIMP_DRAWABLE(inputLayers[l]), &rgn_x, &rgn_y, &rgn_width, &rgn_height)) {
      inputLayerDimensions(l, 0) = 0;
      inputLayerDimensions(l, 1) = 0;
      inputLayerDimensions(l, 2) = 0;
      inputLayerDimensions(l, 3) = 0;
      continue;
    }
    const int spectrum = (gimp_drawable_is_rgb(_GIMP_DRAWABLE(inputLayers[l])) ? 3 : 1) + (gimp_drawable_has_alpha(_GIMP_DRAWABLE(inputLayers[l])) ? 1 : 0);

    const int dw = static_cast<int>(_gimp_drawable_get_width(_GIMP_DRAWABLE(inputLayers[l])));
    const int dh = static_cast<int>(_gimp_drawable_get_height(_GIMP_DRAWABLE(inputLayers[l])));
    const int ix = static_cast<int>(entireImage ? rgn_x : (rgn_x + x * rgn_width));
    const int iy = static_cast<int>(entireImage ? rgn_y : (rgn_y + y * rgn_height));
    const int iw = entireImage ? rgn_width : std::min(dw - ix, static_cast<int>(1 + std::ceil(rgn_width * width)));
    const int ih = entireImage ? rgn_height : std::min(dh - iy, static_cast<int>(1 + std::ceil(rgn_height * height)));

    if (entireImage) {
      inputLayerDimensions(l, 0) = rgn_width;
      inputLayerDimensions(l, 1) = rgn_height;
    } else {
      inputLayerDimensions(l, 0) = iw;
      inputLayerDimensions(l, 1) = ih;
    }
    inputLayerDimensions(l, 2) = 1;
    inputLayerDimensions(l, 3) = spectrum;

    const float opacity = gimp_layer_get_opacity(inputLayers[l]);
    const GimpLayerModeEffects blendMode = gimp_layer_get_mode(inputLayers[l]);
    int xPos = 0;
    int yPos = 0;
    if (isSelection) {
      xPos = selX1;
      yPos = selY1;
    } else {
      _gimp_drawable_get_offsets(_GIMP_DRAWABLE(inputLayers[l]), &xPos, &yPos);
    }
    QString noParenthesisName(gimp_item_get_name(_GIMP_ITEM(inputLayers[l])));
    noParenthesisName.replace(QChar('('), QChar(21)).replace(QChar(')'), QChar(22));

    QString name = QString("mode(%1),opacity(%2),pos(%3,%4),name(%5)").arg(blendingMode2String(blendMode)).arg(opacity).arg(xPos).arg(yPos).arg(noParenthesisName);
    QByteArray ba = name.toUtf8();
    gmic_image<char>::string(ba.constData()).move_to(imageNames[l]);
#if GIMP_VERSION_LTE(2, 8)
    GimpDrawable * drawable = gimp_drawable_get(inputLayers[l]);
    GimpPixelRgn region;
    gimp_pixel_rgn_init(&region, drawable, ix, iy, iw, ih, false, false);
    gmic_image<unsigned char> img(spectrum, iw, ih);
    gimp_pixel_rgn_get_rect(&region, img, ix, iy, iw, ih);
    gimp_drawable_detach(drawable);
    img.permute_axes("yzcx");
#else
    GeglRectangle rect;
    gegl_rectangle_set(&rect, ix, iy, iw, ih);
    GeglBuffer * buffer = gimp_drawable_get_buffer(_GIMP_DRAWABLE(inputLayers[l]));
    const char * const format = spectrum == 1 ? "Y' " gmic_pixel_type_str : spectrum == 2 ? "Y'A " gmic_pixel_type_str : spectrum == 3 ? "R'G'B' " gmic_pixel_type_str : "R'G'B'A " gmic_pixel_type_str;
    gmic_image<float> img(spectrum, iw, ih);
    gegl_buffer_get(buffer, &rect, 1, babl_format(format), img.data(), 0, GEGL_ABYSS_NONE);
    (img *= 255).permute_axes("yzcx");
    g_object_unref(buffer);
#endif
    img.move_to(images[l]);
  }
}

void outputImages(gmic_list<gmic_pixel_type> & images, const gmic_list<char> & imageNames, GmicQt::OutputMode outputMode)
{
  // Output modes in original gmic_gimp_gtk : 0/Replace 1/New layer 2/New active layer  3/New image

  // spectrum to GIMP image types conversion table
  GimpImageType spectrum2gimpImageTypes[5] = {GIMP_INDEXED_IMAGE, // (unused)
                                              GIMP_GRAY_IMAGE, GIMP_GRAYA_IMAGE, GIMP_RGB_IMAGE, GIMP_RGBA_IMAGE};

  // Get output layers dimensions and check if input/output layers have compatible dimensions.
  unsigned int max_spectrum = 0;
  struct Position {
    gint x, y;
    Position() : x(0), y(0) {}
  } top_left, bottom_right;
  cimglist_for(images, l)
  {
    if (images[l].is_empty()) {
      images.remove(l--);
      continue;
    } // Discard possible empty images.

    bottom_right.x = std::max(bottom_right.x, (gint)images[l]._width);
    bottom_right.y = std::max(bottom_right.y, (gint)images[l]._height);
    if (images[l]._spectrum > max_spectrum) {
      max_spectrum = images[l]._spectrum;
    }
  }

  int image_nb_layers = 0;
  get_gimp_layers_flat_list(gmic_qt_gimp_image_id, &image_nb_layers);
  unsigned int image_width = 0, image_height = 0;
  if (inputLayers.size()) {
    image_width = _gimp_image_get_width(gmic_qt_gimp_image_id);
    image_height = _gimp_image_get_height(gmic_qt_gimp_image_id);
  }

  int is_selection = 0, sel_x0 = 0, sel_y0 = 0, sel_x1 = 0, sel_y1 = 0;
  if (!gimp_selection_bounds(gmic_qt_gimp_image_id, &is_selection, &sel_x0, &sel_y0, &sel_x1, &sel_y1)) {
    is_selection = 0;
  } else if (outputMode == GmicQt::OutputMode::InPlace || outputMode == GmicQt::OutputMode::NewImage) {
    sel_x0 = sel_y0 = 0;
  }

  bool is_compatible_dimensions = (images.size() == inputLayers.size());
  for (unsigned int p = 0; p < images.size() && is_compatible_dimensions; ++p) {
    const gmic_library::gmic_image<gmic_pixel_type> & img = images[p];
    const bool source_is_alpha = (inputLayerDimensions(p, 3) == 2 || inputLayerDimensions(p, 3) >= 4);
    const bool dest_is_alpha = (img.spectrum() == 2 || img.spectrum() >= 4);
    if (dest_is_alpha && !source_is_alpha) {
      gimp_layer_add_alpha(inputLayers[p]);
      ++inputLayerDimensions(p, 3);
    }
    if (img.width() != inputLayerDimensions(p, 0) || img.height() != inputLayerDimensions(p, 1)) {
      is_compatible_dimensions = false;
    }
  }

  // Transfer output layers back into GIMP.
  GimpLayerModeEffects layer_blendmode = GIMP_NORMAL_MODE;
  gint layer_posx = 0, layer_posy = 0;
  double layer_opacity = 100;
  gmic_library::gmic_image<char> layer_name;

  if (outputMode == GmicQt::OutputMode::InPlace) {
    gint rgn_x, rgn_y, rgn_width, rgn_height;
    gimp_image_undo_group_start(gmic_qt_gimp_image_id);
    if (is_compatible_dimensions) { // Direct replacement of the layer data.
      for (unsigned int p = 0; p < images.size(); ++p) {
        layer_blendmode = gimp_layer_get_mode(inputLayers[p]);
        layer_opacity = gimp_layer_get_opacity(inputLayers[p]);
        _gimp_drawable_get_offsets(_GIMP_DRAWABLE(inputLayers[p]), &layer_posx, &layer_posy);
        gmic_library::gmic_image<char>::string(gimp_item_get_name(_GIMP_ITEM(inputLayers[p]))).move_to(layer_name);
        get_output_layer_props(imageNames[p], layer_blendmode, layer_opacity, layer_posx, layer_posy, layer_name);
        gmic_library::gmic_image<gmic_pixel_type> & img = images[p];
        GmicQt::calibrateImage(img, inputLayerDimensions(p, 3), false);
        if (gimp_drawable_mask_intersect(_GIMP_DRAWABLE(inputLayers[p]), &rgn_x, &rgn_y, &rgn_width, &rgn_height)) {
#if GIMP_VERSION_LTE(2, 8)
          GimpDrawable * drawable = gimp_drawable_get(inputLayers[p]);
          GimpPixelRgn region;
          gimp_pixel_rgn_init(&region, drawable, rgn_x, rgn_y, rgn_width, rgn_height, true, true);
          image2uchar(img);
          gimp_pixel_rgn_set_rect(&region, (guchar *)img.data(), rgn_x, rgn_y, rgn_width, rgn_height);
          gimp_drawable_flush(drawable);
          gimp_drawable_merge_shadow(inputLayers[p], true);
          gimp_drawable_update(inputLayers[p], rgn_x, rgn_y, rgn_width, rgn_height);
          gimp_drawable_detach(drawable);
#else
          GeglRectangle rect;
          gegl_rectangle_set(&rect, rgn_x, rgn_y, rgn_width, rgn_height);
          GeglBuffer * buffer = gimp_drawable_get_shadow_buffer(_GIMP_DRAWABLE(inputLayers[p]));
          const char * const format = img.spectrum() == 1 ? "Y' float" : img.spectrum() == 2 ? "Y'A float" : img.spectrum() == 3 ? "R'G'B' float" : "R'G'B'A float";
          (img /= 255).permute_axes("cxyz");
          gegl_buffer_set(buffer, &rect, 0, babl_format(format), img.data(), 0);
          g_object_unref(buffer);
          gimp_drawable_merge_shadow(_GIMP_DRAWABLE(inputLayers[p]), true);
          gimp_drawable_update(_GIMP_DRAWABLE(inputLayers[p]), 0, 0, img.width(), img.height());
#endif
          gimp_layer_set_mode(inputLayers[p], layer_blendmode);
          gimp_layer_set_opacity(inputLayers[p], layer_opacity);
          if (!is_selection) {
            gimp_layer_set_offsets(inputLayers[p], layer_posx, layer_posy);
          } else {
#if GIMP_VERSION_LTE(2, 8)
            gimp_layer_translate(inputLayers[p], 0, 0);
#else
            gimp_item_transform_translate(_GIMP_ITEM(inputLayers[p]), 0, 0);
#endif
          }
          if (layer_name) {
            _GIMP_ITEM_SET_NAME(_GIMP_ITEM(inputLayers[p]), layer_name);
          }
        }
        img.assign();
      }
    } else { // Indirect replacement: create new layers.
      gimp_selection_none(gmic_qt_gimp_image_id);
      const int layer_pos = _gimp_image_get_item_position(gmic_qt_gimp_image_id, _GIMP_ITEM(inputLayers[0]));
      top_left.x = top_left.y = 0;
      bottom_right.x = bottom_right.y = 0;
      for (unsigned int p = 0; p < images.size(); ++p) {
        if (images[p]) {
          layer_posx = layer_posy = 0;
          if (p < inputLayers.size()) {
            layer_blendmode = gimp_layer_get_mode(inputLayers[p]);
            layer_opacity = gimp_layer_get_opacity(inputLayers[p]);
            if (!is_selection) {
              _gimp_drawable_get_offsets(_GIMP_DRAWABLE(inputLayers[p]), &layer_posx, &layer_posy);
            }
            gmic_library::gmic_image<char>::string(gimp_item_get_name(_GIMP_ITEM(inputLayers[p]))).move_to(layer_name);
            gimp_image_remove_layer(gmic_qt_gimp_image_id, inputLayers[p]);
          } else {
            layer_blendmode = GIMP_NORMAL_MODE;
            layer_opacity = 100;
            layer_name.assign();
          }
          get_output_layer_props(imageNames[p], layer_blendmode, layer_opacity, layer_posx, layer_posy, layer_name);
          if (is_selection) {
            layer_posx = 0;
            layer_posy = 0;
          }
          top_left.x = std::min(top_left.x, layer_posx);
          top_left.y = std::min(top_left.y, layer_posy);
          bottom_right.x = std::max(bottom_right.x, (gint)(layer_posx + images[p]._width));
          bottom_right.y = std::max(bottom_right.y, (gint)(layer_posy + images[p]._height));
          gmic_library::gmic_image<gmic_pixel_type> & img = images[p];
          if (_gimp_image_get_base_type(gmic_qt_gimp_image_id) == GIMP_GRAY) {
            GmicQt::calibrateImage(img, (img.spectrum() == 1 || img.spectrum() == 3) ? 1 : 2, false);
          } else {
            GmicQt::calibrateImage(img, (img.spectrum() == 1 || img.spectrum() == 3) ? 3 : 4, false);
          }
          _GimpLayerPtr layer_id = gimp_layer_new(gmic_qt_gimp_image_id, nullptr, img.width(), img.height(), spectrum2gimpImageTypes[std::min(img.spectrum(), 4)], layer_opacity, layer_blendmode);
          gimp_layer_set_offsets(layer_id, layer_posx, layer_posy);
          if (layer_name) {
            _GIMP_ITEM_SET_NAME(_GIMP_ITEM(layer_id), layer_name);
          }
          gimp_image_insert_layer(gmic_qt_gimp_image_id, layer_id, _GIMP_NULL_LAYER, layer_pos + p);

#if GIMP_VERSION_LTE(2, 8)
          GimpDrawable * drawable = gimp_drawable_get(layer_id);
          GimpPixelRgn region;
          gimp_pixel_rgn_init(&region, drawable, 0, 0, drawable->width, drawable->height, true, true);
          image2uchar(img);
          gimp_pixel_rgn_set_rect(&region, (guchar *)img.data(), 0, 0, img.width(), img.height());
          gimp_drawable_flush(drawable);
          gimp_drawable_merge_shadow(layer_id, true);
          gimp_drawable_update(layer_id, 0, 0, drawable->width, drawable->height);
          gimp_drawable_detach(drawable);
#else
          GeglBuffer * buffer = gimp_drawable_get_shadow_buffer(_GIMP_DRAWABLE(layer_id));
          const char * const format = img.spectrum() == 1 ? "Y' float" : img.spectrum() == 2 ? "Y'A float" : img.spectrum() == 3 ? "R'G'B' float" : "R'G'B'A float";
          (img /= 255).permute_axes("cxyz");
          gegl_buffer_set(buffer, NULL, 0, babl_format(format), img.data(), 0);
          g_object_unref(buffer);
          gimp_drawable_merge_shadow(_GIMP_DRAWABLE(layer_id), true);
          gimp_drawable_update(_GIMP_DRAWABLE(layer_id), 0, 0, img.width(), img.height());
#endif
          img.assign();
        }
      }
      const unsigned int max_width = bottom_right.x - top_left.x;
      const unsigned int max_height = bottom_right.y - top_left.y;
      for (unsigned int p = images._width; p < inputLayers.size(); ++p) {
        gimp_image_remove_layer(gmic_qt_gimp_image_id, inputLayers[p]);
      }
      if ((unsigned int)image_nb_layers == inputLayers.size()) {
        gimp_image_resize(gmic_qt_gimp_image_id, max_width, max_height, -top_left.x, -top_left.y);
      } else {
        gimp_image_resize(gmic_qt_gimp_image_id, std::max(image_width, max_width), std::max(image_height, max_height), 0, 0);
      }
    }
    gimp_image_undo_group_end(gmic_qt_gimp_image_id);
  } else if (outputMode == GmicQt::OutputMode::NewActiveLayers || outputMode == GmicQt::OutputMode::NewLayers) {
#if GIMP_CHECK_VERSION(2, 99, 12)
    GList * selected_layers = gimp_image_list_selected_layers(gmic_qt_gimp_image_id);
    GList * first_layer = g_list_first(selected_layers);
    _GimpLayerPtr active_layer_id = (_GimpLayerPtr)first_layer->data;
#else
    _GimpLayerPtr active_layer_id = gimp_image_get_active_layer(gmic_qt_gimp_image_id);
#endif
    if (_GIMP_LAYER_IS_NOT_NULL(active_layer_id)) {
      gimp_image_undo_group_start(gmic_qt_gimp_image_id);
      _GimpLayerPtr top_layer_id = _gimp_top_layer;
      _GimpLayerPtr layer_id = _gimp_top_layer;
      top_left.x = top_left.y = 0;
      bottom_right.x = bottom_right.y = 0;
      for (unsigned int p = 0; p < images.size(); ++p) {
        if (images[p]) {
          layer_blendmode = GIMP_NORMAL_MODE;
          layer_opacity = 100;
          layer_posx = layer_posy = 0;
          if (inputLayers.size() == 1) {
            if (!is_selection) {
              _gimp_drawable_get_offsets(_GIMP_DRAWABLE(active_layer_id), &layer_posx, &layer_posy);
            }
            gmic_library::gmic_image<char>::string(gimp_item_get_name(_GIMP_ITEM(active_layer_id))).move_to(layer_name);
          } else {
            layer_name.assign();
          }
          get_output_layer_props(imageNames[p], layer_blendmode, layer_opacity, layer_posx, layer_posy, layer_name);
          top_left.x = std::min(top_left.x, layer_posx);
          top_left.y = std::min(top_left.y, layer_posy);
          bottom_right.x = std::max(bottom_right.x, (gint)(layer_posx + images[p]._width));
          bottom_right.y = std::max(bottom_right.y, (gint)(layer_posy + images[p]._height));

          gmic_library::gmic_image<gmic_pixel_type> & img = images[p];
          if (_gimp_image_get_base_type(gmic_qt_gimp_image_id) == GIMP_GRAY) {
            GmicQt::calibrateImage(img, !is_selection && (img.spectrum() == 1 || img.spectrum() == 3) ? 1 : 2, false);
          } else {
            GmicQt::calibrateImage(img, !is_selection && (img.spectrum() == 1 || img.spectrum() == 3) ? 3 : 4, false);
          }
          layer_id = gimp_layer_new(gmic_qt_gimp_image_id, nullptr, img.width(), img.height(), spectrum2gimpImageTypes[std::min(img.spectrum(), 4)], layer_opacity, layer_blendmode);
          if (!p) {
            top_layer_id = layer_id;
          }
          gimp_layer_set_offsets(layer_id, layer_posx, layer_posy);
          if (layer_name) {
            _GIMP_ITEM_SET_NAME(_GIMP_ITEM(layer_id), layer_name);
          }
          gimp_image_insert_layer(gmic_qt_gimp_image_id, layer_id, _GIMP_NULL_LAYER, p);

#if GIMP_VERSION_LTE(2, 8)
          GimpDrawable * drawable = gimp_drawable_get(layer_id);
          GimpPixelRgn region;
          gimp_pixel_rgn_init(&region, drawable, 0, 0, drawable->width, drawable->height, true, true);
          image2uchar(img);
          gimp_pixel_rgn_set_rect(&region, (guchar *)img.data(), 0, 0, img.width(), img.height());
          gimp_drawable_flush(drawable);
          gimp_drawable_merge_shadow(layer_id, true);
          gimp_drawable_update(layer_id, 0, 0, drawable->width, drawable->height);
          gimp_drawable_detach(drawable);
#else
          GeglBuffer * buffer = gimp_drawable_get_shadow_buffer(_GIMP_DRAWABLE(layer_id));
          const char * const format = img.spectrum() == 1 ? "Y' float" : img.spectrum() == 2 ? "Y'A float" : img.spectrum() == 3 ? "R'G'B' float" : "R'G'B'A float";
          (img /= 255).permute_axes("cxyz");
          gegl_buffer_set(buffer, NULL, 0, babl_format(format), img.data(), 0);
          g_object_unref(buffer);
          gimp_drawable_merge_shadow(_GIMP_DRAWABLE(layer_id), true);
          gimp_drawable_update(_GIMP_DRAWABLE(layer_id), 0, 0, img.width(), img.height());
#endif
          img.assign();
        }
        const unsigned int max_width = bottom_right.x - top_left.x;
        const unsigned int max_height = bottom_right.y - top_left.y;
        const unsigned int Mw = std::max(image_width, max_width);
        const unsigned int Mh = std::max(image_height, max_height);
        if (Mw && Mh) {
          gimp_image_resize(gmic_qt_gimp_image_id, Mw, Mh, -top_left.x, -top_left.y);
        }
#if GIMP_CHECK_VERSION(2, 99, 12)
        GimpLayer * selected_layer;
        if (outputMode == GmicQt::OutputMode::NewLayers) {
          selected_layer = active_layer_id;
        } else {
          selected_layer = top_layer_id;
        }
        gimp_image_set_selected_layers(gmic_qt_gimp_image_id, 1, (const GimpLayer **)&selected_layer);
#else
        if (outputMode == GmicQt::OutputMode::NewLayers) {
          gimp_image_set_active_layer(gmic_qt_gimp_image_id, active_layer_id);
        } else {
          gimp_image_set_active_layer(gmic_qt_gimp_image_id, top_layer_id);
        }
#endif
      }
      gimp_image_undo_group_end(gmic_qt_gimp_image_id);
    }
  } else if (outputMode == GmicQt::OutputMode::NewImage && images.size()) {
#if GIMP_CHECK_VERSION(2, 99, 12)
    GList * selected_layers = gimp_image_list_selected_layers(gmic_qt_gimp_image_id);
    GList * first = g_list_first(selected_layers);
    _GimpLayerPtr active_layer_id = (_GimpLayerPtr)first->data;
#else
    _GimpLayerPtr active_layer_id = gimp_image_get_active_layer(gmic_qt_gimp_image_id);
#endif
    const unsigned int max_width = (unsigned int)bottom_right.x;
    const unsigned int max_height = (unsigned int)bottom_right.y;
    if (_GIMP_LAYER_IS_NOT_NULL(active_layer_id)) {
#if GIMP_VERSION_LTE(2, 8)
      _GimpImagePtr nimage_id = gimp_image_new(max_width, max_height, max_spectrum <= 2 ? GIMP_GRAY : GIMP_RGB);
#else
      _GimpImagePtr nimage_id = gimp_image_new_with_precision(max_width, max_height, max_spectrum <= 2 ? GIMP_GRAY : GIMP_RGB, gimp_image_get_precision(gmic_qt_gimp_image_id));
#endif
      gimp_image_undo_group_start(nimage_id);
      for (unsigned int p = 0; p < images.size(); ++p) {
        if (images[p]) {
          layer_blendmode = GIMP_NORMAL_MODE;
          layer_opacity = 100;
          layer_posx = layer_posy = 0;
          if (inputLayers.size() == 1) {
            if (!is_selection) {
              _gimp_drawable_get_offsets(_GIMP_DRAWABLE(active_layer_id), &layer_posx, &layer_posy);
            }
            gmic_library::gmic_image<char>::string(gimp_item_get_name(_GIMP_ITEM(active_layer_id))).move_to(layer_name);
          } else {
            layer_name.assign();
          }
          get_output_layer_props(imageNames[p], layer_blendmode, layer_opacity, layer_posx, layer_posy, layer_name);
          if (is_selection) {
            layer_posx = 0;
            layer_posy = 0;
          }
          gmic_library::gmic_image<gmic_pixel_type> & img = images[p];
          if (_gimp_image_get_base_type(nimage_id) != GIMP_GRAY) {
            GmicQt::calibrateImage(img, (img.spectrum() == 1 || img.spectrum() == 3) ? 3 : 4, false);
          }
          _GimpLayerPtr layer_id = gimp_layer_new(nimage_id, nullptr, img.width(), img.height(), spectrum2gimpImageTypes[std::min(img.spectrum(), 4)], layer_opacity, layer_blendmode);
          gimp_layer_set_offsets(layer_id, layer_posx, layer_posy);
          if (layer_name) {
            _GIMP_ITEM_SET_NAME(_GIMP_ITEM(layer_id), layer_name);
          }
          gimp_image_insert_layer(nimage_id, layer_id, _GIMP_NULL_LAYER, p);

#if GIMP_VERSION_LTE(2, 8)
          GimpDrawable * drawable = gimp_drawable_get(layer_id);
          GimpPixelRgn region;
          gimp_pixel_rgn_init(&region, drawable, 0, 0, drawable->width, drawable->height, true, true);
          image2uchar(img);
          gimp_pixel_rgn_set_rect(&region, (guchar *)img.data(), 0, 0, img.width(), img.height());
          gimp_drawable_flush(drawable);
          gimp_drawable_merge_shadow(layer_id, true);
          gimp_drawable_update(layer_id, 0, 0, drawable->width, drawable->height);
          gimp_drawable_detach(drawable);
#else
          GeglBuffer * buffer = gimp_drawable_get_shadow_buffer(_GIMP_DRAWABLE(layer_id));
          const char * const format = img.spectrum() == 1 ? "Y' float" : img.spectrum() == 2 ? "Y'A float" : img.spectrum() == 3 ? "R'G'B' float" : "R'G'B'A float";
          (img /= 255).permute_axes("cxyz");
          gegl_buffer_set(buffer, NULL, 0, babl_format(format), img.data(), 0);
          g_object_unref(buffer);
          gimp_drawable_merge_shadow(_GIMP_DRAWABLE(layer_id), true);
          gimp_drawable_update(_GIMP_DRAWABLE(layer_id), 0, 0, img.width(), img.height());
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

} // namespace GmicQtHost

#if GIMP_VERSION_LTE(2, 98)
/*
 * 'Run' function, required by the GIMP plug-in API.
 */
void gmic_qt_run(const gchar * /* name */, gint /* nparams */, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals)
{
#if (GIMP_MAJOR_VERSION == 2 && GIMP_MINOR_VERSION > 8) || (GIMP_MAJOR_VERSION >= 3)
  gegl_init(nullptr, nullptr);
  gimp_plugin_enable_precision();
#endif

  GmicQt::RunParameters pluginParameters;
  bool accepted = true;
  static GimpParam return_values[1];
  *return_vals = return_values;
  *nreturn_vals = 1;
  return_values[0].type = GIMP_PDB_STATUS;
  int run_mode = (GimpRunMode)param[0].data.d_int32;
  switch (run_mode) {
  case GIMP_RUN_INTERACTIVE:
    gmic_qt_gimp_image_id = param[1].data.d_drawable;
    GmicQt::run(GmicQt::UserInterfaceMode::Full, //
                GmicQt::RunParameters(),         //
                std::list<GmicQt::InputMode>(),  //
                std::list<GmicQt::OutputMode>(), //
                &accepted);
    break;
  case GIMP_RUN_WITH_LAST_VALS:
    gmic_qt_gimp_image_id = param[1].data.d_drawable;
    GmicQt::run(GmicQt::UserInterfaceMode::ProgressDialog,                                                       //
                GmicQt::lastAppliedFilterRunParameters(GmicQt::ReturnedRunParametersFlag::AfterFilterExecution), //
                std::list<GmicQt::InputMode>(),                                                                  //
                std::list<GmicQt::OutputMode>(),                                                                 //
                &accepted);
    break;
  case GIMP_RUN_NONINTERACTIVE:
    gmic_qt_gimp_image_id = param[1].data.d_drawable;
    pluginParameters.command = param[5].data.d_string;
    pluginParameters.inputMode = static_cast<GmicQt::InputMode>(param[3].data.d_int32 + (int)GmicQt::InputMode::NoInput);
    pluginParameters.outputMode = static_cast<GmicQt::OutputMode>(param[4].data.d_int32 + (int)GmicQt::OutputMode::InPlace);
    run(GmicQt::UserInterfaceMode::Silent, //
        pluginParameters,                  //
        std::list<GmicQt::InputMode>(),    //
        std::list<GmicQt::OutputMode>(),   //
        &accepted);
    break;
  }
  return_values[0].data.d_status = accepted ? GIMP_PDB_SUCCESS : GIMP_PDB_CANCEL;
}

void gmic_qt_query()
{
  static const GimpParamDef args[] = {{GIMP_PDB_INT32, (gchar *)"run_mode", (gchar *)"Interactive, non-interactive"},
                                      {GIMP_PDB_IMAGE, (gchar *)"image", (gchar *)"Input image"},
                                      {GIMP_PDB_DRAWABLE, (gchar *)"drawable", (gchar *)"Input drawable (unused)"},
                                      {GIMP_PDB_INT32, (gchar *)"input",
                                       (gchar *)"Input layers mode, when non-interactive"
                                                " (0=none, 1=active, 2=all, 3=active & below, 4=active & above, 5=all visibles, 6=all invisibles)"},
                                      {GIMP_PDB_INT32, (gchar *)"output",
                                       (gchar *)"Output mode, when non-interactive "
                                                "(0=in place,1=new layers,2=new active layers,3=new image)"},
                                      {GIMP_PDB_STRING, (gchar *)"command", (gchar *)"G'MIC command string, when non-interactive"}};

  const char name[] = "plug-in-gmic-qt";
  QByteArray blurb = QString("G'MIC-Qt (%1)").arg(GmicQt::gmicVersionString()).toLatin1();
  QByteArray path("G'MIC-Qt...");
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
                         nullptr);                  // return_vals
  gimp_plugin_menu_register(name, "<Image>/Filters");
}

GimpPlugInInfo PLUG_IN_INFO = {nullptr, nullptr, gmic_qt_query, gmic_qt_run};

MAIN()

#else

static GList * gmic_qt_query(GimpPlugIn * plug_in)
{
  return g_list_append(NULL, g_strdup(PLUG_IN_PROC));
}

/*
 * 'Run' function, required by the GIMP plug-in API.
 */
#if (GIMP_MAJOR_VERSION <= 2) && (GIMP_MINOR_VERSION <= 99) && (GIMP_MICRO_VERSION < 6)
static GimpValueArray * gmic_qt_run(GimpProcedure * procedure, GimpRunMode run_mode, GimpImage * image, GimpDrawable * drawable, const GimpValueArray * args, gpointer run_data)
#else
static GimpValueArray * gmic_qt_run(GimpProcedure * procedure, GimpRunMode run_mode, GimpImage * image, gint n_drawables, GimpDrawable ** drawables, const GimpValueArray * args, gpointer run_data)
#endif
{
  gegl_init(NULL, NULL);
  // gimp_plugin_enable_precision(); // what is this?
  GmicQt::RunParameters pluginParameters;
  bool accepted = true;
  switch (run_mode) {
  case GIMP_RUN_INTERACTIVE:
    gmic_qt_gimp_image_id = image;
    GmicQt::run(GmicQt::UserInterfaceMode::Full,
                GmicQt::RunParameters(),         //
                std::list<GmicQt::InputMode>(),  //
                std::list<GmicQt::OutputMode>(), //
                &accepted);
    break;
  case GIMP_RUN_WITH_LAST_VALS:
    gmic_qt_gimp_image_id = image;
    GmicQt::run(GmicQt::UserInterfaceMode::ProgressDialog,                                                       //
                GmicQt::lastAppliedFilterRunParameters(GmicQt::ReturnedRunParametersFlag::AfterFilterExecution), //
                std::list<GmicQt::InputMode>(),                                                                  //
                std::list<GmicQt::OutputMode>(),                                                                 //
                &accepted);
    break;
  case GIMP_RUN_NONINTERACTIVE:
    gmic_qt_gimp_image_id = image;
    pluginParameters.command = g_value_get_string(gimp_value_array_index(args, 2));
    pluginParameters.inputMode = static_cast<GmicQt::InputMode>(g_value_get_int(gimp_value_array_index(args, 0)) + (int)GmicQt::InputMode::NoInput);
    pluginParameters.outputMode = static_cast<GmicQt::OutputMode>(g_value_get_int(gimp_value_array_index(args, 1)) + (int)GmicQt::OutputMode::InPlace);
    GmicQt::run(GmicQt::UserInterfaceMode::Silent, //
                pluginParameters,                  //
                std::list<GmicQt::InputMode>(),    //
                std::list<GmicQt::OutputMode>(),   //
                &accepted);
    break;
  }
  return gimp_procedure_new_return_values(procedure, accepted ? GIMP_PDB_SUCCESS : GIMP_PDB_CANCEL, NULL);
}

static GimpProcedure * gmic_qt_create_procedure(GimpPlugIn * plug_in, const gchar * name)
{
  GimpProcedure * procedure = NULL;

  if (strcmp(name, PLUG_IN_PROC) == 0) {
    procedure = gimp_image_procedure_new(plug_in, name, GIMP_PDB_PROC_TYPE_PLUGIN, gmic_qt_run, NULL, NULL);

    gimp_procedure_set_image_types(procedure, "RGB*, GRAY*");

    QByteArray path("G'MIC-Qt...");
    path.prepend("_");
    gimp_procedure_set_menu_label(procedure, path.constData());
    gimp_procedure_add_menu_path(procedure, "<Image>/Filters");

    QByteArray blurb = QString("G'MIC-Qt (%1)").arg(GmicQt::gmicVersionString()).toLatin1();
    gimp_procedure_set_documentation(procedure,
                                     blurb.constData(), // blurb
                                     blurb.constData(), // help
                                     name);             // help_id
    gimp_procedure_set_attribution(procedure,
                                   "S\303\251bastien Fourey", // author
                                   "S\303\251bastien Fourey", // copyright
                                   "2017");                   // date

    GIMP_PROC_ARG_INT(procedure,
                      "input",                                                                                                                                   // name
                      "input",                                                                                                                                   // nick
                      "Input layers mode, when non-interactive (0=none, 1=active, 2=all, 3=active & below, 4=active & above, 5=all visibles, 6=all invisibles)", // blurb
                      0,                                                                                                                                         // min
                      6,                                                                                                                                         // max
                      0,                                                                                                                                         // default
                      G_PARAM_READWRITE);                                                                                                                        // flags

    GIMP_PROC_ARG_INT(procedure,
                      "output",                                                                                      // name
                      "output",                                                                                      // nick
                      "Output mode, when non-interactive (0=in place,1=new layers,2=new active layers,3=new image)", // blurb
                      0,                                                                                             // min
                      3,                                                                                             // max
                      0,                                                                                             // default
                      G_PARAM_READWRITE);                                                                            // flags

    GIMP_PROC_ARG_STRING(procedure,
                         "command",                                    // name
                         "command",                                    // nick
                         "G'MIC command string, when non-interactive", // blurb
                         "",                                           // default
                         G_PARAM_READWRITE);                           // flags
  }

  return procedure;
}
#endif

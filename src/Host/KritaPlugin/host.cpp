/*
 * Copyright (C) 2017 Boudewijn Rempt <boud@valdyas.org>
 * Copyright (C) 2020 L. E. Segovia <amy@amyspark.me>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <ImageConverter.h>
#include <QByteArray>
#include <QDebug>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QSharedMemory>
#include <QStandardPaths>
#include <QUuid>
#include <algorithm>
#include <memory>

#include "Host/host.h"
#include "gmic.h"
#include "gmic_qt.h"
#include "kis_qmic_interface.h"

/*
 * No messages are sent in the plugin version of GMic.
 * Instead, a list of KisQMicImageSP (shared pointers to KisQMic instances)
 * are sent. These have:
 * 
 * layer name
 * shared pointer to data
 * width
 * height
 * a mutex to control access.
 * 
 * For the sake of debuggability, the overall control flow has been maintained.
 */

namespace GmicQt {
const QString HostApplicationName = QString("Krita");
const char *HostApplicationShortname = GMIC_QT_XSTRINGIFY(GMIC_HOST);
const bool DarkThemeIsDefault = false;
} // namespace GmicQt

QVector<KisQMicImageSP> sharedMemorySegments;
std::shared_ptr<KisImageInterface> iface;

void gmic_qt_get_layers_extent(int *width, int *height,
                               GmicQt::InputMode mode) {
  auto size = iface->gmic_qt_get_image_size();
  *width = size.width();
  *height = size.height();

  // qDebug() << "gmic-qt: layers extent:" << *width << *height;
}

void gmic_qt_get_cropped_images(gmic_list<float> &images,
                                gmic_list<char> &imageNames, double x, double y,
                                double width, double height,
                                GmicQt::InputMode mode) {

  // qDebug() << "gmic-qt: get_cropped_images:" << x << y << width << height;

  const bool entireImage = x < 0 && y < 0 && width < 0 && height < 0;
  if (entireImage) {
    x = 0.0;
    y = 0.0;
    width = 1.0;
    height = 1.0;
  }

  // Create a message for Krita
  QRectF cropRect = {x, y, width, height};
  auto imagesList = iface->gmic_qt_get_cropped_images(mode, cropRect);

  if (imagesList.isEmpty()) {
    qWarning() << "\tgmic-qt: empty answer!";
    return;
  }

  // qDebug() << "\tgmic-qt: " << answer;

  images.assign(imagesList.size());
  imageNames.assign(imagesList.size());

  // qDebug() << "\tgmic-qt: imagelist size" << imagesList.size();


  // Get the layers as prepared by Krita in G'Mic format
  for (int i = 0; i < imagesList.length(); ++i) {
    const auto &layer = imagesList[i];
    QByteArray ba = layer->m_layerName.toUtf8().toHex();
    gmic_image<char>::string(ba.constData()).move_to(imageNames[i]);

    // Fill images from the shared memory areas

    {
      QMutexLocker(&layer->m_mutex);

      // qDebug() << "Memory segment" << (quintptr)image.data() << image->size() << (quintptr)&image->m_data << (quintptr)image->m_data.get();

      // convert the data to the list of float
      gmic_image<float> gimg;
      gimg.assign(layer->m_width, layer->m_height, 1, 4);
      memcpy(gimg._data, layer->constData(),
             layer->m_width * layer->m_height * 4 * sizeof(float));
      gimg.move_to(images[i]);
    }
  }

  iface->gmic_qt_detach();

  // qDebug() << "\tgmic-qt:  Images size" << images.size() << ", names size" <<
  // imageNames.size();
}

void gmic_qt_output_images(gmic_list<float> &images,
                           const gmic_list<char> &imageNames,
                           GmicQt::OutputMode mode,
                           const char * /*verboseLayersLabel*/) {

  // qDebug() << "qmic-qt-output-images";

  sharedMemorySegments.clear();

  // qDebug() << "\tqmic-qt: shared memory" << sharedMemorySegments.count();

  // Create qsharedmemory segments for each image
  // Create a message for Krita based on mode, the keys of the qsharedmemory
  // segments and the imageNames
  QVector<KisQMicImageSP> layers;

  for (uint i = 0; i < images.size(); ++i) {

    // qDebug() << "\tgmic-qt: image number" << i;

    gmic_image<float> gimg = images.at(i);

    QString layerName((const char *)imageNames[i]);

    KisQMicImageSP m = KisQMicImageSP::create(layerName, gimg._width, gimg._height, gimg._spectrum);
    sharedMemorySegments << m;

    {
      QMutexLocker lock(&m->m_mutex);

      memcpy(m->m_data, gimg._data,
           gimg._width * gimg._height * gimg._spectrum * sizeof(float));
    }

    layers << m;
  }

  iface->gmic_qt_output_images(mode, layers);
}

void gmic_qt_show_message(const char *) {
  // May be left empty for Krita.
  // Only used by launchPluginHeadless(), called in the non-interactive
  // script mode of GIMP.
}

void gmic_qt_apply_color_profile(cimg_library::CImg<gmic_pixel_type> &) {}

/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file host_none.cpp
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
#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QMessageBox>
#include <QPainter>
#include <QRegularExpression>
#include <algorithm>
#include <iostream>
#include "Common.h"
#include "Host/None/ImageDialog.h"
#include "Host/host.h"
#include "ImageConverter.h"
#include "MainWindow.h"
#include "gmic_qt.h"
#include "gmic.h"

#ifdef _GMIC_QT_DEBUG_
//#define DEFAULT_IMAGE "local/img_3820.png"
//#define DEFAULT_IMAGE "local/Untitled.png"
//#define DEFAULT_IMAGE "local/space-shuttle.png"
//#define DEFAULT_IMAGE "local/space-shuttle-transp.png"
//#define DEFAULT_IMAGE "local/bug.jpg"
//#define DEFAULT_IMAGE "local/bug2.jpg"
#define DEFAULT_IMAGE "local/crop_inktober.jpg"
//#define DEFAULT_IMAGE "local/lena.png"
//#define DEFAULT_IMAGE "local/lena_border.png"
//#define DEFAULT_IMAGE "local/transp.png"
//#define DEFAULT_IMAGE "local/small_lena.png"
//#define DEFAULT_IMAGE "local/ken.jpg"
//#define DEFAULT_IMAGE "local/ferrari.jpg"
//#define DEFAULT_IMAGE "local/audio-speakers.png"
//#define DEFAULT_IMAGE "local/audio-speakers-top.png"
#endif

#define STRINGIFY(X) #X
#define XSTRINGIFY(X) STRINGIFY(X)

namespace gmic_qt_standalone
{
QImage input_image;
QString image_filename;
QWidget * visibleMainWindow()
{
  for (QWidget * w : QApplication::topLevelWidgets()) {
    if ((typeid(*w) == typeid(MainWindow)) && (w->isVisible())) {
      return w;
    }
  }
  return nullptr;
}
void askForImageFilename()
{
  QWidget * mainWidget = visibleMainWindow();
  Q_ASSERT_X(mainWidget, __PRETTY_FUNCTION__, "No top level window yet");
  QString filename = QFileDialog::getOpenFileName(mainWidget, QObject::tr("Select an image to open..."), ".", QObject::tr("PNG & JPG files (*.png *.jpeg *.jpg *.PNG *.JPEG *.JPG)"), nullptr);
  if (!filename.isEmpty() && QFileInfo(filename).isReadable() && input_image.load(filename)) {
    input_image = input_image.convertToFormat(QImage::Format_ARGB32);
    image_filename = QFileInfo(filename).fileName();
  } else {
    if (!filename.isEmpty()) {
      QMessageBox::warning(mainWidget, QObject::tr("Error"), QObject::tr("Could not open file."));
    }
    input_image.load(":/resources/gmicky.png");
    input_image = input_image.convertToFormat(QImage::Format_ARGB32);
    image_filename = QObject::tr("Default image");
  }
}
const QImage & transparentImage()
{
  static QImage image;
  if (image.isNull()) {
    image = QImage(640, 480, QImage::Format_ARGB32);
    image.fill(QColor(0, 0, 0, 0));
  }
  return image;
}
} // namespace gmic_qt_standalone

namespace GmicQt
{
const QString HostApplicationName;
const char * HostApplicationShortname = XSTRINGIFY(GMIC_HOST);
const bool DarkThemeIsDefault = false;
} // namespace GmicQt

void gmic_qt_get_image_size(int * width, int * height)
{
  // TSHOW(gmic_qt_standalone::visibleMainWindow());
  if (gmic_qt_standalone::input_image.isNull()) {
    if (gmic_qt_standalone::visibleMainWindow()) {
      gmic_qt_standalone::askForImageFilename();
      *width = gmic_qt_standalone::input_image.width();
      *height = gmic_qt_standalone::input_image.height();
    } else {
      *width = 640;
      *height = 480;
    }
  } else {
    *width = gmic_qt_standalone::input_image.width();
    *height = gmic_qt_standalone::input_image.height();
  }
}

void gmic_qt_get_layers_extent(int * width, int * height, GmicQt::InputMode)
{
  gmic_qt_get_image_size(width, height);
}

void gmic_qt_get_cropped_images(gmic_list<float> & images, gmic_list<char> & imageNames, double x, double y, double width, double height, GmicQt::InputMode mode)
{
  const QImage & input_image = gmic_qt_standalone::input_image.isNull() ? gmic_qt_standalone::transparentImage() : gmic_qt_standalone::input_image;

  const bool entireImage = x < 0 && y < 0 && width < 0 && height < 0;
  if (entireImage) {
    x = 0.0;
    y = 0.0;
    width = 1.0;
    height = 1.0;
  }
  if (mode == GmicQt::NoInput) {
    images.assign();
    imageNames.assign();
    return;
  }

  images.assign(1);
  imageNames.assign(1);

  QString noParenthesisName(gmic_qt_standalone::image_filename);
  noParenthesisName.replace(QChar('('), QChar(21)).replace(QChar(')'), QChar(22));

  QString name = QString("pos(0,0),name(%1)").arg(noParenthesisName);
  QByteArray ba = name.toUtf8();
  gmic_image<char>::string(ba.constData()).move_to(imageNames[0]);

  const int ix = static_cast<int>(entireImage ? 0 : std::floor(x * input_image.width()));
  const int iy = static_cast<int>(entireImage ? 0 : std::floor(y * input_image.height()));
  const int iw = entireImage ? input_image.width() : std::min(input_image.width() - ix, static_cast<int>(1 + std::ceil(width * input_image.width())));
  const int ih = entireImage ? input_image.height() : std::min(input_image.height() - iy, static_cast<int>(1 + std::ceil(height * input_image.height())));
  ImageConverter::convert(input_image.copy(ix, iy, iw, ih), images[0]);
}

void gmic_qt_output_images(gmic_list<float> & images, const gmic_list<char> & imageNames, GmicQt::OutputMode mode, const char * verboseLayersLabel)
{
  if (images.size() > 0) {
    ImageDialog * dialog = new ImageDialog(QApplication::topLevelWidgets().at(0));
    for (unsigned int i = 0; i < images.size(); ++i) {
      QString name((const char *)imageNames[i]);
      name.replace(QRegularExpression(".*name\\("), "");
      int pos = 0;
      int open = 1;
      int len = name.size();
      while (pos < len && open > 0) {
        if (name[pos] == '(') {
          ++open;
        } else if (name[pos] == ')') {
          --open;
        }
        ++pos;
      }
      name.remove(pos ? (pos - 1) : pos, len);
      dialog->addImage(images[i], name);
    }
    dialog->exec();
    gmic_qt_standalone::input_image = dialog->currentImage();
    gmic_qt_standalone::image_filename = QString((const char *)imageNames[dialog->currentImageIndex()]);
    delete dialog;
  }
  unused(mode);
  unused(verboseLayersLabel);
}

void gmic_qt_show_message(const char * message)
{
  std::cout << message << std::endl;
}

int main(int argc, char * argv[])
{
  TIMING;
  QString filename;
  if (argc == 2) {
    filename = argv[1];
  }
#ifdef DEFAULT_IMAGE
  if (filename.isEmpty() && QFileInfo(DEFAULT_IMAGE).isReadable()) {
    filename = DEFAULT_IMAGE;
  }
#endif
  if (filename.isEmpty()) {
    return launchPlugin();
  }
  if (QFileInfo(filename).isReadable() && gmic_qt_standalone::input_image.load(filename)) {
    gmic_qt_standalone::input_image = gmic_qt_standalone::input_image.convertToFormat(QImage::Format_ARGB32);
    gmic_qt_standalone::image_filename = QFileInfo(filename).fileName();
    return launchPlugin();
  }
  std::cerr << "Could not open file " << filename.toLocal8Bit().constData() << "\n";
  return 1;
}

void gmic_qt_apply_color_profile(cimg_library::CImg<gmic_pixel_type> &) {}

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
#include <QCommandLineParser>
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
#include "Misc.h"
#include "gmic_qt.h"
#include "gmic.h"

#ifdef _GMIC_QT_DEBUG_
#define DEFAULT_IMAGE "local/default.png"
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
QString imageName(const char * text)
{
  QString result;
  const char * start = std::strstr(text, "name(");
  if (start) {
    const char * ps = start + 5;
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
      result = QString::fromUtf8(start + 5, (unsigned int)(ps - start - 5)).chopped(1);
      for (QChar & c : result) {
        if (c == 21) {
          c = '(';
        } else if (c == 22) {
          c = ')';
        }
      }
    }
  }
  return result;
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

void gmic_qt_output_images(gmic_list<float> & images, const gmic_list<char> & imageNames, GmicQt::OutputMode mode)
{
  if (images.size() > 0) {
    QWidgetList widgets = QApplication::topLevelWidgets();
    if (widgets.size()) {
      ImageDialog * dialog = new ImageDialog(widgets.at(0));
      for (unsigned int i = 0; i < images.size(); ++i) {
        QString name = gmic_qt_standalone::imageName((const char *)imageNames[i]);
        dialog->addImage(images[i], name);
      }
      dialog->exec();
      gmic_qt_standalone::input_image = dialog->currentImage();
      gmic_qt_standalone::image_filename = gmic_qt_standalone::imageName((const char *)imageNames[dialog->currentImageIndex()]);
      delete dialog;
    }
  }
  unused(mode);
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

  GmicQt::PluginParameters parameters;
  std::list<GmicQt::InputMode> disabledInputModes;
  disabledInputModes.push_back(GmicQt::NoInput);
  // disabledInputModes.push_back(GmicQt::Active);
  disabledInputModes.push_back(GmicQt::All);
  disabledInputModes.push_back(GmicQt::ActiveAndBelow);
  disabledInputModes.push_back(GmicQt::ActiveAndAbove);
  disabledInputModes.push_back(GmicQt::AllVisible);
  disabledInputModes.push_back(GmicQt::AllInvisible);

  std::list<GmicQt::OutputMode> disabledOutputModes;
  // disabledOutputModes.push_back(GmicQt::InPlace);
  disabledOutputModes.push_back(GmicQt::NewImage);
  disabledOutputModes.push_back(GmicQt::NewLayers);
  disabledOutputModes.push_back(GmicQt::NewActiveLayers);

  if (filename.isEmpty()) {
    return launchPlugin(GmicQt::FullGUI, parameters, disabledInputModes, disabledOutputModes);
  }
  if (QFileInfo(filename).isReadable() && gmic_qt_standalone::input_image.load(filename)) {
    gmic_qt_standalone::input_image = gmic_qt_standalone::input_image.convertToFormat(QImage::Format_ARGB32);
    gmic_qt_standalone::image_filename = QFileInfo(filename).fileName();
    GmicQt::PluginParameters parameters;
    int status;

    if (true) {
      //  parameters.filterPath = "/Artistic/Cartoon";
      //  parameters.command = "cartoon 3,200,20,0.25,1.5,8,0,50,50";
      //  parameters.filterPath = "/Artistic/Cutout";
      // parameters.inputMode = GmicQt::Active;
      // parameters.outputMode = GmicQt::InPlace;
      // parameters = GmicQt::lastAppliedFilterPluginParameters(GmicQt::BeforeFilterExecution);
      parameters.filterPath = "00Toto";
      parameters.filterPath = "Black & White";
      // parameters.command = "fx_toto";
      status = GmicQt::launchPlugin(GmicQt::ProgressDialogGUI, parameters);
      //      status = GmicQt::launchPlugin(GmicQt::FullGUI, parameters);
    } else {
      // parameters.command = "fx_toto 10,20,\"Je suis Sebastien\"";
      parameters.filterPath = "Relief Light";
      status = GmicQt::launchPlugin(GmicQt::FullGUI, parameters);
    }
    parameters = GmicQt::lastAppliedFilterPluginParameters(GmicQt::AfterFilterExecution);
    return status;
  }
  std::cerr << "Could not open file " << filename.toLocal8Bit().constData() << "\n";
  return 1;
}

void gmic_qt_apply_color_profile(cimg_library::CImg<gmic_pixel_type> &) {}

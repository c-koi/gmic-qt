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
#include <fstream>
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

namespace
{

namespace gmic_qt_standalone
{
QImage input_image;
QString input_image_filename;
QString output_image_filename;
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
  QStringList extensions;
  QString filters;
  ImageDialog::supportedImageFormats(extensions, filters);
  QString filename = QFileDialog::getOpenFileName(mainWidget, QObject::tr("Select an image to open..."), ".", filters, nullptr);
  if (!filename.isEmpty() && QFileInfo(filename).isReadable() && input_image.load(filename)) {
    input_image = input_image.convertToFormat(QImage::Format_ARGB32);
    input_image_filename = QFileInfo(filename).fileName();
  } else {
    if (!filename.isEmpty()) {
      QMessageBox::warning(mainWidget, QObject::tr("Error"), QObject::tr("Could not open file."));
    }
    input_image.load(":/resources/gmicky.png");
    input_image = input_image.convertToFormat(QImage::Format_ARGB32);
    input_image_filename = QObject::tr("Default image");
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

std::string basename(const std::string & text)
{
  std::string::size_type position = text.rfind('/');
  if (position == std::string::npos) {
    return text;
  } else {
    return text.substr(position + 1, text.size() - (position + 1));
  }
}
} // namespace gmic_qt_standalone
} // namespace

namespace GmicQt
{
const QString HostApplicationName;
const char * HostApplicationShortname = XSTRINGIFY(GMIC_HOST);
const bool DarkThemeIsDefault = false;
} // namespace GmicQt

void gmic_qt_get_image_size(int * width, int * height)
{
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

  QString noParenthesisName(gmic_qt_standalone::input_image_filename);
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
    if (gmic_qt_standalone::output_image_filename.isEmpty()) {
      QWidgetList widgets = QApplication::topLevelWidgets();
      if (widgets.size()) {
        ImageDialog * dialog = new ImageDialog(widgets.at(0));
        for (unsigned int i = 0; i < images.size(); ++i) {
          QString name = gmic_qt_standalone::imageName((const char *)imageNames[i]);
          dialog->addImage(images[i], name);
        }
        dialog->exec();
        gmic_qt_standalone::input_image = dialog->currentImage();
        gmic_qt_standalone::input_image_filename = gmic_qt_standalone::imageName((const char *)imageNames[dialog->currentImageIndex()]);
        delete dialog;
      }
    } else {
      ImageConverter::convert(images[0], gmic_qt_standalone::input_image);
      std::cout << "[gmic_qt] Writing output file " << gmic_qt_standalone::output_image_filename.toStdString() << std::endl;
      gmic_qt_standalone::input_image.save(gmic_qt_standalone::output_image_filename);
      gmic_qt_standalone::input_image_filename = gmic_qt_standalone::imageName((const char *)imageNames[0]);
    }
  }
  unused(mode);
}

void gmic_qt_show_message(const char * message)
{
  std::cout << message << std::endl;
}

void usage(const std::string & argv0)
{
  std::cout << QObject::tr("Usage: %1 [OPTIONS]... [INPUT_FILE]").arg(QString::fromStdString(argv0)).toStdString() << std::endl;
  std::cout << QObject::tr("Launch the G'MIC-Qt plugin as a standalone application.\n"
                           "\n"
                           "Options:\n"
                           "                             --help : Display this help\n"
                           "                      --output FILE : Write output image to FILE\n"
                           "                           --repeat : Use last applied filter and parameters\n"
                           "   --path FILTER_PATH | FILTER_NAME : Select filter\n"
                           "                                      e.g. \"/Black & White/Charcoal\"\n"
                           "                                           \"Charcoal\"\n")
                   .toStdString();
}

int main(int argc, char * argv[])
{
  TIMING;
  QString filename;

  int narg = 1;
  bool repeat = false;
  std::string filterPath;
  std::string command;
  while (narg < argc) {
    std::string arg(argv[narg]);
    if (arg == "--help") {
      usage(gmic_qt_standalone::basename(argv[0]));
      return EXIT_SUCCESS;
    } else if (arg == "--output") {
      if (narg < argc - 1) {
        ++narg;
        gmic_qt_standalone::output_image_filename = argv[narg];
      } else {
        std::cerr << QObject::tr("Missing filename for option --output").toStdString() << std::endl;
        return EXIT_FAILURE;
      }
    } else if (arg == "--repeat") {
      repeat = true;
    } else if (arg == "--command") {
      if (narg < argc - 1) {
        ++narg;
        command = argv[narg];
      } else {
        std::cerr << QObject::tr("Missing command for option --command").toStdString() << std::endl;
        return EXIT_FAILURE;
      }
    } else if (arg == "--path") {
      if (narg < argc - 1) {
        ++narg;
        filterPath = argv[narg];
      } else {
        std::cerr << QObject::tr("Missing path for option --path").toStdString() << std::endl;
        return EXIT_FAILURE;
      }
    } else if (narg == argc - 1) {
      if (QFileInfo(argv[narg]).isReadable()) {
        filename = QString::fromStdString(argv[narg]);
      }
    }
    ++narg;
  }

#ifdef DEFAULT_IMAGE
  if (filename.isEmpty() && QFileInfo(DEFAULT_IMAGE).isReadable()) {
    filename = DEFAULT_IMAGE;
  }
#endif

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

  GmicQt::PluginParameters parameters;
  if (repeat) {
    parameters = GmicQt::lastAppliedFilterPluginParameters(GmicQt::AfterFilterExecution);
    std::cout << "[gmic_qt] Running with last parameters...\n";
    std::cout << "Command: " << parameters.command << std::endl;
    std::cout << "Path: " << parameters.filterPath << std::endl;
    std::cout << "Input Mode: " << parameters.inputMode << std::endl;
    std::cout << "Output Mode: " << parameters.outputMode << std::endl;
  } else {
    parameters.filterPath = filterPath;
    parameters.command = command;
  }

  if (filename.isEmpty()) {
    return launchPlugin(GmicQt::FullGUI, parameters, disabledInputModes, disabledOutputModes);
  }
  if (QFileInfo(filename).isReadable() && gmic_qt_standalone::input_image.load(filename)) {
    gmic_qt_standalone::input_image = gmic_qt_standalone::input_image.convertToFormat(QImage::Format_ARGB32);
    gmic_qt_standalone::input_image_filename = QFileInfo(filename).fileName();
    int status = GmicQt::launchPlugin(GmicQt::FullGUI, parameters);
    return status;
  }
  std::cerr << "Could not open file " << filename.toLocal8Bit().constData() << "\n";
  return 1;
}

void gmic_qt_apply_color_profile(cimg_library::CImg<gmic_pixel_type> &) {}

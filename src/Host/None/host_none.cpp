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
#include <QGuiApplication>
#include <QMainWindow>
#include <QMessageBox>
#include <QPainter>
#include <QRegularExpression>
#include <algorithm>
#include <fstream>
#include <iostream>
#include "GmicQt.h"
#include "Host/GmicQtHost.h"
#include "Host/None/ImageDialog.h"
#include "gmic.h"

#define STRINGIFY(X) #X
#define XSTRINGIFY(X) STRINGIFY(X)

namespace gmic_qt_standalone
{

QImage input_image;
QString current_image_filename;
QString input_image_filename;
QString output_image_filename;
int jpeg_quality = -1;

QWidget * visibleMainWindow()
{
  for (QWidget * w : QApplication::topLevelWidgets()) {
    if ((dynamic_cast<QMainWindow *>(w) != nullptr) && (w->isVisible())) {
      return w;
    }
  }
  return nullptr;
}

void askForInputImageFilename()
{
  QWidget * mainWidget = visibleMainWindow();
  Q_ASSERT_X(mainWidget, __PRETTY_FUNCTION__, "No top level window yet");
  QStringList extensions;
  QString filters;
  gmic_qt_standalone::ImageDialog::supportedImageFormats(extensions, filters);
  QString filename = QFileDialog::getOpenFileName(mainWidget, QObject::tr("Select an image to open..."), ".", filters, nullptr);
  if (!filename.isEmpty() && QFileInfo(filename).isReadable() && input_image.load(filename)) {
    input_image = input_image.convertToFormat(QImage::Format_ARGB32);
    current_image_filename = QFileInfo(filename).fileName();
  } else {
    if (!filename.isEmpty()) {
      QMessageBox::warning(mainWidget, QObject::tr("Error"), QObject::tr("Could not open file."));
    }
    input_image.load(":/resources/gmicky.png");
    input_image = input_image.convertToFormat(QImage::Format_ARGB32);
    current_image_filename = QObject::tr("Default image");
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
    if (!level || (*(ps - 1) == ')')) {
      result = QString::fromUtf8(start + 5, (unsigned int)(ps - start - 5));
      result.chop(1);
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

namespace GmicQtHost
{
const QString ApplicationName;
const char * const ApplicationShortname = XSTRINGIFY(GMIC_HOST);
const bool DarkThemeIsDefault = false;

void getLayersExtent(int * width, int * height, GmicQt::InputMode)
{
  if (gmic_qt_standalone::input_image.isNull()) {
    if (gmic_qt_standalone::visibleMainWindow()) {
      gmic_qt_standalone::askForInputImageFilename();
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

void getCroppedImages(gmic_list<float> & images, gmic_list<char> & imageNames, double x, double y, double width, double height, GmicQt::InputMode mode)
{
  const QImage & input_image = gmic_qt_standalone::input_image.isNull() ? gmic_qt_standalone::transparentImage() : gmic_qt_standalone::input_image;

  const bool entireImage = x < 0 && y < 0 && width < 0 && height < 0;
  if (entireImage) {
    x = 0.0;
    y = 0.0;
    width = 1.0;
    height = 1.0;
  }
  if (mode == GmicQt::InputMode::NoInput) {
    images.assign();
    imageNames.assign();
    return;
  }

  images.assign(1);
  imageNames.assign(1);

  QString noParenthesisName(gmic_qt_standalone::current_image_filename);
  noParenthesisName.replace(QChar('('), QChar(21)).replace(QChar(')'), QChar(22));

  QString name = QString("pos(0,0),name(%1)").arg(noParenthesisName);
  QByteArray ba = name.toUtf8();
  gmic_image<char>::string(ba.constData()).move_to(imageNames[0]);

  const int ix = static_cast<int>(entireImage ? 0 : std::floor(x * input_image.width()));
  const int iy = static_cast<int>(entireImage ? 0 : std::floor(y * input_image.height()));
  const int iw = entireImage ? input_image.width() : std::min(input_image.width() - ix, static_cast<int>(1 + std::ceil(width * input_image.width())));
  const int ih = entireImage ? input_image.height() : std::min(input_image.height() - iy, static_cast<int>(1 + std::ceil(height * input_image.height())));
  GmicQt::convertQImageToCImg(input_image.copy(ix, iy, iw, ih), images[0]);
}

void outputImages(gmic_list<float> & images, const gmic_list<char> & imageNames, GmicQt::OutputMode mode)
{
  if (images.size() > 0) {
    if (gmic_qt_standalone::output_image_filename.isEmpty()) {
      QWidgetList widgets = QApplication::topLevelWidgets();
      if (widgets.size()) {
        auto dialog = new gmic_qt_standalone::ImageDialog(widgets.at(0));
        dialog->setJPEGQuality(gmic_qt_standalone::jpeg_quality);
        for (unsigned int i = 0; i < images.size(); ++i) {
          QString name = gmic_qt_standalone::imageName((const char *)imageNames[i]);
          dialog->addImage(images[i], name);
        }
        dialog->exec();
        gmic_qt_standalone::input_image = dialog->currentImage();
        gmic_qt_standalone::current_image_filename = gmic_qt_standalone::imageName((const char *)imageNames[dialog->currentImageIndex()]);
        delete dialog;
      }
    } else {
      GmicQt::convertCImgToQImage(images[0], gmic_qt_standalone::input_image);
      QString outputFilename = gmic_qt_standalone::output_image_filename;
      if (outputFilename.contains("%b")) {
        const QString basename = QFileInfo(gmic_qt_standalone::input_image_filename).completeBaseName();
        outputFilename.replace("%b", basename);
      }
      if (outputFilename.contains("%f")) {
        const QString filename = QFileInfo(gmic_qt_standalone::input_image_filename).fileName();
        outputFilename.replace("%f", filename);
      }
      std::cout << "[gmic_qt] Writing output file " << outputFilename.toStdString() << std::endl;
      gmic_qt_standalone::input_image.save(outputFilename, nullptr, gmic_qt_standalone::jpeg_quality);
      gmic_qt_standalone::current_image_filename = gmic_qt_standalone::imageName((const char *)imageNames[0]);
    }
  }
  unused(mode);
}

void showMessage(const char * message)
{
  std::cout << message << std::endl;
}

void applyColorProfile(cimg_library::CImg<gmic_pixel_type> &) {}

} // namespace GmicQtHost

void usage(const std::string & argv0)
{
  std::cout << "Usage: " << argv0 << " [OPTIONS ...] [INPUT_FILES ...]" << std::endl;
  std::cout << "Launch the G'MIC-Qt plugin as a standalone application.\n"
               "\n"
               "Options:\n"
               "                               -h --help : Display this help\n"
               "                        -o --output FILE : Write output image to FILE\n"
               "                                            %b will be replaced by the input file basename (i.e. without path and extension)\n"
               "                                            %f will be replaced by the input filename (without path)\n"
               "                        -q --quality NNN : JPEG quality of output file(s) in 0..100\n"
               "                             -r --repeat : Use last applied filter and parameters\n"
               "     -p --path FILTER_PATH | FILTER_NAME : Select filter\n"
               "                                           e.g. \"/Black & White/Charcoal\"\n"
               "                                                \"Charcoal\"\n"
               "             -c --command \"GMIC COMMAND\" : Run gmic command. If a filter name or path is provided,\n"
               "                                           then parameters are completed using filter's defaults.\n"
               "                              -a --apply : Apply filter or command and quit (requires one of -r -p -c)\n"
               "                      -R --reapply-first : Launch GUI once for first input file, then apply selected filter\n"
               "                                           and parameters to all other files\n"
               "                             --show-last : Print last applied plugin parameters\n"
               "                       --show-last-after : Print last applied plugin parameters (after filter execution)\n";
}

namespace
{
inline bool loadImage(QImage & image, const QString & filename, int argc, char * argv[])
{
  QGuiApplication app(argc, argv);
  return image.load(filename);
}
} // namespace

int main(int argc, char * argv[])
{
  TIMING;
  int narg = 1;
  bool repeat = false;
  bool apply = false;
  bool reapplyFirst = false;
  bool printLast = false;
  bool printLastAfter = false;
  std::string filterPath;
  std::string command;
  QStringList filenames;
  while (narg < argc) {
    QString arg = QString::fromLocal8Bit(argv[narg]);
    if ((arg == "--help") || (arg == "-h")) {
      usage(gmic_qt_standalone::basename(argv[0]));
      return EXIT_SUCCESS;
    } else if ((arg == "--apply") || (arg == "-a")) {
      apply = true;
    } else if ((arg == "--reapply-first") || (arg == "-R")) {
      reapplyFirst = true;
    } else if ((arg == "--output") || (arg == "-o")) {
      if (narg < argc - 1) {
        ++narg;
        gmic_qt_standalone::output_image_filename = argv[narg];
      } else {
        std::cerr << "Missing filename for option " << arg.toStdString() << std::endl;
        return EXIT_FAILURE;
      }
    } else if ((arg == "--quality") || (arg == "-q")) {
      if (narg < argc - 1) {
        ++narg;
        gmic_qt_standalone::jpeg_quality = std::max(0, std::min(100, atoi(argv[narg])));
      } else {
        std::cerr << "Missing argument for option " << arg.toStdString() << std::endl;
        return EXIT_FAILURE;
      }
    } else if ((arg == "--repeat") || (arg == "-r")) {
      repeat = true;
    } else if ((arg == "--command") || (arg == "-c")) {
      if (narg < argc - 1) {
        ++narg;
        command = argv[narg];
      } else {
        std::cerr << "Missing command for option " << arg.toStdString() << std::endl;
        return EXIT_FAILURE;
      }
    } else if ((arg == "--path") || (arg == "-p")) {
      if (narg < argc - 1) {
        ++narg;
        filterPath = argv[narg];
      } else {
        std::cerr << "Missing path for option " << arg.toStdString() << std::endl;
        return EXIT_FAILURE;
      }
    } else if (arg == "--show-last") {
      printLast = true;
    } else if (arg == "--show-last-after") {
      printLastAfter = true;
    } else if (arg.startsWith("--") || arg.startsWith("-")) {
      std::cerr << "Unrecognized option " << arg.toStdString() << std::endl;
      return EXIT_FAILURE;
    } else {
      while (narg < argc) {
        QString filename = QString::fromLocal8Bit(argv[narg]);
        if (QFileInfo(filename).isReadable()) {
          filenames.push_back(filename);
        } else {
          std::cerr << "File cannot be read: " << argv[narg] << std::endl;
          return EXIT_FAILURE;
        }
        ++narg;
      }
    }
    ++narg;
  }

  if (apply && (command.empty() && filterPath.empty() && !repeat)) {
    std::cerr << "Option --apply requires one of --repeat -path --command" << std::endl;
    return EXIT_FAILURE;
  }

  if (printLast || printLastAfter) {
    GmicQt::ReturnedRunParametersFlag flag = printLast ? GmicQt::ReturnedRunParametersFlag::BeforeFilterExecution : GmicQt::ReturnedRunParametersFlag::AfterFilterExecution;
    GmicQt::RunParameters parameters = GmicQt::lastAppliedFilterRunParameters(flag);
    std::cout << "Path: " << parameters.filterPath << std::endl;
    std::cout << "Name: " << parameters.filterName() << std::endl;
    std::cout << "Command: " << parameters.command << std::endl;
    std::cout << "InputMode: " << static_cast<int>(parameters.inputMode) << std::endl;
    std::cout << "OutputMode: " << static_cast<int>(parameters.outputMode) << std::endl;
    return EXIT_SUCCESS;
  }

  std::list<GmicQt::InputMode> disabledInputModes;
  disabledInputModes.push_back(GmicQt::InputMode::NoInput);
  // disabledInputModes.push_back(InputMode::Active);
  disabledInputModes.push_back(GmicQt::InputMode::All);
  disabledInputModes.push_back(GmicQt::InputMode::ActiveAndBelow);
  disabledInputModes.push_back(GmicQt::InputMode::ActiveAndAbove);
  disabledInputModes.push_back(GmicQt::InputMode::AllVisible);
  disabledInputModes.push_back(GmicQt::InputMode::AllInvisible);

  std::list<GmicQt::OutputMode> disabledOutputModes;
  // disabledOutputModes.push_back(GmicQt::OutputMode::InPlace);
  disabledOutputModes.push_back(GmicQt::OutputMode::NewImage);
  disabledOutputModes.push_back(GmicQt::OutputMode::NewLayers);
  disabledOutputModes.push_back(GmicQt::OutputMode::NewActiveLayers);
  GmicQt::RunParameters parameters;
  if (repeat) {
    parameters = GmicQt::lastAppliedFilterRunParameters(GmicQt::ReturnedRunParametersFlag::AfterFilterExecution);
    std::cout << "[gmic_qt] Running with last parameters..." << std::endl;
    std::cout << "Command: " << parameters.command << std::endl;
    std::cout << "Path: " << parameters.filterPath << std::endl;
    std::cout << "Input Mode: " << (int)parameters.inputMode << std::endl;
    std::cout << "Output Mode: " << (int)parameters.outputMode << std::endl;
  } else {
    parameters.filterPath = filterPath;
    parameters.command = command;
  }

  if (filenames.isEmpty()) {
    return GmicQt::run(GmicQt::UserInterfaceMode::Full, parameters, disabledInputModes, disabledOutputModes);
  }
  bool firstLaunch = true;
  for (const QString & filename : filenames) {
    if (loadImage(gmic_qt_standalone::input_image, filename, argc, argv)) {
      gmic_qt_standalone::input_image = gmic_qt_standalone::input_image.convertToFormat(QImage::Format_ARGB32);
      gmic_qt_standalone::current_image_filename = QFileInfo(filename).fileName();
      gmic_qt_standalone::input_image_filename = gmic_qt_standalone::current_image_filename;
      GmicQt::UserInterfaceMode uiMode;
      if (apply || (reapplyFirst && !firstLaunch)) {
        uiMode = GmicQt::UserInterfaceMode::ProgressDialog;
      } else {
        uiMode = GmicQt::UserInterfaceMode::Full;
      }
      if (reapplyFirst && !firstLaunch) {
        parameters = GmicQt::lastAppliedFilterRunParameters(GmicQt::ReturnedRunParametersFlag::BeforeFilterExecution);
      }
      int status = GmicQt::run(uiMode, parameters);
      if (status) {
        std::cerr << "GmicQt::launchPlugin() returned status " << status << std::endl;
        return status;
      }
      firstLaunch = false;
    } else {
      std::cerr << "Could not open image file " << filename.toStdString() << std::endl;
    }
  }
  return EXIT_SUCCESS;
}

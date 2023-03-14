/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file GmicQt.cpp
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
#include "GmicQt.h"
#include <QApplication>
#include <QDebug>
#include <QImage>
#include <QList>
#include <QLocale>
#include <QSettings>
#include <QString>
#include <QThread>
#include <QTimer>
#include <cstdlib>
#include <cstring>
#include "Common.h"
#include "Globals.h"
#include "HeadlessProcessor.h"
#include "LanguageSettings.h"
#include "Logger.h"
#include "MainWindow.h"
#include "Misc.h"
#include "Settings.h"
#include "Widgets/InOutPanel.h"
#include "Widgets/ProgressInfoWindow.h"
#include "gmic.h"
#ifdef _IS_MACOS_
#include <libgen.h>
#include <mach-o/dyld.h>
#include <stdlib.h>
#endif

namespace
{
void configureApplication();
void disableModes(const std::list<GmicQt::InputMode> & disabledInputModes, //
                  const std::list<GmicQt::OutputMode> & disabledOutputModes);

inline bool archIsLittleEndian()
{
  const int x = 1;
  return (*reinterpret_cast<const unsigned char *>(&x));
}

inline unsigned char float2uchar_bounded(const float & in)
{
  return (in < 0.0f) ? 0 : ((in > 255.0f) ? 255 : static_cast<unsigned char>(in));
}

} // namespace

namespace GmicQt
{
InputMode DefaultInputMode = InputMode::Active;
OutputMode DefaultOutputMode = OutputMode::InPlace;
const OutputMessageMode DefaultOutputMessageMode = OutputMessageMode::Quiet;
const int GmicVersion = gmic_version;

const QString & gmicVersionString()
{
  static QString value = QString("%1.%2.%3").arg(gmic_version / 100).arg((gmic_version / 10) % 10).arg(gmic_version % 10);
  return value;
}

RunParameters lastAppliedFilterRunParameters(ReturnedRunParametersFlag flag)
{
  configureApplication();
  RunParameters parameters;
  QSettings settings;
  const QString path = settings.value(QString("LastExecution/host_%1/FilterPath").arg(GmicQtHost::ApplicationShortname)).toString();
  parameters.filterPath = path.toStdString();
  QString args = settings.value(QString("LastExecution/host_%1/Arguments").arg(GmicQtHost::ApplicationShortname)).toString();
  if (flag == ReturnedRunParametersFlag::AfterFilterExecution) {
    QString lastAppliedCommandGmicStatus = settings.value(QString("LastExecution/host_%1/GmicStatusString").arg(GmicQtHost::ApplicationShortname)).toString();
    if (!lastAppliedCommandGmicStatus.isEmpty()) {
      args = lastAppliedCommandGmicStatus;
    }
  }
  QString command = settings.value(QString("LastExecution/host_%1/Command").arg(GmicQtHost::ApplicationShortname)).toString();
  appendWithSpace(command, args);
  parameters.command = command.toStdString();
  parameters.inputMode = (InputMode)settings.value(QString("LastExecution/host_%1/InputMode").arg(GmicQtHost::ApplicationShortname), (int)InputMode::Active).toInt();
  parameters.outputMode = (OutputMode)settings.value(QString("LastExecution/host_%1/OutputMode").arg(GmicQtHost::ApplicationShortname), (int)OutputMode::InPlace).toInt();
  return parameters;
}

int run(UserInterfaceMode interfaceMode,                   //
        RunParameters parameters,                          //
        const std::list<InputMode> & disabledInputModes,   //
        const std::list<OutputMode> & disabledOutputModes, //
        bool * dialogWasAccepted)
{
  int dummy_argc = 1;
  char dummy_app_name[] = GMIC_QT_APPLICATION_NAME;

#ifdef _IS_WINDOWS_
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
#endif

  char * fullexname = nullptr;
#ifdef _IS_MACOS_
  {
    char exname[2048] = {0};
    // get the path where the executable is stored
    uint32_t size = sizeof(exname);
    if (_NSGetExecutablePath(exname, &size) == 0) {
      printf("Executable path is [%s]\n", exname);
      fullexname = realpath(exname, nullptr);
      printf("Full executable name is [%s]\n", fullexname);
      if (fullexname) {
        char * fullpath = dirname(fullexname);
        printf("Full executable path is [%s]\n", fullpath);
        if (fullpath) {
          char pluginpath[2048] = {0};
          strncpy(pluginpath, fullpath, 2047);
          strncat(pluginpath, "/GMIC/plugins/:", 2047);
          char * envpath = getenv("QT_PLUGIN_PATH");
          if (envpath) {
            strncat(pluginpath, envpath, 2047);
          }
          printf("Plugins path is [%s]\n", pluginpath);
          setenv("QT_PLUGIN_PATH", pluginpath, 1);
        }
      }
    } else {
      fprintf(stderr, "Buffer too small; need size %u\n", size);
    }
    setenv("QT_DEBUG_PLUGINS", "1", 1);
  }
#endif
  if (!fullexname) {
    fullexname = dummy_app_name;
  }
  char * dummy_argv[1] = {fullexname};

  disableModes(disabledInputModes, disabledOutputModes);
  if (interfaceMode == UserInterfaceMode::Silent) {
    configureApplication();
    QCoreApplication app(dummy_argc, dummy_argv);
    Settings::load(interfaceMode);
    Logger::setMode(Settings::outputMessageMode());
    HeadlessProcessor processor(&app);
    if (!processor.setPluginParameters(parameters)) {
      Logger::error(processor.error());
      setValueIfNotNullPointer(dialogWasAccepted, false);
      return 1;
    }
    QTimer::singleShot(0, &processor, &HeadlessProcessor::startProcessing);
    int status = QCoreApplication::exec();
    setValueIfNotNullPointer(dialogWasAccepted, processor.processingCompletedProperly());
    return status;
  } else if (interfaceMode == UserInterfaceMode::ProgressDialog) {
    configureApplication();
    QApplication app(dummy_argc, dummy_argv);
    QApplication::setWindowIcon(QIcon(":resources/gmic_hat.png"));
    Settings::load(interfaceMode);
    Logger::setMode(Settings::outputMessageMode());
    LanguageSettings::installTranslators();
    HeadlessProcessor processor(&app);
    if (!processor.setPluginParameters(parameters)) {
      Logger::error(processor.error());
      setValueIfNotNullPointer(dialogWasAccepted, false);
      return 1;
    }
    ProgressInfoWindow progressWindow(&processor);
    unused(progressWindow);
    processor.startProcessing();
    int status = QApplication::exec();
    setValueIfNotNullPointer(dialogWasAccepted, processor.processingCompletedProperly());
    return status;
  } else if (interfaceMode == UserInterfaceMode::Full) {
    configureApplication();
    QApplication app(dummy_argc, dummy_argv);
    QApplication::setWindowIcon(QIcon(":resources/gmic_hat.png"));
    Settings::load(interfaceMode);
    LanguageSettings::installTranslators();
    MainWindow mainWindow;
    mainWindow.setPluginParameters(parameters);
    if (QSettings().value("Config/MainWindowMaximized", false).toBool()) {
      mainWindow.showMaximized();
    } else {
      mainWindow.show();
    }
    int status = QApplication::exec();
    setValueIfNotNullPointer(dialogWasAccepted, mainWindow.isAccepted());
    return status;
  }
  return 0;
}

std::string RunParameters::filterName() const
{
  auto position = filterPath.rfind("/");
  if (position == std::string::npos) {
    return filterPath;
  }
  return filterPath.substr(position + 1, filterPath.length() - (position + 1));
}

template <typename T> //
void calibrateImage(gmic_library::gmic_image<T> & img, const int spectrum, const bool isPreview)
{
  if (!img || !spectrum) {
    return;
  }
  switch (spectrum) {
  case 1: // To GRAY
    switch (img.spectrum()) {
    case 1: // from GRAY
      break;
    case 2: // from GRAYA
      if (isPreview) {
        T *ptr_r = img.data(0, 0, 0, 0), *ptr_a = img.data(0, 0, 0, 1);
        cimg_forXY(img, x, y)
        {
          const unsigned int a = (unsigned int)*(ptr_a++), i = 96 + (((x ^ y) & 8) << 3);
          *ptr_r = (T)((a * (unsigned int)*ptr_r + (255 - a) * i) >> 8);
          ++ptr_r;
        }
      }
      img.channel(0);
      break;
    case 3: // from RGB
      (img.get_shared_channel(0) += img.get_shared_channel(1) += img.get_shared_channel(2)) /= 3;
      img.channel(0);
      break;
    case 4: // from RGBA
      (img.get_shared_channel(0) += img.get_shared_channel(1) += img.get_shared_channel(2)) /= 3;
      if (isPreview) {
        T *ptr_r = img.data(0, 0, 0, 0), *ptr_a = img.data(0, 0, 0, 3);
        cimg_forXY(img, x, y)
        {
          const unsigned int a = (unsigned int)*(ptr_a++), i = 96 + (((x ^ y) & 8) << 3);
          *ptr_r = (T)((a * (unsigned int)*ptr_r + (255 - a) * i) >> 8);
          ++ptr_r;
        }
      }
      img.channel(0);
      break;
    default: // from multi-channel (>4)
      img.channel(0);
    }
    break;

  case 2: // To GRAYA
    switch (img.spectrum()) {
    case 1: // from GRAY
      img.resize(-100, -100, 1, 2, 0).get_shared_channel(1).fill(255);
      break;
    case 2: // from GRAYA
      break;
    case 3: // from RGB
      (img.get_shared_channel(0) += img.get_shared_channel(1) += img.get_shared_channel(2)) /= 3;
      img.channels(0, 1).get_shared_channel(1).fill(255);
      break;
    case 4: // from RGBA
      (img.get_shared_channel(0) += img.get_shared_channel(1) += img.get_shared_channel(2)) /= 3;
      img.get_shared_channel(1) = img.get_shared_channel(3);
      img.channels(0, 1);
      break;
    default: // from multi-channel (>4)
      img.channels(0, 1);
    }
    break;

  case 3: // to RGB
    switch (img.spectrum()) {
    case 1: // from GRAY
      img.resize(-100, -100, 1, 3);
      break;
    case 2: // from GRAYA
      if (isPreview) {
        T *ptr_r = img.data(0, 0, 0, 0), *ptr_a = img.data(0, 0, 0, 1);
        cimg_forXY(img, x, y)
        {
          const unsigned int a = (unsigned int)*(ptr_a++), i = 96 + (((x ^ y) & 8) << 3);
          *ptr_r = (T)((a * (unsigned int)*ptr_r + (255 - a) * i) >> 8);
          ++ptr_r;
        }
      }
      img.channel(0).resize(-100, -100, 1, 3);
      break;
    case 3: // from RGB
      break;
    case 4: // from RGBA
      if (isPreview) {
        T *ptr_r = img.data(0, 0, 0, 0), *ptr_g = img.data(0, 0, 0, 1), *ptr_b = img.data(0, 0, 0, 2), *ptr_a = img.data(0, 0, 0, 3);
        cimg_forXY(img, x, y)
        {
          const unsigned int a = (unsigned int)*(ptr_a++), i = 96 + (((x ^ y) & 8) << 3);
          *ptr_r = (T)((a * (unsigned int)*ptr_r + (255 - a) * i) >> 8);
          *ptr_g = (T)((a * (unsigned int)*ptr_g + (255 - a) * i) >> 8);
          *ptr_b = (T)((a * (unsigned int)*ptr_b + (255 - a) * i) >> 8);
          ++ptr_r;
          ++ptr_g;
          ++ptr_b;
        }
      }
      img.channels(0, 2);
      break;
    default: // from multi-channel (>4)
      img.channels(0, 2);
    }
    break;

  case 4: // to RGBA
    switch (img.spectrum()) {
    case 1: // from GRAY
      img.resize(-100, -100, 1, 4).get_shared_channel(3).fill(255);
      break;
    case 2: // from GRAYA
      img.resize(-100, -100, 1, 4, 0);
      img.get_shared_channel(3) = img.get_shared_channel(1);
      img.get_shared_channel(1) = img.get_shared_channel(0);
      img.get_shared_channel(2) = img.get_shared_channel(0);
      break;
    case 3: // from RGB
      img.resize(-100, -100, 1, 4, 0).get_shared_channel(3).fill(255);
      break;
    case 4: // from RGBA
      break;
    default: // from multi-channel (>4)
      img.channels(0, 3);
    }
    break;
  }
}

template void calibrateImage(gmic_library::gmic_image<gmic_pixel_type> & img, const int spectrum, const bool is_preview);
template void calibrateImage(gmic_library::gmic_image<unsigned char> & img, const int spectrum, const bool is_preview);

void convertGmicImageToQImage(const gmic_library::gmic_image<float> & in, QImage & out)
{
  out = QImage(in.width(), in.height(), QImage::Format_RGB888);

  if (in.spectrum() >= 4 && out.format() != QImage::Format_ARGB32) {
    out = out.convertToFormat(QImage::Format_ARGB32);
  }

  if (in.spectrum() == 3 && out.format() != QImage::Format_RGB888) {
    out = out.convertToFormat(QImage::Format_RGB888);
  }

  if (in.spectrum() == 2 && out.format() != QImage::Format_ARGB32) {
    out = out.convertToFormat(QImage::Format_ARGB32);
  }

// Format_Grayscale8 was added in Qt 5.5.
#if QT_VERSION_GTE(5, 5, 0)
  if (in.spectrum() == 1 && out.format() != QImage::Format_Grayscale8) {
    out = out.convertToFormat(QImage::Format_Grayscale8);
  }
#else
  if (in.spectrum() == 1) {
    out = out.convertToFormat(QImage::Format_RGB888);
  }
#endif

  if (in.spectrum() >= 4) {
    const float * srcR = in.data(0, 0, 0, 0);
    const float * srcG = in.data(0, 0, 0, 1);
    const float * srcB = in.data(0, 0, 0, 2);
    const float * srcA = in.data(0, 0, 0, 3);
    int height = out.height();
    if (archIsLittleEndian()) {
      for (int y = 0; y < height; ++y) {
        int n = in.width();
        unsigned char * dst = out.scanLine(y);
        while (n--) {
          dst[0] = float2uchar_bounded(*srcB++);
          dst[1] = float2uchar_bounded(*srcG++);
          dst[2] = float2uchar_bounded(*srcR++);
          dst[3] = float2uchar_bounded(*srcA++);
          dst += 4;
        }
      }
    } else {
      for (int y = 0; y < height; ++y) {
        int n = in.width();
        unsigned char * dst = out.scanLine(y);
        while (n--) {
          dst[0] = float2uchar_bounded(*srcA++);
          dst[1] = float2uchar_bounded(*srcR++);
          dst[2] = float2uchar_bounded(*srcG++);
          dst[3] = float2uchar_bounded(*srcB++);
          dst += 4;
        }
      }
    }
  } else if (in.spectrum() == 3) {
    const float * srcR = in.data(0, 0, 0, 0);
    const float * srcG = in.data(0, 0, 0, 1);
    const float * srcB = in.data(0, 0, 0, 2);
    int height = out.height();
    for (int y = 0; y < height; ++y) {
      int n = in.width();
      unsigned char * dst = out.scanLine(y);
      while (n--) {
        dst[0] = float2uchar_bounded(*srcR++);
        dst[1] = float2uchar_bounded(*srcG++);
        dst[2] = float2uchar_bounded(*srcB++);
        dst += 3;
      }
    }
  } else if (in.spectrum() == 2) {
    //
    // Gray + Alpha
    //
    const float * src = in.data(0, 0, 0, 0);
    const float * srcA = in.data(0, 0, 0, 1);
    int height = out.height();
    if (archIsLittleEndian()) {
      for (int y = 0; y < height; ++y) {
        int n = in.width();
        unsigned char * dst = out.scanLine(y);
        while (n--) {
          dst[2] = dst[1] = dst[0] = float2uchar_bounded(*src++);
          dst[3] = float2uchar_bounded(*srcA++);
          dst += 4;
        }
      }
    } else {
      for (int y = 0; y < height; ++y) {
        int n = in.width();
        unsigned char * dst = out.scanLine(y);
        while (n--) {
          dst[1] = dst[2] = dst[3] = float2uchar_bounded(*src++);
          dst[0] = float2uchar_bounded(*srcA++);
          dst += 4;
        }
      }
    }
  } else {
    //
    // 8-bits Gray levels
    //
    const float * src = in.data(0, 0, 0, 0);
    int height = out.height();
    for (int y = 0; y < height; ++y) {
      int n = in.width();
      unsigned char * dst = out.scanLine(y);
#if QT_VERSION_GTE(5, 5, 0)
      while (n--) {
        *dst++ = static_cast<unsigned char>(*src++);
      }
#else
      while (n--) {
        dst[0] = float2uchar_bounded(*src);
        dst[1] = float2uchar_bounded(*src);
        dst[2] = float2uchar_bounded(*src);
        ++src;
        dst += 3;
      }
#endif
    }
  }
}

void convertQImageToGmicImage(const QImage & in, gmic_library::gmic_image<float> & out)
{
  Q_ASSERT_X(in.format() == QImage::Format_ARGB32 || in.format() == QImage::Format_RGB888, "convert", "bad input format");

  if (in.format() == QImage::Format_ARGB32) {
    const int w = in.width();
    const int h = in.height();
    out.assign(w, h, 1, 4);
    float * dstR = out.data(0, 0, 0, 0);
    float * dstG = out.data(0, 0, 0, 1);
    float * dstB = out.data(0, 0, 0, 2);
    float * dstA = out.data(0, 0, 0, 3);
    if (archIsLittleEndian()) {
      for (int y = 0; y < h; ++y) {
        const unsigned char * src = in.scanLine(y);
        int n = in.width();
        while (n--) {
          *dstB++ = static_cast<float>(src[0]);
          *dstG++ = static_cast<float>(src[1]);
          *dstR++ = static_cast<float>(src[2]);
          *dstA++ = static_cast<float>(src[3]);
          src += 4;
        }
      }
    } else {
      for (int y = 0; y < h; ++y) {
        const unsigned char * src = in.scanLine(y);
        int n = in.width();
        while (n--) {
          *dstA++ = static_cast<float>(src[0]);
          *dstR++ = static_cast<float>(src[1]);
          *dstG++ = static_cast<float>(src[2]);
          *dstB++ = static_cast<float>(src[3]);
          src += 4;
        }
      }
    }
    return;
  }

  if (in.format() == QImage::Format_RGB888) {
    const int w = in.width();
    const int h = in.height();
    out.assign(w, h, 1, 3);
    float * dstR = out.data(0, 0, 0, 0);
    float * dstG = out.data(0, 0, 0, 1);
    float * dstB = out.data(0, 0, 0, 2);
    for (int y = 0; y < h; ++y) {
      const unsigned char * src = in.scanLine(y);
      int n = in.width();
      while (n--) {
        *dstR++ = static_cast<float>(src[0]);
        *dstG++ = static_cast<float>(src[1]);
        *dstB++ = static_cast<float>(src[2]);
        src += 3;
      }
    }
    return;
  }
}

} // namespace GmicQt

namespace
{

void configureApplication()
{
  QCoreApplication::setOrganizationName(GMIC_QT_ORGANISATION_NAME);
  QCoreApplication::setOrganizationDomain(GMIC_QT_ORGANISATION_DOMAIN);
  QCoreApplication::setApplicationName(GMIC_QT_APPLICATION_NAME);
  QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar);
#if !QT_VERSION_GTE(6, 0, 0)
  if (QSettings().value(HIGHDPI_KEY, false).toBool()) {
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  }
#endif
}

void disableModes(const std::list<GmicQt::InputMode> & disabledInputModes, //
                  const std::list<GmicQt::OutputMode> & disabledOutputModes)
{
  for (const GmicQt::InputMode & mode : disabledInputModes) {
    GmicQt::InOutPanel::disableInputMode(mode);
  }
  for (const GmicQt::OutputMode & mode : disabledOutputModes) {
    GmicQt::InOutPanel::disableOutputMode(mode);
  }
}

} // namespace

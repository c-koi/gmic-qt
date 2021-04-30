/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file gmic_qt.cpp
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
#include "gmic_qt.h"
#include <QApplication>
#include <QDebug>
#include <QList>
#include <QLocale>
#include <QSettings>
#include <QString>
#include <QThread>
#include <QTimer>
#include <cstdlib>
#include <cstring>
#include "Common.h"
#include "DialogSettings.h"
#include "Globals.h"
#include "HeadlessProcessor.h"
#include "LanguageSettings.h"
#include "Logger.h"
#include "MainWindow.h"
#include "Misc.h"
#include "Updater.h"
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
bool pluginProcessingValidAndAccepted = false;
void configureApplication();
void disableModes(const std::list<GmicQt::InputMode> & disabledInputModes, //
                  const std::list<GmicQt::OutputMode> & disabledOutputModes);
} // namespace

namespace GmicQt
{
InputMode DefaultInputMode = Active;
OutputMode DefaultOutputMode = InPlace;
const OutputMessageMode DefaultOutputMessageMode = Quiet;

const QString & gmicVersionString()
{
  static QString value = QString("%1.%2.%3").arg(gmic_version / 100).arg((gmic_version / 10) % 10).arg(gmic_version % 10);
  return value;
}

PluginParameters lastAppliedFilterPluginParameters(PluginParameterFlag flag)
{
  // TODO : Complete
  configureApplication();
  PluginParameters parameters;
  QSettings settings;
  const QString path = settings.value(QString("LastExecution/host_%1/FilterPath").arg(GmicQt::HostApplicationShortname)).toString();
  if (path.isEmpty()) {
    return parameters;
  }
  parameters.filterPath = "/" + path.toStdString();

  QString args = settings.value(QString("LastExecution/host_%1/Arguments").arg(GmicQt::HostApplicationShortname)).toString();
  if (flag == AfterFilterExecution) {
    QStringList lastAppliedCommandGmicStatus = settings.value(QString("LastExecution/host_%1/GmicStatus").arg(GmicQt::HostApplicationShortname)).toStringList();
    if (!lastAppliedCommandGmicStatus.isEmpty()) {
      args = flattenGmicParameterList(lastAppliedCommandGmicStatus);
    }
  }
  QString command = settings.value(QString("LastExecution/host_%1/Command").arg(GmicQt::HostApplicationShortname)).toString();
  appendWithSpace(command, args);
  parameters.command = command.toStdString();
  parameters.inputMode = (GmicQt::InputMode)settings.value(QString("LastExecution/host_%1/InputMode").arg(GmicQt::HostApplicationShortname), GmicQt::InputMode::Active).toInt();
  parameters.outputMode = (GmicQt::OutputMode)settings.value(QString("LastExecution/host_%1/OutputMode").arg(GmicQt::HostApplicationShortname), GmicQt::OutputMode::InPlace).toInt();
  // parameters.previewMode = (GmicQt::PreviewMode)settings.value(QString("LastExecution/host_%1/PreviewMode").arg(GmicQt::HostApplicationShortname), GmicQt::DefaultPreviewMode).toInt();

  return parameters;
}

bool pluginDialogWasAccepted()
{
  return pluginProcessingValidAndAccepted;
}

int launchPlugin(UserInterfaceMode interfaceMode,                         //
                 PluginParameters parameters,                             //
                 const std::list<GmicQt::InputMode> & disabledInputModes, //
                 const std::list<GmicQt::OutputMode> & disabledOutputModes)
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
  if (interfaceMode == GmicQt::NoGUI) { // launchPluginHeadless(const char * command, GmicQt::InputMode input, GmicQt::OutputMode output)
    QCoreApplication app(dummy_argc, dummy_argv);
    configureApplication();
    DialogSettings::loadSettings(GmicQt::NonGuiApplication);
    Logger::setMode(DialogSettings::outputMessageMode());
    // TODO : What if command is nullptr?
    HeadlessProcessor headlessProcessor(&app, parameters.command.c_str(), parameters.inputMode, parameters.outputMode);
    QTimer::singleShot(0, &headlessProcessor, SLOT(startProcessing()));
    int status = QCoreApplication::exec();
    pluginProcessingValidAndAccepted = headlessProcessor.processingCompletedProperly();
    return status;
  } else if (interfaceMode == GmicQt::ProgressDialogGUI) { // launchPluginHeadlessUsingLastParameters
    QApplication app(dummy_argc, dummy_argv);
    QApplication::setWindowIcon(QIcon(":resources/gmic_hat.png"));
    configureApplication();
    DialogSettings::loadSettings(GmicQt::GuiApplication);
    Logger::setMode(DialogSettings::outputMessageMode());
    LanguageSettings::installTranslators();
    HeadlessProcessor processor(&app); // TODO : use command parameter
    ProgressInfoWindow progressWindow(&processor);
    if (processor.command().isEmpty()) {
      pluginProcessingValidAndAccepted = false;
      return 0;
    }
    processor.startProcessing();
    int status = QApplication::exec();
    pluginProcessingValidAndAccepted = processor.processingCompletedProperly();
    return status;
  } else if (interfaceMode == GmicQt::FullGUI) {
    QApplication app(dummy_argc, dummy_argv);
    QApplication::setWindowIcon(QIcon(":resources/gmic_hat.png"));
    configureApplication();
    DialogSettings::loadSettings(GmicQt::GuiApplication);
    LanguageSettings::installTranslators();
    MainWindow mainWindow;
    if (QSettings().value("Config/MainWindowMaximized", false).toBool()) {
      mainWindow.showMaximized();
    } else {
      mainWindow.show();
    }
    int status = QApplication::exec();
    pluginProcessingValidAndAccepted = mainWindow.isAccepted();
    return status;
  }
  return 0;
}

std::string PluginParameters::filterName() const
{
  auto position = filterPath.rfind("/");
  if (position == std::string::npos) {
    return filterPath;
  }
  return filterPath.substr(position + 1, filterPath.length() - (position + 1));
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
}

void disableModes(const std::list<GmicQt::InputMode> & disabledInputModes, //
                  const std::list<GmicQt::OutputMode> & disabledOutputModes)
{
  for (const GmicQt::InputMode & mode : disabledInputModes) {
    InOutPanel::disableInputMode(mode);
  }
  for (const GmicQt::OutputMode & mode : disabledOutputModes) {
    InOutPanel::disableOutputMode(mode);
  }
}

} // namespace

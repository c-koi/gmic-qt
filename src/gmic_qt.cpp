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
#include <QJsonObject>
#include <QList>
#include <QLocale>
#include <QSettings>
#include <QString>
#include <QTimer>
#include <QTranslator>
#include <cstring>
#include "Common.h"
#include "HeadlessProcessor.h"
#include "MainWindow.h"
#include "Updater.h"
#include "Widgets/LanguageSelectionWidget.h"
#include "Widgets/ProgressInfoWindow.h"
#include "gmic.h"

#ifdef _IS_WINDOWS_
#include <windows.h>
#include <tlhelp32.h>
#endif
#ifdef _IS_LINUX_
#include <unistd.h>
#endif

namespace GmicQt
{

const InputMode DefaultInputMode = Active;
const OutputMode DefaultOutputMode = InPlace;
const OutputMessageMode DefaultOutputMessageMode = Quiet;
const PreviewMode DefaultPreviewMode = FirstOutput;
const InputOutputState InputOutputState::Default(GmicQt::DefaultInputMode, GmicQt::DefaultOutputMode, GmicQt::DefaultPreviewMode, GmicQt::DefaultOutputMessageMode);

const float PreviewFactorAny = -1.0f;
const float PreviewFactorFullImage = 1.0f;
const float PreviewFactorActualSize = 0.0f;

const QString & path_rc(bool create)
{
  QString qpath = QString::fromLocal8Bit(gmic::path_rc());
  QFileInfo dir(qpath);
  static QString result;
  if (dir.isDir()) {
    result = qpath;
    return result;
  }
  if (!create || !gmic::init_rc()) {
    result.clear();
  } else {
    result = QString::fromLocal8Bit(gmic::path_rc());
  }
  return result;
}

unsigned int host_app_pid()
{
#if defined(_IS_LINUX_)
  return static_cast<int>(getppid());
#elif defined(_IS_WINDOWS_)
  HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  PROCESSENTRY32 pe;
  memset(&pe, 0, sizeof(PROCESSENTRY32));
  pe.dwSize = sizeof(PROCESSENTRY32);
  DWORD pid = GetCurrentProcessId();
  if (Process32First(h, &pe)) {
    do {
      if (pe.th32ProcessID == pid) {
        CloseHandle(h);
        return static_cast<unsigned int>(pe.th32ParentProcessID);
      }
    } while (Process32Next(h, &pe));
  }
  CloseHandle(h);
  return static_cast<unsigned int>(pid); // Process own id if no parent was found
#else
  return 0;
#endif
}

const QString & gmicVersionString()
{
  static QString value = QString("%1.%2.%3").arg(gmic_version / 100).arg((gmic_version / 10) % 10).arg(gmic_version % 10);
  return value;
}

InputOutputState::InputOutputState() : inputMode(UnspecifiedInputMode), outputMode(UnspecifiedOutputMode), previewMode(GmicQt::UnspecifiedPreviewMode), outputMessageMode(UnspecifiedOutputMessageMode)
{
}

InputOutputState::InputOutputState(InputMode inputMode, OutputMode outputMode, PreviewMode previewMode, OutputMessageMode outputMessageMode)
    : inputMode(inputMode), outputMode(outputMode), previewMode(previewMode), outputMessageMode(outputMessageMode)
{
}

bool InputOutputState::operator==(const InputOutputState & other) const
{
  return inputMode == other.inputMode && outputMode == other.outputMode && previewMode == other.previewMode && outputMessageMode == other.outputMessageMode;
}

bool InputOutputState::operator!=(const InputOutputState & other) const
{
  return inputMode != other.inputMode || outputMode != other.outputMode || previewMode != other.previewMode || outputMessageMode != other.outputMessageMode;
}

bool InputOutputState::isDefault() const
{
  return (inputMode == GmicQt::DefaultInputMode) && (outputMode == GmicQt::DefaultOutputMode) && (previewMode == GmicQt::DefaultPreviewMode) && (outputMessageMode == GmicQt::DefaultOutputMessageMode);
}

void InputOutputState::toJSONObject(QJsonObject & object) const
{
  object = QJsonObject();
  if (inputMode != DefaultInputMode) {
    object.insert("InputLayers", inputMode);
  }
  if (outputMode != DefaultOutputMode) {
    object.insert("OutputMode", outputMode);
  }
  if (previewMode != DefaultPreviewMode) {
    object.insert("PreviewMode", previewMode);
  }
  if (outputMessageMode != DefaultOutputMessageMode) {
    object.insert("OutputMessages", outputMessageMode);
  }
}

InputOutputState InputOutputState::fromJSONObject(const QJsonObject & object)
{
  GmicQt::InputOutputState state;
  state.inputMode = static_cast<InputMode>(object.value("InputLayers").toInt(UnspecifiedInputMode));
  state.outputMode = static_cast<OutputMode>(object.value("OutputMode").toInt(UnspecifiedOutputMode));
  state.previewMode = static_cast<PreviewMode>(object.value("PreviewMode").toInt(UnspecifiedPreviewMode));
  state.outputMessageMode = static_cast<OutputMessageMode>(object.value("OutputMessages").toInt(UnspecifiedOutputMessageMode));
  return state;
}
}

int launchPlugin()
{
  int dummy_argc = 1;
  char dummy_app_name[] = GMIC_QT_APPLICATION_NAME;
  char * dummy_argv[1] = {dummy_app_name};

#ifdef _IS_WINDOWS_
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
#endif

  QApplication app(dummy_argc, dummy_argv);
  app.setWindowIcon(QIcon(":resources/gmic_hat.png"));
  QCoreApplication::setOrganizationName(GMIC_QT_ORGANISATION_NAME);
  QCoreApplication::setOrganizationDomain(GMIC_QT_ORGANISATION_DOMAIN);
  QCoreApplication::setApplicationName(GMIC_QT_APPLICATION_NAME);
  QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar);

  // Translate according to current locale or configured language
  QString lang = LanguageSelectionWidget::configuredTranslator();
  if (!lang.isEmpty() && (lang != "en")) {
    QTranslator * translator = new QTranslator(&app);
    translator->load(QString(":/translations/%1.qm").arg(lang));
    app.installTranslator(translator);
  }

  Updater::setInstanceParent(&app);
  int ageLimit;
  {
    QSettings settings;
    GmicQt::OutputMessageMode mode = static_cast<GmicQt::OutputMessageMode>(settings.value("OutputMessageModeValue", GmicQt::Quiet).toInt());
    Updater::setOutputMessageMode(mode);
    ageLimit = settings.value(INTERNET_UPDATE_PERIODICITY_KEY, 0).toInt();
  }
  Updater * updater = Updater::getInstance();
  MainWindow mainWindow;
  QObject::connect(updater, SIGNAL(downloadsFinished(bool)), &mainWindow, SLOT(startupUpdateFinished(bool)));
  updater->startUpdate(ageLimit, 4, ageLimit != INTERNET_NEVER_UPDATE_PERIODICITY);
  return app.exec();
}

int launchPluginHeadlessUsingLastParameters()
{
  int dummy_argc = 1;
  char dummy_app_name[] = GMIC_QT_APPLICATION_NAME;
  char * dummy_argv[1] = {dummy_app_name};
#ifdef _IS_WINDOWS_
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
#endif
  QApplication app(dummy_argc, dummy_argv);
  app.setWindowIcon(QIcon(":resources/gmic_hat.png"));
  QCoreApplication::setOrganizationName(GMIC_QT_ORGANISATION_NAME);
  QCoreApplication::setOrganizationDomain(GMIC_QT_ORGANISATION_DOMAIN);
  QCoreApplication::setApplicationName(GMIC_QT_APPLICATION_NAME);
  QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar);
  Updater::setInstanceParent(&app);
  HeadlessProcessor processor;
  ProgressInfoWindow progressWindow(&processor);
  if (processor.command().isEmpty()) {
    return 0;
  } else {
    processor.startProcessing();
    return app.exec();
  }
}

int launchPluginHeadless(const char * command, GmicQt::InputMode input, GmicQt::OutputMode output)
{
  int dummy_argc = 1;
  char dummy_app_name[] = GMIC_QT_APPLICATION_NAME;
  char * dummy_argv[1] = {dummy_app_name};
#ifdef _IS_WINDOWS_
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
#endif
  QCoreApplication app(dummy_argc, dummy_argv);
  QCoreApplication::setOrganizationName(GMIC_QT_ORGANISATION_NAME);
  QCoreApplication::setOrganizationDomain(GMIC_QT_ORGANISATION_DOMAIN);
  QCoreApplication::setApplicationName(GMIC_QT_APPLICATION_NAME);
  QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar);
  Updater::setInstanceParent(&app);
  HeadlessProcessor headlessProcessor(&app, command, input, output);
  QTimer idle;
  idle.setInterval(0);
  idle.setSingleShot(true);
  QObject::connect(&idle, SIGNAL(timeout()), &headlessProcessor, SLOT(startProcessing()));
  idle.start();
  return app.exec();
}

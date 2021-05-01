/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file HeadlessProcessor.cpp
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
#include "HeadlessProcessor.h"
#include <QDebug>
#include <QMessageBox>
#include <QSettings>
#include <QStringList>
#include "Common.h"
#include "FilterParameters/FilterParametersWidget.h"
#include "FilterSelector/FiltersPresenter.h"
#include "FilterThread.h"
#include "GmicStdlib.h"
#include "HtmlTranslator.h"
#include "Logger.h"
#include "Misc.h"
#include "ParametersCache.h"
#include "Updater.h"
#include "Widgets/ProgressInfoWindow.h"
#include "gmic.h"

#ifdef _IS_WINDOWS_
#include <windows.h>
#include <process.h>
#include <psapi.h>
#endif

/**
 * @brief HeadlessProcessor::HeadlessProcessor using "last parameters" from config file
 * @param parent
 */
HeadlessProcessor::HeadlessProcessor(QObject * parent) //
    : QObject(parent), _filterThread(nullptr), _gmicImages(new cimg_library::CImgList<gmic_pixel_type>)
{
  _progressWindow = nullptr;
  _processingCompletedProperly = false;
  Updater::getInstance()->updateSources(false);
  GmicStdLib::Array = Updater::getInstance()->buildFullStdlib();
}

HeadlessProcessor::~HeadlessProcessor()
{
  delete _gmicImages;
}

bool HeadlessProcessor::setPluginParameters(const GmicQt::PluginParameters & parameters)
{
  QSettings settings;
  // FIXME : Use translated version of the name
  _path = QString::fromStdString(parameters.filterPath);
  _inputMode = (parameters.inputMode == GmicQt::UnspecifiedInputMode) ? GmicQt::DefaultInputMode : parameters.inputMode;
  _outputMode = (parameters.outputMode == GmicQt::UnspecifiedOutputMode) ? GmicQt::DefaultOutputMode : parameters.outputMode;

  if (_path.isEmpty()) { // A pure command
    if (parameters.command.empty()) {
      _errorMessage = tr("At least a filter path or a filter command must be provided.");
    } else {
      QString command;
      QString arguments;
      QStringList argumentList;
      FiltersPresenter::Filter filter;
      if (parseGmicUniqueFilterCommand(parameters.command.c_str(), command, arguments) //
          && parseGmicUniqueFilterParameters(arguments.toUtf8().constData(), argumentList)) {
        filter = FiltersPresenter::findFilterFromCommandInStdlib(command);
      }
      if (filter.isValid()) {
        _filterName = filter.plainTextName;
        _hash = filter.hash;
        _path = filter.fullPath;
        _command = command;
        _arguments = arguments;
      } else {
        _filterName = tr("Custom command (%1)").arg(elided(QString::fromStdString(parameters.command), 35));
        _command = "skip 0";
        _arguments = QString::fromStdString(parameters.command);
      }
    }
  } else { // A path is given
    QString plainPath = HtmlTranslator::html2txt(_path, false);

    // FIXME : What if the filter is in the .gmic ? Does this work ?!
    FiltersPresenter::Filter filter = FiltersPresenter::findFilterFromAbsolutePathOrNameInStdlib(plainPath);
    if (filter.isInvalid()) {
      _errorMessage = tr("Cannot find filter matching path %1").arg(_path);
    } else {
      if (parameters.command.empty()) {
        QString error;
        QVector<AbstractParameter *> defaultParameters = FilterParametersWidget::buildParameters(filter.parameters, nullptr, nullptr, &error);
        if (error.isEmpty()) {
          _filterName = filter.plainTextName;
          _hash = filter.hash;
          _command = filter.command;
          _arguments = FilterParametersWidget::valueString(defaultParameters);
        } else {
          _errorMessage = tr("Error parsing filter parameters definition for filter:\n\n%1\n\nCannot retrieve default parameters.\n\n%2").arg(_path).arg(error);
        }
      } else {
        QString command;
        QString arguments;
        const bool validSingleGmicCommand = parseGmicUniqueFilterCommand(parameters.command.c_str(), command, arguments);
        if (validSingleGmicCommand && (command == filter.command)) {
          _filterName = filter.plainTextName;
          _hash = filter.hash;
          _command = command;
          _arguments = arguments;
        } else {
          _errorMessage = tr("Supplied command [%1] does not match path [%2] (should be %3).").arg(command).arg(plainPath).arg(filter.command);
        }
      }
    }
  }
  _outputMessageMode = (GmicQt::OutputMessageMode)settings.value("OutputMessageMode", GmicQt::DefaultOutputMessageMode).toInt();
  Logger::setMode(_outputMessageMode);
  return _errorMessage.isEmpty();
}

const QString & HeadlessProcessor::error() const
{
  return _errorMessage;
}

void HeadlessProcessor::startProcessing()
{
  ENTERING;
  if (!_errorMessage.isEmpty()) {
    endApplication(_errorMessage);
  }

  _singleShotTimer.setInterval(750);
  _singleShotTimer.setSingleShot(true);
  connect(&_singleShotTimer, &QTimer::timeout, this, &HeadlessProcessor::progressWindowShouldShow);
  ParametersCache::load(true);

  _singleShotTimer.start();
  _gmicImages->assign();
  gmic_list<char> imageNames;
  gmic_qt_get_cropped_images(*_gmicImages, imageNames, -1, -1, -1, -1, _inputMode);
  if (!_progressWindow) {
    gmic_qt_show_message(QString("G'MIC: %1").arg(_arguments).toUtf8().constData());
  }
  QString env = QString("_input_layers=%1").arg(_inputMode);
  env += QString(" _output_mode=%1").arg(_outputMode);
  env += QString(" _output_messages=%1").arg(_outputMessageMode);
  _filterThread = new FilterThread(this, _command, _arguments, env, _outputMessageMode);
  _filterThread->swapImages(*_gmicImages);
  _filterThread->setImageNames(imageNames);
  _processingCompletedProperly = false;
  connect(_filterThread, SIGNAL(finished()), this, SLOT(onProcessingFinished()));
  _timer.setInterval(250);
  connect(&_timer, &QTimer::timeout, this, &HeadlessProcessor::sendProgressInformation);
  _timer.start();
  _filterThread->start();
}

QString HeadlessProcessor::command() const
{
  return _command;
}

QString HeadlessProcessor::filterName() const
{
  return _filterName;
}

void HeadlessProcessor::setProgressWindow(ProgressInfoWindow * progressInfoWindow)
{
  _progressWindow = progressInfoWindow;
}

bool HeadlessProcessor::processingCompletedProperly()
{
  return _processingCompletedProperly;
}

void HeadlessProcessor::sendProgressInformation()
{
  if (!_filterThread) {
    return;
  }
  float progress = _filterThread->progress();
  int ms = _filterThread->duration();
  unsigned long memory = 0;
#if defined(_IS_LINUX_)
  QFile status("/proc/self/status");
  if (status.open(QFile::ReadOnly)) {
    QByteArray text = status.readAll();
    const char * str = strstr(text.constData(), "VmRSS:");
    unsigned int kiB = 0;
    if (str && sscanf(str + 7, "%u", &kiB)) {
      memory = 1024 * (unsigned long)kiB;
    }
  }
#elif defined(_IS_WINDOWS_)
  PROCESS_MEMORY_COUNTERS counters;
  if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters))) {
    memory = static_cast<unsigned long>(counters.WorkingSetSize);
  }
#else
// TODO: MACOS
#endif
  emit progression(progress, ms, memory);
}

void HeadlessProcessor::onProcessingFinished()
{
  _timer.stop();
  QString errorMessage;
  QStringList status = _filterThread->gmicStatus();
  if (_filterThread->failed()) {
    errorMessage = _filterThread->errorMessage();
    if (errorMessage.isEmpty()) {
      errorMessage = tr("Filter execution failed, but with no error message.");
    }
  } else {
    gmic_list<gmic_pixel_type> images = _filterThread->images();
    if (!_filterThread->aborted()) {
      gmic_qt_output_images(images, _filterThread->imageNames(), _outputMode);
      _processingCompletedProperly = true;
    }
    if (!status.isEmpty() && !_hash.isEmpty()) {
      ParametersCache::setValues(_hash, status);
      ParametersCache::save();
    }
    QSettings settings;
    settings.setValue(QString("LastExecution/host_%1/GmicStatus").arg(GmicQt::HostApplicationShortname), status);
    settings.setValue(QString("LastExecution/host_%1/FilterPath").arg(GmicQt::HostApplicationShortname), _path);
    settings.setValue(QString("LastExecution/host_%1/FilterHash").arg(GmicQt::HostApplicationShortname), _hash);
    settings.setValue(QString("LastExecution/host_%1/Command").arg(GmicQt::HostApplicationShortname), _command);
    settings.setValue(QString("LastExecution/host_%1/Arguments").arg(GmicQt::HostApplicationShortname), _arguments);
    settings.setValue(QString("LastExecution/host_%1/InputMode").arg(GmicQt::HostApplicationShortname), _inputMode);
    settings.setValue(QString("LastExecution/host_%1/OutputMode").arg(GmicQt::HostApplicationShortname), _outputMode);
  }
  _filterThread->deleteLater();
  _filterThread = nullptr;
  endApplication(errorMessage);
}

void HeadlessProcessor::cancel()
{
  if (_filterThread) {
    _filterThread->abortGmic();
  }
}

void HeadlessProcessor::endApplication(const QString & errorMessage)
{
  _singleShotTimer.stop();
  emit done(errorMessage);
  if (!errorMessage.isEmpty()) {
    Logger::error(errorMessage);
  }
  QCoreApplication::exit(not errorMessage.isEmpty());
}

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
#include <QSettings>
#include <QStringList>
#include "Common.h"
#include "FilterParameters/FilterParametersWidget.h"
#include "FilterThread.h"
#include "GmicStdlib.h"
#include "HtmlTranslator.h"
#include "Logger.h"
#include "Misc.h"
#include "ParametersCache.h"
#include "Updater.h"
#include "gmic.h"

#ifdef _IS_WINDOWS_
#include <windows.h>
#include <process.h>
#include <psapi.h>
#endif

/**
 * @brief HeadlessProcessor::HeadlessProcessor
 * @param parent
 * @param command
 * @param inputMode
 * @param outputMode
 */
HeadlessProcessor::HeadlessProcessor(QObject * parent, const char * command, GmicQt::InputMode inputMode, GmicQt::OutputMode outputMode)
    : QObject(parent), _filterThread(nullptr), _gmicImages(new cimg_library::CImgList<gmic_pixel_type>)
{
  _filterName = "Custom command";
  _lastCommand = "skip 0";
  _lastArguments = command;
  _outputMessageMode = GmicQt::Quiet;
  _inputMode = inputMode;
  _outputMode = outputMode;
  // FIXME : Remove this key QString("LastExecution/host_%1/PreviewMode").arg(GmicQt::HostApplicationShortname)

  _timer.setInterval(250);
  connect(&_timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
  _hasProgressWindow = false;
  ParametersCache::load(true);
  _processingCompletedProperly = false;
}

/**
 * @brief HeadlessProcessor::HeadlessProcessor using "last parameters" from config file
 * @param parent
 */
HeadlessProcessor::HeadlessProcessor(QObject * parent) : QObject(parent), _filterThread(nullptr), _gmicImages(new cimg_library::CImgList<gmic_pixel_type>)
{
  QSettings settings;
  // FIXME : Use translated version of the name
  const QString path = settings.value(QString("LastExecution/host_%1/FilterPath").arg(GmicQt::HostApplicationShortname)).toString();
  _filterName = HtmlTranslator::html2txt(filterFullPathBasename(path), true);
  _lastCommand = settings.value(QString("LastExecution/host_%1/Command").arg(GmicQt::HostApplicationShortname)).toString();
  _lastArguments = settings.value(QString("LastExecution/host_%1/Arguments").arg(GmicQt::HostApplicationShortname)).toString();

  // TODO : How do we handle the status in the new context ?

  QStringList lastAppliedCommandGmicStatus = settings.value(QString("LastExecution/host_%1/GmicStatus").arg(GmicQt::HostApplicationShortname)).toStringList();
  _gmicStatusQuotedParameters = settings.value(QString("LastExecution/host_%1/QuotedParameters").arg(GmicQt::HostApplicationShortname)).toString();
  if (!lastAppliedCommandGmicStatus.isEmpty()) {
    _lastArguments = flattenGmicParameterList(lastAppliedCommandGmicStatus, _gmicStatusQuotedParameters);
  }

  _outputMessageMode = (GmicQt::OutputMessageMode)settings.value("OutputMessageMode", GmicQt::DefaultOutputMessageMode).toInt();
  _inputMode = (GmicQt::InputMode)settings.value(QString("LastExecution/host_%1/InputMode").arg(GmicQt::HostApplicationShortname), GmicQt::InputMode::Active).toInt();
  _outputMode = (GmicQt::OutputMode)settings.value(QString("LastExecution/host_%1/OutputMode").arg(GmicQt::HostApplicationShortname), GmicQt::OutputMode::InPlace).toInt();
  _timer.setInterval(250);
  connect(&_timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
  _singleShotTimer.setInterval(750);
  _singleShotTimer.setSingleShot(true);
  connect(&_singleShotTimer, SIGNAL(timeout()), this, SIGNAL(singleShotTimeout()));
  _hasProgressWindow = false;
  ParametersCache::load(true);
}

HeadlessProcessor::~HeadlessProcessor()
{
  delete _gmicImages;
}

void HeadlessProcessor::startProcessing()
{
  _singleShotTimer.start();
  Updater::getInstance()->updateSources(false);
  GmicStdLib::Array = Updater::getInstance()->buildFullStdlib();
  _gmicImages->assign();
  gmic_list<char> imageNames;
  gmic_qt_get_cropped_images(*_gmicImages, imageNames, -1, -1, -1, -1, _inputMode);
  if (!_hasProgressWindow) {
    gmic_qt_show_message(QString("G'MIC: %1").arg(_lastArguments).toUtf8().constData());
  }
  QString env = QString("_input_layers=%1").arg(_inputMode);
  env += QString(" _output_mode=%1").arg(_outputMode);
  env += QString(" _output_messages=%1").arg(_outputMessageMode);
  _filterThread = new FilterThread(this, _lastCommand, _lastArguments, env, _outputMessageMode);
  _filterThread->swapImages(*_gmicImages);
  _filterThread->setImageNames(imageNames);
  _processingCompletedProperly = false;
  connect(_filterThread, SIGNAL(finished()), this, SLOT(onProcessingFinished()));
  _timer.start();
  _filterThread->start();
}

QString HeadlessProcessor::command() const
{
  return _lastCommand;
}

QString HeadlessProcessor::filterName() const
{
  return _filterName;
}

void HeadlessProcessor::setProgressWindowFlag(bool value)
{
  _hasProgressWindow = value;
}

bool HeadlessProcessor::processingCompletedProperly()
{
  return _processingCompletedProperly;
}

void HeadlessProcessor::onTimeout()
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
  if (!status.isEmpty()) {
    QSettings settings;
    settings.setValue(QString("LastExecution/host_%1/GmicStatus").arg(GmicQt::HostApplicationShortname), status);
    QString lastArguments = flattenGmicParameterList(status, _gmicStatusQuotedParameters);
    settings.setValue(QString("LastExecution/host_%1/Arguments").arg(GmicQt::HostApplicationShortname), lastArguments);
    QString hash = settings.value(QString("LastExecution/host_%1/FilterHash").arg(GmicQt::HostApplicationShortname)).toString();
    ParametersCache::setValues(hash, status);
    ParametersCache::save();
  }
  if (_filterThread->failed()) {
    errorMessage = _filterThread->errorMessage();
  } else {
    gmic_list<gmic_pixel_type> images = _filterThread->images();
    if (!_filterThread->aborted()) {
      gmic_qt_output_images(images, _filterThread->imageNames(), _outputMode);
      _processingCompletedProperly = true;
    }
  }
  _filterThread->deleteLater();
  _filterThread = nullptr;
  _singleShotTimer.stop();
  emit done(errorMessage);
  if (!_hasProgressWindow && !errorMessage.isEmpty()) {
    Logger::error(errorMessage);
  }
  QCoreApplication::exit(0);
}

void HeadlessProcessor::cancel()
{
  if (_filterThread) {
    _filterThread->abortGmic();
  }
}

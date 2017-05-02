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
#include <QSettings>
#include <QDebug>
#include "Updater.h"
#include "Common.h"
#include "GmicStdlibParser.h"
#include "FilterThread.h"
#include "gmic.h"

#ifdef _IS_WINDOWS_
#include <process.h>
#include <psapi.h>
#endif

HeadlessProcessor::HeadlessProcessor(QObject *parent, const char *command, GmicQt::InputMode inputMode, GmicQt::OutputMode outputMode)
  : QObject(parent),
    _filterThread(0),
    _gmicImages(new cimg_library::CImgList<gmic_pixel_type>)
{
  _filterName = "Custom command";
  _lastCommand = "skip 0";
  _lastArguments = command;
  _outputMessageMode = GmicQt::Quiet;
  _inputMode = inputMode;
  _outputMode = outputMode;
  _lastEnvironment.clear();

  _timer.setInterval(250);
  connect(&_timer,SIGNAL(timeout()),
          this,SLOT(onTimeout()));
  _hasProgressWindow = false;
}

HeadlessProcessor::HeadlessProcessor(QObject *parent)
  : QObject(parent),
    _filterThread(0),
    _gmicImages(new cimg_library::CImgList<gmic_pixel_type>)
{
  QSettings settings;
  _filterName = settings.value(QString("LastExecution/host_%1/FilterName").arg(GmicQt::HostApplicationShortname)).toString();
  _lastCommand = settings.value(QString("LastExecution/host_%1/Command").arg(GmicQt::HostApplicationShortname)).toString();
  _lastArguments = settings.value(QString("LastExecution/host_%1/Arguments").arg(GmicQt::HostApplicationShortname)).toString();
  _outputMessageMode = (GmicQt::OutputMessageMode) settings.value(QString("LastExecution/host_%1/OutputMessageMode").arg(GmicQt::HostApplicationShortname),GmicQt::Quiet).toInt();
  _inputMode = (GmicQt::InputMode) settings.value(QString("LastExecution/host_%1/InputMode").arg(GmicQt::HostApplicationShortname),GmicQt::InputMode::Active).toInt();;
  _outputMode = (GmicQt::OutputMode) settings.value(QString("LastExecution/host_%1/OutputMode").arg(GmicQt::HostApplicationShortname),GmicQt::OutputMode::InPlace).toInt();;
  _lastEnvironment = settings.value(QString("LastExecution/host_%1/GmicEnvironment").arg(GmicQt::HostApplicationShortname), QString()).toString();
  _timer.setInterval(250);
  connect(&_timer,SIGNAL(timeout()),
          this,SLOT(onTimeout()));
  _singleShotTimer.setInterval(750);
  _singleShotTimer.setSingleShot(true);
  connect(&_singleShotTimer,SIGNAL(timeout()),
          this,SIGNAL(singleShotTimeout()));
  _hasProgressWindow = false;
}

HeadlessProcessor::~HeadlessProcessor()
{
  delete _gmicImages;
}

void HeadlessProcessor::startProcessing()
{
  _singleShotTimer.start();
  Updater::getInstance()->updateSources(false);
  GmicStdLibParser::GmicStdlib = Updater::getInstance()->buildFullStdlib();
  _gmicImages->assign();
  gmic_list<char> imageNames;
  gmic_qt_get_cropped_images(*_gmicImages,imageNames,-1,-1,-1,-1,_inputMode);
  if ( !_hasProgressWindow ) {
    gmic_qt_show_message(QString("G'MIC: %1").arg(_lastArguments).toUtf8().constData());
  }
  _filterThread = new FilterThread(this,
                                   _filterName,
                                   _lastCommand,
                                   _lastArguments,
                                   _lastEnvironment,
                                   _outputMessageMode);
  _filterThread->setInputImages(*_gmicImages,imageNames);
  connect(_filterThread,SIGNAL(finished()),
          this,SLOT(onProcessingFinished()));
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

void HeadlessProcessor::onTimeout()
{
  if ( !_filterThread ) {
    return;
  }
  float progress = _filterThread->progress();
  int ms = _filterThread->duration();
  unsigned long memory = 0;
#if defined(_IS_LINUX_)
  QFile status("/proc/self/status");
  if ( status.open(QFile::ReadOnly) ) {
    QByteArray text = status.readAll();
    const char * str = strstr(text.constData(),"VmRSS:");
    unsigned int kiB = 0;
    if ( str && sscanf(str + 7,"%u",&kiB) ) {
      memory = 1024 * (unsigned long)kiB;
    }
  }
#elif defined(_IS_WINDOWS_)
  PROCESS_MEMORY_COUNTERS counters;
  if (GetProcessMemoryInfo(GetCurrentProcess(),&counters,sizeof(counters))) {
    memory = static_cast<unsigned long>(counters.WorkingSetSize);
  }
#else
  // TODO: MACOS
#endif
  emit progression(progress,ms,memory);
}

void HeadlessProcessor::onProcessingFinished()
{
  QString errorMessage;
  _timer.stop();
  QStringList list = GmicStdLibParser::parseStatus(_filterThread->gmicStatus());
  if ( ! list.isEmpty() ) {
    QSettings settings;
    QString params = list.join(",");
    settings.setValue(QString("LastExecution/host_%1/Arguments").arg(GmicQt::HostApplicationShortname),params);
  }
  if ( _filterThread->failed() ) {
    errorMessage = _filterThread->errorMessage();
  } else {
    gmic_list<gmic_pixel_type> images = _filterThread->images();
    if ( !_filterThread->aborted() ) {
      gmic_qt_output_images(images,
                            _filterThread->imageNames(),
                            _outputMode,
                            (_outputMessageMode == GmicQt::VerboseLayerName) ?
                              QString("[G'MIC] %1: %2")
                              .arg(_filterThread->name())
                              .arg(_filterThread->fullCommand())
                              .toLatin1().constData()
                            : 0);
    }
  }
  _filterThread->deleteLater();
  _filterThread = 0;
  _singleShotTimer.stop();
  emit done(errorMessage);
  if ( !_hasProgressWindow && !errorMessage.isEmpty() ) {
    qWarning() << "Error:" << errorMessage;
  }
  qApp->exit(0);
}

void HeadlessProcessor::cancel()
{
  if ( _filterThread ) {
    _filterThread->abortGmic();
  }
}

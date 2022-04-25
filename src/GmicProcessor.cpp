/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file GmicProcessor.cpp
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

#include "GmicProcessor.h"
#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QRegExp>
#include <QSize>
#include <QString>
#include <cstring>
#include "CroppedActiveLayerProxy.h"
#include "CroppedImageListProxy.h"
#include "FilterSyncRunner.h"
#include "FilterThread.h"
#include "Globals.h"
#include "Host/GmicQtHost.h"
#include "ImageTools.h"
#include "LayersExtentProxy.h"
#include "Logger.h"
#include "Misc.h"
#include "OverrideCursor.h"
#include "gmic.h"

namespace GmicQt
{

GmicProcessor::GmicProcessor(QObject * parent) : QObject(parent)
{
  _filterThread = nullptr;
  _gmicImages = new cimg_library::CImgList<gmic_pixel_type>;
  _previewImage = new cimg_library::CImg<float>;
  _waitingCursorTimer.setSingleShot(true);
  connect(&_waitingCursorTimer, SIGNAL(timeout()), this, SLOT(showWaitingCursor()));
  cimg_library::cimg::srand();
  _previewRandomSeed = cimg_library::cimg::_rand();
  _lastAppliedCommandInOutState = InputOutputState::Unspecified;
  _filterExecutionTime.start();
  _completeFullImageProcessingCount = 0;
}

void GmicProcessor::init()
{
  abortCurrentFilterThread();
  _gmicImages->assign();
}

void GmicProcessor::setContext(const GmicProcessor::FilterContext & context)
{
  _filterContext = context;
}

void GmicProcessor::execute()
{
  gmic_list<char> imageNames;
  FilterContext::VisibleRect & rect = _filterContext.visibleRect;
  _gmicImages->assign();
  if ((_filterContext.requestType == FilterContext::RequestType::Preview) || //
      (_filterContext.requestType == FilterContext::RequestType::SynchronousPreview)) {
    if (_filterContext.previewFromFullImage) {
      CroppedImageListProxy::get(*_gmicImages, imageNames, 0.0, 0.0, 1.0, 1.0, _filterContext.inputOutputState.inputMode, 1.0);
      updateImageNames(imageNames);
    } else {
      CroppedImageListProxy::get(*_gmicImages, imageNames, rect.x, rect.y, rect.w, rect.h, _filterContext.inputOutputState.inputMode, _filterContext.zoomFactor);
      updateImageNames(imageNames);
    }
  } else {
    CroppedImageListProxy::get(*_gmicImages, imageNames, rect.x, rect.y, rect.w, rect.h, _filterContext.inputOutputState.inputMode, 1.0);
  }
  _waitingCursorTimer.start(WAITING_CURSOR_DELAY);
  const InputOutputState & io = _filterContext.inputOutputState;
  QString env = QString("_input_layers=%1").arg(static_cast<int>(io.inputMode));
  env += QString(" _output_mode=%1").arg(static_cast<int>(io.outputMode));
  env += QString(" _output_messages=%1").arg(static_cast<int>(_filterContext.outputMessageMode));
  if ((_filterContext.requestType == FilterContext::RequestType::Preview) || //
      (_filterContext.requestType == FilterContext::RequestType::SynchronousPreview)) {
    env += QString(" _preview_area_width=%1").arg(_filterContext.previewWindowWidth);
    env += QString(" _preview_area_height=%1").arg(_filterContext.previewWindowHeight);
    env += QString(" _preview_timeout=%1").arg(_filterContext.previewTimeout);
  }
  int maxWidth;
  int maxHeight;
  int preview_x0;
  int preview_y0;
  int preview_x1;
  int preview_y1;
  QSize previewSize;
  LayersExtentProxy::getExtent(_filterContext.inputOutputState.inputMode, maxWidth, maxHeight);
  if (_filterContext.previewFromFullImage) {
    preview_x0 = static_cast<int>(rect.x * maxWidth);
    preview_y0 = static_cast<int>(rect.y * maxHeight);
    preview_x1 = preview_x0 + std::min(maxWidth, static_cast<int>(1 + std::ceil(maxWidth * rect.w))) - 1;
    preview_y1 = preview_y0 + std::min(maxHeight, static_cast<int>(1 + std::ceil(maxHeight * rect.h))) - 1;
    previewSize = QSize(1 + preview_x1 - preview_x0, 1 + preview_y1 - preview_y0);
    if (_filterContext.zoomFactor < 1.0) {
      previewSize = QSize(static_cast<int>(std::round(previewSize.width() * _filterContext.zoomFactor)), //
                          static_cast<int>(std::round(previewSize.height() * _filterContext.zoomFactor)));
    }
  } else {
    if (_filterContext.zoomFactor < 1.0) {
      maxWidth = static_cast<int>(std::round(maxWidth * _filterContext.zoomFactor));
      maxHeight = static_cast<int>(std::round(maxHeight * _filterContext.zoomFactor));
    }
    preview_x0 = 0;
    preview_y0 = 0;
    preview_x1 = std::min(maxWidth, static_cast<int>(1 + std::ceil(maxWidth * rect.w))) - 1;
    preview_y1 = std::min(maxHeight, static_cast<int>(1 + std::ceil(maxHeight * rect.h))) - 1;
    previewSize = QSize(1 + preview_x1 - preview_x0, 1 + preview_y1 - preview_y0);
  }
  env += QString(" _preview_x0=%1").arg(preview_x0);
  env += QString(" _preview_y0=%1").arg(preview_y0);
  env += QString(" _preview_x1=%1").arg(preview_x1);
  env += QString(" _preview_y1=%1").arg(preview_y1);
  env += QString(" _preview_width=%1").arg(previewSize.width());
  env += QString(" _preview_height=%1").arg(previewSize.height());
  if (_filterContext.requestType == FilterContext::RequestType::SynchronousPreview) {
    FilterSyncRunner runner(this, _filterContext.filterCommand, _filterContext.filterArguments, env, _filterContext.outputMessageMode);
    runner.swapImages(*_gmicImages);
    runner.setImageNames(imageNames);
    runner.setLogSuffix("preview");
    cimg_library::cimg::srand();
    _previewRandomSeed = cimg_library::cimg::_rand();
    _filterExecutionTime.restart();
    runner.run();
    manageSynchonousRunner(runner);
    recordPreviewFilterExecutionDurationMS((int)_filterExecutionTime.elapsed());
  } else if (_filterContext.requestType == FilterContext::RequestType::Preview) {
    _filterThread = new FilterThread(this, _filterContext.filterCommand, _filterContext.filterArguments, env, _filterContext.outputMessageMode);
    _filterThread->swapImages(*_gmicImages);
    _filterThread->setImageNames(imageNames);
    _filterThread->setLogSuffix("preview");
    connect(_filterThread, SIGNAL(finished()), this, SLOT(onPreviewThreadFinished()), Qt::QueuedConnection);
    cimg_library::cimg::srand();
    _previewRandomSeed = cimg_library::cimg::_rand();
    _filterExecutionTime.restart();
    _filterThread->start();
  } else if (_filterContext.requestType == FilterContext::RequestType::FullImage) {
    _lastAppliedFilterHash = _filterContext.filterHash;
    _lastAppliedFilterPath = _filterContext.filterFullPath;
    _lastAppliedCommand = _filterContext.filterCommand;
    _lastAppliedCommandArguments = _filterContext.filterArguments;
    _lastAppliedCommandInOutState = _filterContext.inputOutputState;
    _filterThread = new FilterThread(this, _filterContext.filterCommand, _filterContext.filterArguments, env, _filterContext.outputMessageMode);
    _filterThread->swapImages(*_gmicImages);
    _filterThread->setImageNames(imageNames);
    _filterThread->setLogSuffix("apply");
    connect(_filterThread, SIGNAL(finished()), this, SLOT(onApplyThreadFinished()), Qt::QueuedConnection);
    cimg_library::cimg::srand(_previewRandomSeed);
    _filterThread->start();
  }
}

bool GmicProcessor::isProcessingFullImage() const
{
  return _filterContext.requestType == FilterContext::RequestType::FullImage;
}

bool GmicProcessor::isProcessing() const
{
  return _filterThread;
}

bool GmicProcessor::isIdle() const
{
  return !_filterThread;
}

int GmicProcessor::duration() const
{
  if (_filterThread) {
    return _filterThread->duration();
  }
  return 0;
}

float GmicProcessor::progress() const
{
  if (_filterThread) {
    return _filterThread->progress();
  }
  return 0.0f;
}

int GmicProcessor::lastPreviewFilterExecutionDurationMS() const
{
  if (_lastFilterPreviewExecutionDurations.empty()) {
    return 0;
  }
  return _lastFilterPreviewExecutionDurations.back();
}

void GmicProcessor::resetLastPreviewFilterExecutionDurations()
{
  _lastFilterPreviewExecutionDurations.clear();
}

void GmicProcessor::recordPreviewFilterExecutionDurationMS(int duration)
{
  _lastFilterPreviewExecutionDurations.push_back(duration);
  while (_lastFilterPreviewExecutionDurations.size() >= KEYPOINTS_INTERACTIVE_AVERAGING_COUNT) {
    _lastFilterPreviewExecutionDurations.pop_front();
  }
}

int GmicProcessor::averagePreviewFilterExecutionDuration() const
{
  if (_lastFilterPreviewExecutionDurations.empty()) {
    return 0;
  }
  int count = 0;
  double sum = 0;
  for (int duration : _lastFilterPreviewExecutionDurations) {
    sum += duration;
    ++count;
  }
  return static_cast<int>(sum / count);
}

int GmicProcessor::completedFullImageProcessingCount() const
{
  return _completeFullImageProcessingCount;
}

void GmicProcessor::cancel()
{
  abortCurrentFilterThread();
}

bool GmicProcessor::hasUnfinishedAbortedThreads() const
{
  return !_unfinishedAbortedThreads.isEmpty();
}

const cimg_library::CImg<float> & GmicProcessor::previewImage() const
{
  return *_previewImage;
}

const QStringList & GmicProcessor::gmicStatus() const
{
  return _gmicStatus;
}

void GmicProcessor::saveSettings(QSettings & settings)
{
  if (_lastAppliedCommand.isEmpty()) {
    const QString empty;
    settings.setValue(QString("LastExecution/host_%1/FilterHash").arg(GmicQtHost::ApplicationShortname), empty);
    settings.setValue(QString("LastExecution/host_%1/FilterPath").arg(GmicQtHost::ApplicationShortname), empty);
    settings.setValue(QString("LastExecution/host_%1/Command").arg(GmicQtHost::ApplicationShortname), empty);
    settings.setValue(QString("LastExecution/host_%1/Arguments").arg(GmicQtHost::ApplicationShortname), empty);
    settings.setValue(QString("LastExecution/host_%1/GmicStatusString").arg(GmicQtHost::ApplicationShortname), QString());
    settings.setValue(QString("LastExecution/host_%1/InputMode").arg(GmicQtHost::ApplicationShortname), 0);
    settings.setValue(QString("LastExecution/host_%1/OutputMode").arg(GmicQtHost::ApplicationShortname), 0);
  } else {
    settings.setValue(QString("LastExecution/host_%1/FilterPath").arg(GmicQtHost::ApplicationShortname), _lastAppliedFilterPath);
    settings.setValue(QString("LastExecution/host_%1/FilterHash").arg(GmicQtHost::ApplicationShortname), _lastAppliedFilterHash);
    settings.setValue(QString("LastExecution/host_%1/Command").arg(GmicQtHost::ApplicationShortname), _lastAppliedCommand);
    settings.setValue(QString("LastExecution/host_%1/Arguments").arg(GmicQtHost::ApplicationShortname), _lastAppliedCommandArguments);
    QString status = flattenGmicParameterList(_lastAppliedCommandGmicStatus, _gmicStatusQuotedParameters);
    settings.setValue(QString("LastExecution/host_%1/GmicStatusString").arg(GmicQtHost::ApplicationShortname), status);
    settings.setValue(QString("LastExecution/host_%1/InputMode").arg(GmicQtHost::ApplicationShortname), (int)_lastAppliedCommandInOutState.inputMode);
    settings.setValue(QString("LastExecution/host_%1/OutputMode").arg(GmicQtHost::ApplicationShortname), (int)_lastAppliedCommandInOutState.outputMode);
  }
}

GmicProcessor::~GmicProcessor()
{
  delete _gmicImages;
  delete _previewImage;
  if (!_unfinishedAbortedThreads.isEmpty()) {
    Logger::error(QString("~GmicProcessor(): There are %1 unfinished filter threads.").arg(_unfinishedAbortedThreads.size()));
  }
}

void GmicProcessor::setGmicStatusQuotedParameters(const QVector<bool> & quotedParameters)
{
  _gmicStatusQuotedParameters = quotedParameters;
}

void GmicProcessor::onPreviewThreadFinished()
{
  Q_ASSERT_X(_filterThread, __PRETTY_FUNCTION__, "No filter thread");
  if (_filterThread->isRunning()) {
    return;
  }
  if (_filterThread->failed()) {
    _gmicStatus.clear();
    _parametersVisibilityStates.clear();
    _gmicImages->assign();
    QString message = _filterThread->errorMessage();
    _filterThread->deleteLater();
    _filterThread = nullptr;
    hideWaitingCursor();
    emit previewCommandFailed(message);
    return;
  }
  _gmicStatus = _filterThread->gmicStatus();
  _parametersVisibilityStates = _filterThread->parametersVisibilityStates();
  _gmicImages->assign();
  _filterThread->swapImages(*_gmicImages);
  unsigned int badSpectrumIndex = 0;
  bool correctSpectrums = checkImageSpectrumAtMost4(*_gmicImages, badSpectrumIndex);
  if (correctSpectrums) {
    for (unsigned int i = 0; i < _gmicImages->size(); ++i) {
      GmicQtHost::applyColorProfile((*_gmicImages)[i]);
    }
    buildPreviewImage(*_gmicImages, *_previewImage);
  }
  _filterThread->deleteLater();
  _filterThread = nullptr;
  hideWaitingCursor();
  if (correctSpectrums) {
    emit previewImageAvailable();
    recordPreviewFilterExecutionDurationMS((int)_filterExecutionTime.elapsed());
  } else {
    QString message(tr("Image #%1 returned by filter has %2 channels (should be at most 4)"));
    emit previewCommandFailed(message.arg(badSpectrumIndex).arg((*_gmicImages)[badSpectrumIndex].spectrum()));
  }
}

void GmicProcessor::onApplyThreadFinished()
{
  Q_ASSERT_X(_filterThread, __PRETTY_FUNCTION__, "No filter thread");
  Q_ASSERT_X(!_filterThread->aborted(), __PRETTY_FUNCTION__, "Aborted thread!");
  if (_filterThread->isRunning()) {
    return;
  }
  _gmicStatus = _filterThread->gmicStatus();
  _parametersVisibilityStates = _filterThread->parametersVisibilityStates();
  hideWaitingCursor();

  if (_filterThread->failed()) {
    _lastAppliedFilterPath.clear();
    _lastAppliedCommand.clear();
    _lastAppliedCommandArguments.clear();
    QString message = _filterThread->errorMessage();
    _filterThread->deleteLater();
    _filterThread = nullptr;
    emit fullImageProcessingFailed(message);
  } else {
    _filterThread->swapImages(*_gmicImages);
    unsigned int badSpectrumIndex = 0;
    bool correctSpectrums = checkImageSpectrumAtMost4(*_gmicImages, badSpectrumIndex);
    if (!correctSpectrums) {
      _lastAppliedFilterPath.clear();
      _lastAppliedCommand.clear();
      _lastAppliedCommandArguments.clear();
      _filterThread->deleteLater();
      _filterThread = nullptr;
      QString message(tr("Image #%1 returned by filter has %2 channels\n(should be at most 4)"));
      emit fullImageProcessingFailed(message.arg(badSpectrumIndex).arg((*_gmicImages)[badSpectrumIndex].spectrum()));
    } else {
      if (GmicQtHost::ApplicationName.isEmpty()) {
        emit aboutToSendImagesToHost();
      }
      GmicQtHost::outputImages(*_gmicImages, _filterThread->imageNames(), _filterContext.inputOutputState.outputMode);
      _completeFullImageProcessingCount += 1;
      LayersExtentProxy::clear();
      CroppedActiveLayerProxy::clear();
      CroppedImageListProxy::clear();
      _filterThread->deleteLater();
      _filterThread = nullptr;
      _lastAppliedCommandGmicStatus = _gmicStatus; // TODO : save visibility states?
      emit fullImageProcessingDone();
    }
  }
}

void GmicProcessor::onAbortedThreadFinished()
{
  auto thread = dynamic_cast<FilterThread *>(sender());
  if (_unfinishedAbortedThreads.contains(thread)) {
    _unfinishedAbortedThreads.removeOne(thread);
    thread->deleteLater();
  }
  if (_unfinishedAbortedThreads.isEmpty()) {
    emit noMoreUnfinishedJobs();
  }
}

void GmicProcessor::showWaitingCursor()
{
  if (_filterThread) {
    OverrideCursor::setWaiting(true);
  }
}

void GmicProcessor::hideWaitingCursor()
{
  _waitingCursorTimer.stop();
  OverrideCursor::setWaiting(false);
}

void GmicProcessor::updateImageNames(gmic_list<char> & imageNames)
{
  const double & xFactor = _filterContext.positionStringCorrection.xFactor;
  const double & yFactor = _filterContext.positionStringCorrection.yFactor;
  int maxWidth;
  int maxHeight;
  LayersExtentProxy::getExtent(_filterContext.inputOutputState.inputMode, maxWidth, maxHeight);
  for (size_t i = 0; i < imageNames.size(); ++i) {
    gmic_image<char> & name = imageNames[i];
    QString str((const char *)name);
    QRegExp position("pos\\((\\d*)([^0-9]*)(\\d*)\\)");
    if (str.contains(position) && position.matchedLength() > 0) {
      int xPos = position.cap(1).toInt();
      int yPos = position.cap(3).toInt();
      int newXPos = (int)(xPos * (xFactor / (double)maxWidth));
      int newYPos = (int)(yPos * (yFactor / (double)maxHeight));
      str.replace(position.cap(0), QString("pos(%1%2%3)").arg(newXPos).arg(position.cap(2)).arg(newYPos));
      name.resize(str.size() + 1);
      std::memcpy(name.data(), str.toLatin1().constData(), name.width());
    }
  }
}

void GmicProcessor::abortCurrentFilterThread()
{
  if (!_filterThread) {
    return;
  }
  _filterThread->disconnect(this);
  connect(_filterThread, SIGNAL(finished()), this, SLOT(onAbortedThreadFinished()));
  _unfinishedAbortedThreads.push_back(_filterThread);
  _filterThread->abortGmic();
  _filterThread = nullptr;
  _waitingCursorTimer.stop();
  OverrideCursor::setWaiting(false);
}

void GmicProcessor::manageSynchonousRunner(FilterSyncRunner & runner)
{
  if (runner.failed()) {
    _gmicStatus.clear();
    _gmicImages->assign();
    QString message = runner.errorMessage();
    hideWaitingCursor();
    emit previewCommandFailed(message);
    return;
  }
  _gmicStatus = runner.gmicStatus();
  _parametersVisibilityStates = runner.parametersVisibilityStates();
  _gmicImages->assign();
  runner.swapImages(*_gmicImages);
  for (unsigned int i = 0; i < _gmicImages->size(); ++i) {
    GmicQtHost::applyColorProfile((*_gmicImages)[i]);
  }
  buildPreviewImage(*_gmicImages, *_previewImage);
  hideWaitingCursor();
  emit previewImageAvailable();
}

const QList<int> & GmicProcessor::parametersVisibilityStates() const
{
  return _parametersVisibilityStates;
}

} // namespace GmicQt

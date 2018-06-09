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
#include <QDebug>
#include <QPainter>
#include <QRegExp>
#include <QSize>
#include <QString>
#include <cstring>
#include "FilterThread.h"
#include "Host/host.h"
#include "ImageConverter.h"
#include "ImageTools.h"
#include "LayersExtentProxy.h"
#include "gmic.h"

GmicProcessor::GmicProcessor(QObject * parent) : QObject(parent)
{
  _filterThread = nullptr;
  _gmicImages = new cimg_library::CImgList<gmic_pixel_type>;
  _previewImage = new cimg_library::CImg<float>;
  _waitingCursorTimer.setSingleShot(true);
  connect(&_waitingCursorTimer, SIGNAL(timeout()), this, SLOT(showWaitingCursor()));
  _previewRandomSeed = cimg_library::cimg::srand();
  _lastAppliedCommandInOutState = GmicQt::InputOutputState::Unspecified;
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
  gmic_qt_get_cropped_images(*_gmicImages, imageNames, rect.x, rect.y, rect.w, rect.h, _filterContext.inputOutputState.inputMode);
  if (_filterContext.requestType == FilterContext::PreviewProcessing) {
    updateImageNames(imageNames);
    const double & zoomFactor = _filterContext.zoomFactor;
    if (zoomFactor < 1.0) {
      for (unsigned int i = 0; i < _gmicImages->size(); ++i) {
        gmic_image<float> & image = (*_gmicImages)[i];
        image.resize(std::round(image.width() * zoomFactor), std::round(image.height() * zoomFactor), 1, -100, 1);
      }
    }
  }
  _waitingCursorTimer.start(WAITING_CURSOR_DELAY);
  const GmicQt::InputOutputState & io = _filterContext.inputOutputState;
  QString env = QString("_input_layers=%1").arg(io.inputMode);
  env += QString(" _output_mode=%1").arg(io.outputMode);
  env += QString(" _output_messages=%1").arg(_filterContext.outputMessageMode);
  env += QString(" _preview_mode=%1").arg(io.previewMode);
  if (_filterContext.requestType == FilterContext::PreviewProcessing) {
    env += QString(" _preview_width=%1").arg(_filterContext.previewWidth);
    env += QString(" _preview_height=%1").arg(_filterContext.previewHeight);
    env += QString(" _preview_timeout=%1").arg(_filterContext.previewTimeout);
    _filterThread = new FilterThread(this, _filterContext.filterName, _filterContext.filterCommand, _filterContext.filterArguments, env, _filterContext.outputMessageMode);
    _filterThread->swapImages(*_gmicImages);
    _filterThread->setImageNames(imageNames);
    _filterThread->setLogSuffix("./preview/");
    connect(_filterThread, SIGNAL(finished()), this, SLOT(onPreviewThreadFinished()), Qt::QueuedConnection);
    _previewRandomSeed = cimg_library::cimg::srand();
  } else if (_filterContext.requestType == FilterContext::FullImageProcessing) {
    _lastAppliedFilterName = _filterContext.filterName;
    _lastAppliedCommand = _filterContext.filterCommand;
    _lastAppliedCommandArguments = _filterContext.filterArguments;
    _lastAppliedCommandEnv = env;
    _lastAppliedCommandInOutState = _filterContext.inputOutputState;
    _filterThread = new FilterThread(this, _filterContext.filterName, _filterContext.filterCommand, _filterContext.filterArguments, env, _filterContext.outputMessageMode);
    _filterThread->swapImages(*_gmicImages);
    _filterThread->setImageNames(imageNames);
    _filterThread->setLogSuffix("./apply/");
    connect(_filterThread, SIGNAL(finished()), this, SLOT(onApplyThreadFinished()), Qt::QueuedConnection);
    cimg_library::cimg::srand(_previewRandomSeed);
  }
  _filterThread->start();
}

bool GmicProcessor::isProcessingFullImage() const
{
  return _filterContext.requestType == FilterContext::FullImageProcessing;
}

bool GmicProcessor::isProcessing() const
{
  return _filterThread;
}

int GmicProcessor::duration() const
{
  if (_filterThread) {
    return _filterThread->duration();
  } else {
    return 0;
  }
}

float GmicProcessor::progress() const
{
  if (_filterThread) {
    return _filterThread->progress();
  } else {
    return 0.0f;
  }
}

void GmicProcessor::cancel()
{
  abortCurrentFilterThread();
}

bool GmicProcessor::hasUnfinishedAbortedThreads() const
{
  return _unfinishedAbortedThreads.size();
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
  settings.setValue(QString("LastExecution/host_%1/Command").arg(GmicQt::HostApplicationShortname), _lastAppliedCommand);
  settings.setValue(QString("LastExecution/host_%1/FilterName").arg(GmicQt::HostApplicationShortname), _lastAppliedFilterName);
  settings.setValue(QString("LastExecution/host_%1/Arguments").arg(GmicQt::HostApplicationShortname), _lastAppliedCommandArguments);
  settings.setValue(QString("LastExecution/host_%1/InputMode").arg(GmicQt::HostApplicationShortname), _lastAppliedCommandInOutState.inputMode);
  settings.setValue(QString("LastExecution/host_%1/OutputMode").arg(GmicQt::HostApplicationShortname), _lastAppliedCommandInOutState.outputMode);
  settings.setValue(QString("LastExecution/host_%1/PreviewMode").arg(GmicQt::HostApplicationShortname), _lastAppliedCommandInOutState.previewMode);
  settings.setValue(QString("LastExecution/host_%1/GmicEnvironment").arg(GmicQt::HostApplicationShortname), _lastAppliedCommandEnv);
}

GmicProcessor::~GmicProcessor()
{
  delete _gmicImages;
  delete _previewImage;
  if (_unfinishedAbortedThreads.size()) {
    qWarning() << QString("Error: ~GmicProcessor(): There are %1 unfinished filter threads.").arg(_unfinishedAbortedThreads.size());
  }
}

void GmicProcessor::onPreviewThreadFinished()
{
  ENTERING;
  Q_ASSERT_X(_filterThread, __PRETTY_FUNCTION__, "No filter thread");
  if (_filterThread->isRunning()) {
    return;
  }
  if (_filterThread->failed()) {
    _gmicStatus.clear();
    _gmicImages->assign();
    QString message = _filterThread->errorMessage();
    _filterThread->deleteLater();
    _filterThread = nullptr;
    hideWaitingCursor();
    emit previewCommandFailed(message);
    return;
  }
  _gmicStatus = _filterThread->gmicStatus();
  _gmicImages->assign();
  _filterThread->swapImages(*_gmicImages);
  for (unsigned int i = 0; i < _gmicImages->size(); ++i) {
    gmic_qt_apply_color_profile((*_gmicImages)[i]);
  }
  GmicQt::buildPreviewImage(*_gmicImages, *_previewImage, _filterContext.inputOutputState.previewMode, _filterContext.previewWidth, _filterContext.previewHeight);
  _filterThread->deleteLater();
  _filterThread = nullptr;
  hideWaitingCursor();
  emit previewImageAvailable();
}

void GmicProcessor::onApplyThreadFinished()
{
  Q_ASSERT_X(_filterThread, __PRETTY_FUNCTION__, "No filter thread");
  Q_ASSERT_X(!_filterThread->aborted(), __PRETTY_FUNCTION__, "Aborted thread!");
  if (_filterThread->isRunning()) {
    return;
  }
  _gmicStatus = _filterThread->gmicStatus();
  hideWaitingCursor();

  if (_filterThread->failed()) {
    _lastAppliedFilterName.clear();
    _lastAppliedCommand.clear();
    _lastAppliedCommandArguments.clear();
    QString message = _filterThread->errorMessage();
    _filterThread->deleteLater();
    _filterThread = nullptr;
    emit fullImageProcessingFailed(message);
  } else {
    _filterThread->swapImages(*_gmicImages);
    if (_filterContext.outputMessageMode == GmicQt::VerboseLayerName) {
      QString label = QString("[G'MIC] %1: %2").arg(_filterThread->name()).arg(_filterThread->fullCommand());
      gmic_qt_output_images(*_gmicImages, _filterThread->imageNames(), _filterContext.inputOutputState.outputMode, label.toLocal8Bit().constData());
    } else {
      gmic_qt_output_images(*_gmicImages, _filterThread->imageNames(), _filterContext.inputOutputState.outputMode, 0);
    }
    _filterThread->deleteLater();
    _filterThread = nullptr;
    emit fullImageProcessingDone();
  }
}

void GmicProcessor::onAbortedThreadFinished()
{
  TSHOW(_unfinishedAbortedThreads.size());
  FilterThread * thread = dynamic_cast<FilterThread *>(sender());
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
  if (_filterThread && !(QApplication::overrideCursor() && QApplication::overrideCursor()->shape() == Qt::WaitCursor)) {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  }
}

void GmicProcessor::hideWaitingCursor()
{
  ENTERING;
  _waitingCursorTimer.stop();
  if (QApplication::overrideCursor() && QApplication::overrideCursor()->shape() == Qt::WaitCursor) {
    QApplication::restoreOverrideCursor();
  }
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
      int newXPos = (int)(xPos * (xFactor / (float)maxWidth));
      int newYPos = (int)(yPos * (yFactor / (float)maxHeight));
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
  _filterThread = 0;
  _waitingCursorTimer.stop();
  if (QApplication::overrideCursor() && QApplication::overrideCursor()->shape() == Qt::WaitCursor) {
    QApplication::restoreOverrideCursor();
  }
}

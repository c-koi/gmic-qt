/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file PreviewWidget.cpp
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
#include "Widgets/PreviewWidget.h"
#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <algorithm>
#include <functional>
#include "Common.h"
#include "DialogSettings.h"
#include "Globals.h"
#include "GmicStdlib.h"
#include "ImageConverter.h"
#include "ImageTools.h"
#include "LayersExtentProxy.h"
#include "Logger.h"
#include "Utils.h"
#include "gmic.h"

const PreviewWidget::PreviewRect PreviewWidget::PreviewRect::Full{0.0, 0.0, 1.0, 1.0};

PreviewWidget::PreviewWidget(QWidget * parent) : QWidget(parent), _cachedOriginalImage(new gmic_image<float>())
{
  setAutoFillBackground(false);
  _image = new cimg_library::CImg<float>;
  _image->assign();
  _savedPreview = new cimg_library::CImg<float>;
  _savedPreview->assign();
  _transparency.load(":resources/transparency.png");

  _visibleRect = PreviewRect::Full;
  saveVisibleCenter();

  _cachedOriginalImagePosition = {-1.0, -1.0, -1.0, -1.0};

  _pendingResize = false;
  _previewEnabled = true;
  _currentZoomFactor = 1.0;
  _timerID = 0;
  _savedPreviewIsValid = false;
  _paintOriginalImage = true;
  qApp->installEventFilter(this);
  _rightClickEnabled = false;
  _originalImageSize = QSize(-1, -1);
  _movedKeypointOrigin = QPoint(-1, -1);
  _movedKeypointIndex = -1;
}

PreviewWidget::~PreviewWidget()
{
  delete _image;
  delete _savedPreview;
}

const cimg_library::CImg<float> & PreviewWidget::image() const
{
  return *_image;
}

void PreviewWidget::setPreviewImage(const cimg_library::CImg<float> & image)
{
  _errorMessage.clear();
  _errorImage = QImage();
  _overlayMessage.clear();
  *_image = image;
  *_savedPreview = image;
  _savedPreviewIsValid = true;
  updateOriginalImagePosition();
  _paintOriginalImage = false;
  if (isAtFullZoom()) {
    if (_fullImageSize.isNull()) {
      _currentZoomFactor = 1.0;
    } else {
      _currentZoomFactor = std::min(width() / (double)_fullImageSize.width(), height() / (double)_fullImageSize.height());
    }
    emit zoomChanged(_currentZoomFactor);
  }
  update();
}

void PreviewWidget::setOverlayMessage(const QString & message)
{
  _overlayMessage = message;
  _paintOriginalImage = false;
  update();
}

void PreviewWidget::clearOverlayMessage()
{
  _overlayMessage.clear();
  _paintOriginalImage = false;
  update();
}

void PreviewWidget::setPreviewErrorMessage(const QString & message)
{
  _errorMessage = message;
  _errorImage = QImage();
  updateErrorImage();
  _paintOriginalImage = false;
  update();
}

void PreviewWidget::setFullImageSize(const QSize & size)
{
  _fullImageSize = size;
  _cachedOriginalImagePosition = {-1.0, -1.0, -1.0, -1.0};
  updateVisibleRect();
  saveVisibleCenter();
}

void PreviewWidget::updateFullImageSizeIfDifferent(const QSize & size)
{
  if (size != _fullImageSize) {
    setFullImageSize(size);
  } else {
    // Make sure the cached image is no longer used, even if its size is the same
    _cachedOriginalImagePosition = {-1.0, -1.0, -1.0, -1.0};
  }
}

void PreviewWidget::updateVisibleRect()
{
  if (_fullImageSize.isNull()) {
    _visibleRect = PreviewRect::Full;
  } else {
    _visibleRect.w = std::min(1.0, (width() / (_currentZoomFactor * _fullImageSize.width())));
    _visibleRect.h = std::min(1.0, (height() / (_currentZoomFactor * _fullImageSize.height())));
    _visibleRect.x = std::min(_visibleRect.x, 1.0 - _visibleRect.w);
    _visibleRect.y = std::min(_visibleRect.y, 1.0 - _visibleRect.h);
  }
}

void PreviewWidget::centerVisibleRect()
{
  _visibleRect.moveToCenter();
}

void PreviewWidget::updateOriginalImagePosition()
{
  if (_fullImageSize.isNull()) {
    _originalImageSize = QSize(0, 0);
    _originaImageScaledSize = QSize(0, 0);
    _imagePosition = rect();
    return;
  }
  _originalImageSize = originalImageCropSize();
  if (_currentZoomFactor > 1.0) {
    _originaImageScaledSize = _originalImageSize;
    QSize imageSize(std::round(_originalImageSize.width() * _currentZoomFactor), std::round(_originalImageSize.height() * _currentZoomFactor));
    int left, top;
    if (imageSize.height() > height()) {
      top = -(int)(((_visibleRect.y * _fullImageSize.height()) - std::floor(_visibleRect.y * _fullImageSize.height())) * _currentZoomFactor);
    } else {
      top = (height() - imageSize.height()) / 2;
    }
    if (imageSize.width() > width()) {
      left = -(int)(((_visibleRect.x * _fullImageSize.width()) - std::floor(_visibleRect.x * _fullImageSize.width())) * _currentZoomFactor);
    } else {
      left = (width() - imageSize.width()) / 2;
    }
    _imagePosition = QRect(QPoint(left, top), imageSize);
  } else {
    _originaImageScaledSize = QSize(std::round(_originalImageSize.width() * _currentZoomFactor), std::round(_originalImageSize.height() * _currentZoomFactor));
    _imagePosition = QRect(QPoint(std::max(0, (width() - _originaImageScaledSize.width()) / 2), std::max(0, (height() - _originaImageScaledSize.height()) / 2)), _originaImageScaledSize);
  }
}

void PreviewWidget::updateErrorImage()
{
  cimg_library::CImgList<float> images;
  cimg_library::CImgList<char> imageNames;
  images.assign();
  imageNames.assign();
  gmic_image<float> image;
  getOriginalImageCrop(image);
  image.move_to(images);
  QString fullCommandLine = QString::fromLocal8Bit(GmicQt::commandFromOutputMessageMode(DialogSettings::outputMessageMode()));
  fullCommandLine += QString(" _host=%1 _tk=qt").arg(GmicQt::HostApplicationShortname);
  fullCommandLine += QString(" _preview_width=%1").arg(width());
  fullCommandLine += QString(" _preview_height=%1").arg(height());
  fullCommandLine += QString(" gui_error_preview \"%2\"").arg(_errorMessage);
  try {
    gmic(fullCommandLine.toLocal8Bit().constData(), images, imageNames, GmicStdLib::Array.constData(), true);
  } catch (...) {
    images.assign();
    imageNames.assign();
  }
  if (!images.size() || !images.front()) {
    _errorImage = QImage(size(), QImage::Format_ARGB32);
    _errorImage.fill(QColor(40, 40, 40));
    QPainter painter(&_errorImage);
    painter.setPen(Qt::green);
    painter.drawText(QRect(0, 0, _errorImage.width(), _errorImage.height()), Qt::AlignCenter | Qt::TextWordWrap, _errorMessage);
    return;
  }
  QImage qimage;
  ImageConverter::convert(images.front(), qimage);
  if (qimage.size() != size()) {
    _errorImage = qimage.scaled(size());
  } else {
    _errorImage = qimage;
  }
}

void PreviewWidget::paintKeypoints(QPainter & painter)
{
  QPen pen;
  pen.setColor(Qt::black);
  pen.setWidth(2);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setPen(pen);
  KeypointList::reverse_iterator it = _keypoints.rbegin();
  while (it != _keypoints.rend()) {
    if (!it->isNaN()) {
      QPoint center = keypointToVisiblePointInWidget(*it);
      QPoint realCenter = keypointToPointInWidget(*it);
      Logger::log(QString("center=%1,%2 real=%3,%4\n").arg(center.x()).arg(center.y()).arg(realCenter.x()).arg(realCenter.y()));
      QRect r(0, 0, 11, 11);
      r.moveCenter(center);
      if (center == realCenter) {
        painter.setBrush(it->color);
        pen.setStyle(Qt::SolidLine);
      } else {
        painter.setBrush(it->color.darker(150));
        pen.setStyle(Qt::DotLine);
      }
      pen.setColor(QColor(0, 0, 0, it->color.alpha()));
      painter.setPen(pen);
      painter.drawEllipse(r);
    }
    ++it;
  }
}

int PreviewWidget::keypointUnderMouse(const QPoint & p)
{
  KeypointList::iterator it = _keypoints.begin();
  int index = 0;
  while (it != _keypoints.end()) {
    if (!it->isNaN()) {
      const KeypointList::Keypoint & kp = *it;
      QPoint center = keypointToVisiblePointInWidget(kp);
      if ((center - p).manhattanLength() < 10) {
        return index;
      }
    }
    ++it;
    ++index;
  }
  return -1;
}

QPoint PreviewWidget::keypointToPointInWidget(const KeypointList::Keypoint & kp) const
{
  return QPoint(std::round(_imagePosition.left() + _imagePosition.width() * (kp.x / 100.0f)), std::round(_imagePosition.top() + _imagePosition.height() * (kp.y / 100.0f)));
}

QPoint PreviewWidget::keypointToVisiblePointInWidget(const KeypointList::Keypoint & kp) const
{
  QPoint p = keypointToPointInWidget(kp);
  p.rx() = std::max(_imagePosition.left() + 1, std::min(p.x(), _imagePosition.left() + _imagePosition.width() - 2));
  p.ry() = std::max(_imagePosition.top() + 1, std::min(p.y(), _imagePosition.top() + _imagePosition.height() - 2));
  return p;
}

QPointF PreviewWidget::pointInWidgetToKeypointPosition(const QPoint & p) const
{
  QPointF result(100.0 * (p.x() - _imagePosition.left()) / (float)_imagePosition.width(), 100.0 * (p.y() - _imagePosition.top()) / (float)_imagePosition.height());
  result.rx() = std::min(300.0, std::max(-200.0, result.x()));
  result.ry() = std::min(300.0, std::max(-200.0, result.y()));
  return result;
}

void PreviewWidget::paintPreview(QPainter & painter)
{
  if (!_overlayMessage.isEmpty()) {
    paintOriginalImage(painter);
    painter.fillRect(_imagePosition, QColor(40, 40, 40, 150));
    painter.setPen(Qt::green);
    painter.drawText(_imagePosition, Qt::AlignCenter | Qt::TextWordWrap, _overlayMessage);
    return;
  }
  if (!_errorMessage.isEmpty()) {
    if (_errorImage.isNull() || (_errorImage.size() != size())) {
      updateErrorImage();
    }
    painter.drawImage(0, 0, _errorImage);
    return;
  }

  if (!_image->width() && !_image->height()) {
    painter.fillRect(rect(), QBrush(_transparency));
    paintKeypoints(painter);
    return;
  }
  /*  If preview image has a size different from the original image crop, or
   *  we are at "full image" zoom of an image smaller than the widget,
   *  then the image should fit the widget size.
   */
  const QSize previewImageSize(_image->width(), _image->height());
  if ((previewImageSize != _originaImageScaledSize) || (isAtFullZoom() && _currentZoomFactor > 1.0)) {
    QSize imageSize;
    if (previewImageSize != _originaImageScaledSize) {
      imageSize = previewImageSize.scaled(width(), height(), Qt::KeepAspectRatio);
    } else {
      imageSize = QSize(std::round(_originalImageSize.width() * _currentZoomFactor), std::round(_originalImageSize.height() * _currentZoomFactor));
    }
    _imagePosition = QRect(QPoint(std::max(0, (width() - imageSize.width()) / 2), std::max(0, (height() - imageSize.height()) / 2)), imageSize);
    _originaImageScaledSize = QSize(-1, -1); // Make sure next preview update will not consider originaImageScaledSize
  }
  /*
   *  Otherwise : Preview size == Original scaled size and image position is therefore unchanged
   */

  if (hasAlphaChannel(*_image)) {
    painter.fillRect(_imagePosition, QBrush(_transparency));
  }
  QImage qimage;
  ImageConverter::convert(_image->get_resize(_imagePosition.width(), _imagePosition.height(), 1, -100, 1), qimage);
  painter.drawImage(_imagePosition, qimage);
  paintKeypoints(painter);
}

void PreviewWidget::paintOriginalImage(QPainter & painter)
{
  gmic_image<float> image;
  getOriginalImageCrop(image);
  updateOriginalImagePosition();
  if (!image.width() && !image.height()) {
    painter.fillRect(rect(), QBrush(_transparency));
  } else {
    image.resize(_imagePosition.width(), _imagePosition.height(), 1, -100, 1);
    if (hasAlphaChannel(image)) {
      painter.fillRect(_imagePosition, QBrush(_transparency));
    }
    QImage qimage;
    ImageConverter::convert(image, qimage);
    painter.drawImage(_imagePosition, qimage);
    paintKeypoints(painter);
  }
}

void PreviewWidget::paintEvent(QPaintEvent * e)
{
  TIMING;
  QPainter painter(this);
  if (_paintOriginalImage) {
    paintOriginalImage(painter);
  } else {
    paintPreview(painter);
  }
  e->accept();
}

void PreviewWidget::resizeEvent(QResizeEvent * e)
{
  if (isVisible()) {
    _pendingResize = true;
  }
  e->accept();
  if (!e->size().width() || !e->size().height()) {
    return;
  }
  if (isAtFullZoom()) {
    if (_fullImageSize.isNull()) {
      _currentZoomFactor = 1.0;
    } else {
      _currentZoomFactor = std::min(e->size().width() / (double)_fullImageSize.width(), e->size().height() / (double)_fullImageSize.height());
    }
    emit zoomChanged(_currentZoomFactor);
  } else {
    updateVisibleRect();
    saveVisibleCenter();
  }
  if (QApplication::topLevelWidgets().size() && QApplication::topLevelWidgets().at(0)->isMaximized()) {
    sendUpdateRequest();
  } else {
    displayOriginalImage();
  }
}

void PreviewWidget::normalizedVisibleRect(double & x, double & y, double & width, double & height) const
{
  x = _visibleRect.x;
  y = _visibleRect.y;
  width = _visibleRect.w;
  height = _visibleRect.h;
}

void PreviewWidget::sendUpdateRequest()
{
  invalidateSavedPreview();
  emit previewUpdateRequested();
}

bool PreviewWidget::isAtDefaultZoom() const
{
  return (_previewFactor == GmicQt::PreviewFactorAny) || (std::abs(_currentZoomFactor - defaultZoomFactor()) < 0.05) ||
         ((_previewFactor == GmicQt::PreviewFactorActualSize) && (_currentZoomFactor >= 1.0));
}

double PreviewWidget::currentZoomFactor() const
{
  return _currentZoomFactor;
}

bool PreviewWidget::event(QEvent * event)
{
  if (event->type() == QEvent::WindowActivate && _pendingResize) {
    _pendingResize = false;
    if (width() && height()) {
      updateVisibleRect();
      saveVisibleCenter();
      sendUpdateRequest();
    }
  }
  return QWidget::event(event);
}

bool PreviewWidget::eventFilter(QObject *, QEvent * event)
{
  if (((event->type() == QEvent::MouseButtonRelease) || (event->type() == QEvent::NonClientAreaMouseButtonRelease)) && _pendingResize) {
    _pendingResize = false;
    if (width() && height()) {
      updateVisibleRect();
      saveVisibleCenter();
      sendUpdateRequest();
    }
  }
  return false;
}

void PreviewWidget::wheelEvent(QWheelEvent * event)
{
  double degrees = event->angleDelta().y() / 8.0;
  // int steps = static_cast<int>(std::fabs(degrees) / 15.0);
  int steps = static_cast<int>(std::fabs(degrees) / 15.0);
  if (degrees > 0.0) {
    zoomIn(event->pos() - _imagePosition.topLeft(), steps);
  } else {
    zoomOut(event->pos() - _imagePosition.topLeft(), steps);
  }
  event->accept();
}

void PreviewWidget::mousePressEvent(QMouseEvent * e)
{
  if (e->button() == Qt::LeftButton || e->button() == Qt::MiddleButton) {
    if (_imagePosition.contains(e->pos())) {
      int index = keypointUnderMouse(e->pos());
      if (index != -1) {
        _movedKeypointIndex = index;
        _keypointTimestamp = e->timestamp();
      } else {
        _mousePosition = e->pos();
      }
      abortUpdateTimer();
    } else {
      _mousePosition = QPoint(-1, -1);
    }
    e->accept();
    return;
  }

  if (_rightClickEnabled && (e->button() == Qt::RightButton)) {
    if (_imagePosition.contains(e->pos())) {
      _movedKeypointIndex = keypointUnderMouse(e->pos());
      _movedKeypointOrigin = e->pos();
    }
    if (_previewEnabled) {
      displayOriginalImage();
    }
    e->accept();
    return;
  }

  e->ignore();
}

void PreviewWidget::mouseReleaseEvent(QMouseEvent * e)
{
  if (e->button() == Qt::LeftButton || e->button() == Qt::MiddleButton) {
    if (!isAtFullZoom() && _mousePosition != QPoint(-1, -1)) {
      QPoint move = _mousePosition - e->pos();
      onMouseTranslationInImage(move);
      sendUpdateRequest();
      _mousePosition = QPoint(-1, -1);
    }
    if (_movedKeypointIndex != -1) {
      QPointF p = pointInWidgetToKeypointPosition(e->pos());
      KeypointList::Keypoint & kp = _keypoints[_movedKeypointIndex];
      kp.setPosition(p);
      _movedKeypointIndex = -1;
      // TODO: Check that this is OK
      //      if (kp.burst) {
      //        emit keypointPositionsChanged(KeypointBurstEvent, e->timestamp());
      //      } else {
      emit keypointPositionsChanged(KeypointMouseReleaseEvent, e->timestamp());
      //      }
    }
    e->accept();
    return;
  }

  if (e->button() == Qt::RightButton) {
    if (_movedKeypointIndex != -1 && (_movedKeypointOrigin != e->pos())) {
      emit keypointPositionsChanged(KeypointMouseReleaseEvent, e->timestamp());
    }
    _movedKeypointIndex = -1;
    _movedKeypointOrigin = QPoint(-1, -1);
  }

  if (_rightClickEnabled && _paintOriginalImage && (e->button() == Qt::RightButton)) {
    if (_previewEnabled) {
      if (!_errorImage.isNull()) {
        _paintOriginalImage = false;
        update();
      } else if (_savedPreviewIsValid) {
        restorePreview();
        _paintOriginalImage = false;
        update();
      } else {
        displayOriginalImage();
      }
    }
    e->accept();
    return;
  }
}

void PreviewWidget::mouseMoveEvent(QMouseEvent * e)
{
  if (e->buttons() & (Qt::LeftButton | Qt::MiddleButton)) {
    if (!isAtFullZoom() && (_mousePosition != QPoint(-1, -1))) {
      QPoint move = _mousePosition - e->pos();
      if (move.manhattanLength()) {
        onMouseTranslationInImage(move);
        _mousePosition = e->pos();
      }
    }
    if (_movedKeypointIndex != -1) {
      QPointF p = pointInWidgetToKeypointPosition(e->pos());
      KeypointList::Keypoint & kp = _keypoints[_movedKeypointIndex];
      kp.setPosition(p);
      update();
      if (kp.burst) {
        unsigned char flags = 0;
        if (e->timestamp() - _keypointTimestamp > 15) {
          flags |= KeypointBurstEvent;
        }
        emit keypointPositionsChanged(flags, e->timestamp());
        _keypointTimestamp = e->timestamp();
      } else {
        emit keypointPositionsChanged(0, e->timestamp());
      }
    }
    e->accept();
  } else if (e->buttons() & Qt::RightButton) {
    if (_movedKeypointIndex != -1) {
      QPointF p = pointInWidgetToKeypointPosition(e->pos());
      KeypointList::Keypoint & kp = _keypoints[_movedKeypointIndex];
      kp.setPosition(p);
      update();
      emit keypointPositionsChanged(0, e->timestamp());
    }
  } else {
    e->ignore();
  }
}

bool PreviewWidget::isAtFullZoom() const
{
  return _visibleRect.isFull();
}

void PreviewWidget::abortUpdateTimer()
{
  if (_timerID) {
    killTimer(_timerID);
    _timerID = 0;
  }
}

void PreviewWidget::getPositionStringCorrection(double & xFactor, double & yFactor) const
{
  xFactor = _currentZoomFactor * (_visibleRect.w * _fullImageSize.width());
  yFactor = _currentZoomFactor * (_visibleRect.h * _fullImageSize.height());
}

void PreviewWidget::onMouseTranslationInImage(QPoint shift)
{
  if (shift.manhattanLength()) {
    emit previewVisibleRectIsChanging();
    translateFullImage(shift.x() / _currentZoomFactor, shift.y() / _currentZoomFactor);
    displayOriginalImage();
  }
}

void PreviewWidget::translateFullImage(double dx, double dy)
{
  PreviewPoint previousPosition = _visibleRect.topLeft();
  if (!_fullImageSize.isNull()) {
    translateNormalized(dx / _fullImageSize.width(), dy / _fullImageSize.height());
    if (_visibleRect.topLeft() != previousPosition) {
      saveVisibleCenter();
    }
  }
}

void PreviewWidget::translateNormalized(double dx, double dy)
{
  _visibleRect.x = std::max(0.0, std::min(1.0 - _visibleRect.w, _visibleRect.x + dx));
  _visibleRect.y = std::max(0.0, std::min(1.0 - _visibleRect.h, _visibleRect.y + dy));
}

void PreviewWidget::zoomIn()
{
  zoomIn(_imagePosition.center(), 1);
}

void PreviewWidget::zoomOut()
{
  zoomOut(_imagePosition.center(), 1);
}

void PreviewWidget::zoomFullImage()
{
  _visibleRect = PreviewRect::Full;
  if (_fullImageSize.isNull()) {
    _currentZoomFactor = 1.0;
  } else {
    _currentZoomFactor = std::min(width() / (double)_fullImageSize.width(), height() / (double)_fullImageSize.height());
  }
  onPreviewParametersChanged();
  emit zoomChanged(_currentZoomFactor);
}

void PreviewWidget::zoomIn(QPoint p, int steps)
{
  if (_fullImageSize.isNull()) {
    return;
  }
  double previousZoomFactor = _currentZoomFactor;
  if (_currentZoomFactor >= PREVIEW_MAX_ZOOM_FACTOR) {
    return;
  }
  double mouseX = p.x() / (_currentZoomFactor * _fullImageSize.width()) + _visibleRect.x;
  double mouseY = p.y() / (_currentZoomFactor * _fullImageSize.height()) + _visibleRect.y;
  while (steps--) {
    _currentZoomFactor *= 1.2;
  }
  if (_currentZoomFactor >= PREVIEW_MAX_ZOOM_FACTOR) {
    _currentZoomFactor = PREVIEW_MAX_ZOOM_FACTOR;
  }
  if (_currentZoomFactor == previousZoomFactor) {
    return;
  }
  updateVisibleRect();
  double newMouseX = p.x() / (_currentZoomFactor * _fullImageSize.width()) + _visibleRect.x;
  double newMouseY = p.y() / (_currentZoomFactor * _fullImageSize.height()) + _visibleRect.y;
  translateNormalized(mouseX - newMouseX, mouseY - newMouseY);
  saveVisibleCenter();
  onPreviewParametersChanged();
  emit zoomChanged(_currentZoomFactor);
}

void PreviewWidget::zoomOut(QPoint p, int steps)
{
  if (isAtFullZoom() || _fullImageSize.isNull()) {
    return;
  }
  double mouseX = p.x() / (_currentZoomFactor * _fullImageSize.width()) + _visibleRect.x;
  double mouseY = p.y() / (_currentZoomFactor * _fullImageSize.height()) + _visibleRect.y;
  while (steps--) {
    _currentZoomFactor /= 1.2;
  }
  updateVisibleRect();
  if (isAtFullZoom()) {
    _currentZoomFactor = std::min(width() / (double)_fullImageSize.width(), height() / (double)_fullImageSize.height());
  }
  double newMouseX = p.x() / (_currentZoomFactor * _fullImageSize.width()) + _visibleRect.x;
  double newMouseY = p.y() / (_currentZoomFactor * _fullImageSize.height()) + _visibleRect.y;
  translateNormalized(mouseX - newMouseX, mouseY - newMouseY);
  saveVisibleCenter();
  onPreviewParametersChanged();
  emit zoomChanged(_currentZoomFactor);
}

void PreviewWidget::setZoomLevel(double zoom)
{
  if ((zoom == _currentZoomFactor) || _fullImageSize.isNull()) {
    return;
  }
  if ((zoom > PREVIEW_MAX_ZOOM_FACTOR) || (isAtFullZoom() && (zoom < _currentZoomFactor))) {
    emit zoomChanged(_currentZoomFactor);
    return;
  }
  double previousZoomFactor = _currentZoomFactor;
  QPoint p = _imagePosition.center();
  double mouseX = p.x() / (_currentZoomFactor * _fullImageSize.width()) + _visibleRect.x;
  double mouseY = p.y() / (_currentZoomFactor * _fullImageSize.height()) + _visibleRect.y;
  _currentZoomFactor = zoom;
  updateVisibleRect();
  if (isAtFullZoom()) {
    _currentZoomFactor = std::min(width() / (double)_fullImageSize.width(), height() / (double)_fullImageSize.height());
  }
  if (_currentZoomFactor == previousZoomFactor) {
    return;
  }
  double newMouseX = p.x() / (_currentZoomFactor * _fullImageSize.width()) + _visibleRect.x;
  double newMouseY = p.y() / (_currentZoomFactor * _fullImageSize.height()) + _visibleRect.y;
  translateNormalized(mouseX - newMouseX, mouseY - newMouseY);
  saveVisibleCenter();
  onPreviewParametersChanged();
  emit zoomChanged(_currentZoomFactor);
}

double PreviewWidget::defaultZoomFactor() const
{
  if (_fullImageSize.isNull()) {
    return 1.0;
  }
  if (_previewFactor == GmicQt::PreviewFactorFullImage) {
    return std::min(width() / (double)_fullImageSize.width(), height() / (double)_fullImageSize.height());
  }
  if (_previewFactor > 1.0) {
    return _previewFactor * std::min(width() / (double)_fullImageSize.width(), height() / (double)_fullImageSize.height());
  }
  return 1.0; // We suppose GmicQt::PreviewFactorActualSize
}

void PreviewWidget::saveVisibleCenter()
{
  _savedVisibleCenter = _visibleRect.center();
}

void PreviewWidget::setPreviewFactor(float filterFactor, bool reset)
{
  _previewFactor = filterFactor;
  if (_fullImageSize.isNull()) {
    _visibleRect = PreviewRect::Full;
    _currentZoomFactor = 1.0;
    emit zoomChanged(_currentZoomFactor);
    return;
  }
  if ((_previewFactor == GmicQt::PreviewFactorFullImage) || ((_previewFactor == GmicQt::PreviewFactorAny) && reset)) {
    _currentZoomFactor = std::min(width() / (double)_fullImageSize.width(), height() / (double)_fullImageSize.height());
    _visibleRect = PreviewRect::Full;
  } else if ((_previewFactor == GmicQt::PreviewFactorAny) && !reset) {
    updateVisibleRect();
    _visibleRect.moveCenter(_savedVisibleCenter);
  } else {
    _currentZoomFactor = defaultZoomFactor();
    updateVisibleRect();
    if (reset) {
      _visibleRect.moveToCenter();
    } else {
      _visibleRect.moveCenter(_savedVisibleCenter);
    }
  }
  emit zoomChanged(_currentZoomFactor);
}

void PreviewWidget::displayOriginalImage()
{
  _paintOriginalImage = true;
  update();
}

QSize PreviewWidget::originalImageCropSize()
{
  if (_visibleRect != _cachedOriginalImagePosition) {
    updateCachedOriginalImageCrop();
  }
  return QSize(_cachedOriginalImage->width(), _cachedOriginalImage->height());
}

void PreviewWidget::updateCachedOriginalImageCrop()
{
  gmic_list<float> images;
  gmic_list<char> imageNames;
  gmic_qt_get_cropped_images(images, imageNames, _visibleRect.x, _visibleRect.y, _visibleRect.w, _visibleRect.h, GmicQt::Active);
  if (images.size() > 0) {
    gmic_qt_apply_color_profile(images[0]);
    _cachedOriginalImage->swap(images[0]);
    _cachedOriginalImagePosition = _visibleRect;
  } else {
    _cachedOriginalImage->assign();
    _cachedOriginalImagePosition = PreviewRect::Full;
  }
}

void PreviewWidget::getOriginalImageCrop(cimg_library::CImg<float> & image)
{
  if (_visibleRect == _cachedOriginalImagePosition) {
    image = *_cachedOriginalImage;
    return;
  }
  updateCachedOriginalImageCrop();
  image = *_cachedOriginalImage;
}

void PreviewWidget::onPreviewParametersChanged()
{
  emit previewVisibleRectIsChanging();
  if (_timerID) {
    killTimer(_timerID);
  }
  displayOriginalImage();
  _timerID = startTimer(RESIZE_DELAY);
  _savedPreviewIsValid = false;
}

void PreviewWidget::timerEvent(QTimerEvent * e)
{
  killTimer(e->timerId());
  _timerID = 0;
  sendUpdateRequest();
}

void PreviewWidget::onPreviewToggled(bool on)
{
  _previewEnabled = on;
  if (on) {
    if (_savedPreviewIsValid) {
      restorePreview();
      _paintOriginalImage = false;
      update();
    } else {
      emit previewUpdateRequested();
    }
  } else {
    displayOriginalImage();
  }
}

void PreviewWidget::setPreviewEnabled(bool on)
{
  _previewEnabled = on;
}

const KeypointList & PreviewWidget::keypoints() const
{
  return _keypoints;
}

void PreviewWidget::setKeypoints(const KeypointList & keypoints)
{
  _keypoints = keypoints;
  update();
}

void PreviewWidget::invalidateSavedPreview()
{
  _savedPreviewIsValid = false;
}

void PreviewWidget::restorePreview()
{
  *_image = *_savedPreview;
}

void PreviewWidget::enableRightClick()
{
  _rightClickEnabled = true;
}

void PreviewWidget::disableRightClick()
{
  _rightClickEnabled = false;
}

bool PreviewWidget::PreviewRect::operator!=(const PreviewRect & other) const
{
  return (x != other.x) || (y != other.y) || (w != other.w) || (h != other.h);
}

bool PreviewWidget::PreviewRect::operator==(const PreviewRect & other) const
{
  return (x == other.x) && (y == other.y) && (w == other.w) && (h == other.h);
}

bool PreviewWidget::PreviewRect::isFull() const
{
  return (*this) == PreviewRect::Full;
}

PreviewWidget::PreviewPoint PreviewWidget::PreviewRect::center() const
{
  PreviewPoint p;
  p.x = x + w / 2.0;
  p.y = y + h / 2.0;
  return p;
}

PreviewWidget::PreviewPoint PreviewWidget::PreviewRect::topLeft() const
{
  PreviewPoint p;
  p.x = x;
  p.y = y;
  return p;
}

void PreviewWidget::PreviewRect::moveCenter(const PreviewWidget::PreviewPoint & p)
{
  const double halfWidth = w / 2.0;
  const double halfHeight = h / 2.0;
  x = std::min(std::max(0.0, p.x - halfWidth), 1.0 - w);
  y = std::min(std::max(0.0, p.y - halfHeight), 1.0 - h);
}

void PreviewWidget::PreviewRect::moveToCenter()
{
  x = std::max(0.0, (1.0 - w) / 2.0);
  y = std::max(0.0, (1.0 - h) / 2.0);
}

bool PreviewWidget::PreviewPoint::isValid() const
{
  return (x >= 0.0) && (x <= 1.0) && (y >= 0.0) && (y <= 1.0);
}

bool PreviewWidget::PreviewPoint::operator!=(const PreviewWidget::PreviewPoint & other) const
{
  return (x != other.x) || (y != other.y);
}

bool PreviewWidget::PreviewPoint::operator==(const PreviewWidget::PreviewPoint & other) const
{
  return (x == other.x) && (y == other.y);
}

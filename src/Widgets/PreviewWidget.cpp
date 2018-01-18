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
#include "ImageConverter.h"
#include "ImageTools.h"
#include "LayersExtentProxy.h"
#include "gmic.h"

const PreviewWidget::PreviewPosition PreviewWidget::PreviewPosition::Full{0.0, 0.0, 1.0, 1.0};

PreviewWidget::PreviewWidget(QWidget * parent) : QWidget(parent), _cachedOriginalImage(new gmic_image<float>())
{
  setAutoFillBackground(false);
  _image = new cimg_library::CImg<float>;
  _image->assign();
  _savedPreview = new cimg_library::CImg<float>;
  _savedPreview->assign();
  _transparency.load(":resources/transparency.png");

  _visibleRect = PreviewPosition::Full;
  _cachedOriginalImagePosition = {-1.0, -1.0, -1.0, -1.0};
  _positionAtUpdateRequest = PreviewPosition::Full;

  _pendingResize = false;
  _previewEnabled = true;
  _currentZoomFactor = 1.0;
  _timerID = 0;
  _savedPreviewIsValid = false;
  _paintOriginalImage = true;
  qApp->installEventFilter(this);
  _rightClickEnabled = false;
  _originalImageSize = QSize(-1, -1);
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
  if (_visibleRect != _positionAtUpdateRequest) {
    return;
  }
  *_image = image;
  updateOriginalImagePosition();
  _paintOriginalImage = false;
  if (isAtFullZoom()) {
    _currentZoomFactor = std::min(width() / (double)_fullImageSize.width(), height() / (double)_fullImageSize.height());
    emit zoomChanged(_currentZoomFactor);
  }
  update();
}

void PreviewWidget::setFullImageSize(const QSize & size)
{
  _fullImageSize = size;
  _image->assign();
  _cachedOriginalImagePosition = {-1.0, -1.0, -1.0, -1.0};
  updateVisibleRect();
}

void PreviewWidget::updateVisibleRect()
{
  _visibleRect.w = std::min(1.0, (width() / (_currentZoomFactor * _fullImageSize.width())));
  _visibleRect.h = std::min(1.0, (height() / (_currentZoomFactor * _fullImageSize.height())));
  _visibleRect.x = std::min(_visibleRect.x, 1.0 - _visibleRect.w);
  _visibleRect.y = std::min(_visibleRect.y, 1.0 - _visibleRect.h);
}

void PreviewWidget::centerVisibleRect()
{
  _visibleRect.x = std::max(0.0, (1.0 - _visibleRect.w) / 2.0);
  _visibleRect.y = std::max(0.0, (1.0 - _visibleRect.h) / 2.0);
}

void PreviewWidget::updateOriginalImagePosition()
{
  QSize size = originalImageCropSize();
  if (size == _originalImageSize) {
    return;
  }
  _originalImageSize = size;
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

void PreviewWidget::paintEvent(QPaintEvent * e)
{
  QPainter painter(this);
  QImage qimage;
  if (_paintOriginalImage) {
    gmic_image<float> image;
    getOriginalImageCrop(image);
    updateOriginalImagePosition();
    image.resize(_imagePosition.width(), _imagePosition.height(), 1, -100, 1);
    if (hasAlphaChannel(image)) {
      painter.fillRect(_imagePosition, QBrush(_transparency));
    }
    ImageConverter::convert(image, qimage);
    painter.drawImage(_imagePosition, qimage);
  } else {
    // Display the preview

    //  If preview image has a size different from the original image crop, or
    //  we are at "full image" zoom of an image smaller than the widget,
    //  then the image should fit the widget size.
    if ((QSize(_image->width(), _image->height()) != _originaImageScaledSize) || (isAtFullZoom() && _currentZoomFactor > 1.0)) {
      QSize imageSize = QSize(_image->width(), _image->height()).scaled(width(), height(), Qt::KeepAspectRatio);
      _imagePosition = QRect(QPoint(std::max(0, (width() - imageSize.width()) / 2), std::max(0, (height() - imageSize.height()) / 2)), imageSize);
      _originaImageScaledSize = QSize(-1, -1); // Make sure next preview update will not consider originaImageScaledSize
    }
    /*
     *  Otherwise : Preview size == Original scaled size and image position is therefore unchanged
     */

    if (hasAlphaChannel(*_image)) {
      painter.fillRect(_imagePosition, QBrush(_transparency));
    }
    ImageConverter::convert(_image->get_resize(_imagePosition.width(), _imagePosition.height(), 1, -100, 1), qimage);
    painter.drawImage(_imagePosition, qimage);
  }
  e->accept();
}

void PreviewWidget::resizeEvent(QResizeEvent * e)
{
  _pendingResize = true;
  e->accept();
  if (!e->size().width() || !e->size().height()) {
    return;
  }
  if (isAtFullZoom()) {
    _currentZoomFactor = std::min(e->size().width() / (double)_fullImageSize.width(), e->size().height() / (double)_fullImageSize.height());
    emit zoomChanged(_currentZoomFactor);
  } else {
    updateVisibleRect();
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
  _positionAtUpdateRequest = _visibleRect;
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

void PreviewWidget::timerEvent(QTimerEvent * e)
{
  killTimer(e->timerId());
  _timerID = 0;
  sendUpdateRequest();
}

bool PreviewWidget::event(QEvent * event)
{
  if (event->type() == QEvent::WindowActivate && _pendingResize) {
    _pendingResize = false;
    if (width() && height()) {
      updateVisibleRect();
      sendUpdateRequest();
    }
  }
  return QWidget::event(event);
}

bool PreviewWidget::eventFilter(QObject *, QEvent * event)
{
  if ((event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::NonClientAreaMouseButtonRelease) && _pendingResize) {
    _pendingResize = false;
    if (width() && height()) {
      updateVisibleRect();
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
  if (e->button() == Qt::LeftButton) {
    if (_imagePosition.contains(e->pos())) {
      _mousePosition = e->pos();
      abortUpdateTimer();
    } else {
      _mousePosition = QPoint(-1, -1);
    }
    e->accept();
    return;
  }

  if (_rightClickEnabled && (e->button() == Qt::RightButton)) {
    if (_previewEnabled) {
      savePreview();
      displayOriginalImage();
    }
    e->accept();
    return;
  }

  e->ignore();
}

void PreviewWidget::mouseReleaseEvent(QMouseEvent * e)
{
  if (e->button() == Qt::LeftButton) {
    if (!isAtFullZoom() && _mousePosition != QPoint(-1, -1)) {
      QPoint move = _mousePosition - e->pos();
      onMouseTranslationInImage(move);
      sendUpdateRequest();
      _mousePosition = QPoint(-1, -1);
    }
    e->accept();
    return;
  }

  if (_rightClickEnabled && (e->button() == Qt::RightButton)) {
    if (_previewEnabled) {
      restorePreview();
      _paintOriginalImage = false;
      update();
    }
    e->accept();
    return;
  }
}

void PreviewWidget::mouseMoveEvent(QMouseEvent * e)
{
  if (e->buttons() & Qt::LeftButton) {
    if (!isAtFullZoom() && (_mousePosition != QPoint(-1, -1))) {
      QPoint move = _mousePosition - e->pos();
      if (move.manhattanLength()) {
        onMouseTranslationInImage(move);
        _mousePosition = e->pos();
      }
    }
    e->accept();
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

void PreviewWidget::updateImageNames(gmic_list<char> & imageNames, GmicQt::InputMode mode)
{
  int maxWidth;
  int maxHeight;
  LayersExtentProxy::getExtent(mode, maxWidth, maxHeight);
  float xFactor = (_visibleRect.w * _fullImageSize.width()) / (float)maxWidth;
  float yFactor = (_visibleRect.h * _fullImageSize.height()) / (float)maxHeight;

  for (size_t i = 0; i < imageNames.size(); ++i) {
    gmic_image<char> & name = imageNames[i];
    QString str((const char *)name);
    QRegExp position("pos\\((\\d*)([^0-9]*)(\\d*)\\)");
    if (str.contains(position) && position.matchedLength() > 0) {
      int xPos = position.cap(1).toInt();
      int yPos = position.cap(3).toInt();
      int newXPos = (int)(xPos * xFactor * _currentZoomFactor);
      int newYPos = (int)(yPos * yFactor * _currentZoomFactor);
      str.replace(position.cap(0), QString("pos(%1%2%3)").arg(newXPos).arg(position.cap(2)).arg(newYPos));
      name.resize(str.size() + 1);
      std::memcpy(name.data(), str.toLatin1().constData(), name.width());
    }
  }
}

void PreviewWidget::onMouseTranslationInImage(QPoint shift)
{
  if (shift.manhattanLength()) {
    translateFullImage(shift.x() / _currentZoomFactor, shift.y() / _currentZoomFactor);
    displayOriginalImage();
  }
}

void PreviewWidget::translateNormalized(double dx, double dy)
{
  _visibleRect.x = std::max(0.0, std::min(1.0 - _visibleRect.w, _visibleRect.x + dx));
  _visibleRect.y = std::max(0.0, std::min(1.0 - _visibleRect.h, _visibleRect.y + dy));
}

void PreviewWidget::translateFullImage(double dx, double dy)
{
  translateNormalized(dx / _fullImageSize.width(), dy / _fullImageSize.height());
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
  _visibleRect = PreviewPosition::Full;
  _currentZoomFactor = std::min(width() / (double)_fullImageSize.width(), height() / (double)_fullImageSize.height());
  onPreviewParametersChanged();
  emit zoomChanged(_currentZoomFactor);
}

void PreviewWidget::zoomIn(QPoint p, int steps)
{
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
  onPreviewParametersChanged();
  emit zoomChanged(_currentZoomFactor);
}

void PreviewWidget::zoomOut(QPoint p, int steps)
{
  if (isAtFullZoom()) {
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
  onPreviewParametersChanged();
  emit zoomChanged(_currentZoomFactor);
}

void PreviewWidget::setZoomLevel(double zoom)
{
  if (zoom == _currentZoomFactor) {
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
  onPreviewParametersChanged();
  emit zoomChanged(_currentZoomFactor);
}

double PreviewWidget::defaultZoomFactor() const
{
  if (_previewFactor == GmicQt::PreviewFactorFullImage) {
    return std::min(width() / (double)_fullImageSize.width(), height() / (double)_fullImageSize.height());
  }
  if (_previewFactor > 1.0) {
    return _previewFactor * std::min(width() / (double)_fullImageSize.width(), height() / (double)_fullImageSize.height());
  }
  return 1.0; // We suppose GmicQt::PreviewFactorActualSize
}

void PreviewWidget::setPreviewFactor(float filterFactor, bool reset)
{
  _previewFactor = filterFactor;
  if ((_previewFactor == GmicQt::PreviewFactorFullImage) || ((_previewFactor == GmicQt::PreviewFactorAny) && reset)) {
    _currentZoomFactor = std::min(width() / (double)_fullImageSize.width(), height() / (double)_fullImageSize.height());
    _visibleRect = PreviewWidget::PreviewPosition::Full;
  } else if ((_previewFactor == GmicQt::PreviewFactorAny) && !reset) {
    updateVisibleRect();
  } else {
    _currentZoomFactor = defaultZoomFactor();
    updateVisibleRect();
    if (_previewFactor == GmicQt::PreviewFactorActualSize) {
      centerVisibleRect();
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
  }
}

void PreviewWidget::getOriginalImageCrop(cimg_library::CImg<float> & image)
{
  if (_visibleRect == _cachedOriginalImagePosition) {
    image = *_cachedOriginalImage;
  }
  updateCachedOriginalImageCrop();
  image = *_cachedOriginalImage;
}

void PreviewWidget::onPreviewParametersChanged()
{
  if (_timerID) {
    killTimer(_timerID);
  }
  displayOriginalImage();
  _timerID = startTimer(RESIZE_DELAY);
}

void PreviewWidget::enablePreview(bool on)
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

void PreviewWidget::invalidateSavedPreview()
{
  _savedPreviewIsValid = false;
}

void PreviewWidget::savePreview()
{
  *_savedPreview = *_image;
  _savedPreviewIsValid = true;
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

bool PreviewWidget::PreviewPosition::operator!=(const PreviewPosition & other) const
{
  return (x != other.x) || (y != other.y) || (w != other.w) || (h != other.h);
}

bool PreviewWidget::PreviewPosition::operator==(const PreviewPosition & other) const
{
  return (x == other.x) && (y == other.y) && (w == other.w) && (h == other.h);
}

bool PreviewWidget::PreviewPosition::isFull() const
{
  return (*this) == PreviewPosition::Full;
}

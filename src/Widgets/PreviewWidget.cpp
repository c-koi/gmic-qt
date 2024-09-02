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
#include <QPointF>
#include <algorithm>
#include "Common.h"
#include "CroppedActiveLayerProxy.h"
#include "Globals.h"
#include "GmicStdlib.h"
#include "ImageTools.h"
#include "Misc.h"
#include "OverrideCursor.h"
#include "Settings.h"
#include "gmic.h"

namespace GmicQt
{

const PreviewWidget::PreviewRect PreviewWidget::PreviewRect::Full{0.0, 0.0, 1.0, 1.0};

PreviewWidget::PreviewWidget(QWidget * parent) : QWidget(parent)
{
  setAutoFillBackground(false);
  _image = new gmic_library::gmic_image<float>;
  _image->assign();
  _savedPreview = new gmic_library::gmic_image<float>;
  _savedPreview->assign();
  _transparency.load(":resources/transparency.png");

  _visibleRect = PreviewRect::Full;
  saveVisibleCenter();

  _pendingResize = false;
  _previewEnabled = true;
  _currentZoomFactor = 1.0;
  _zoomConstraint = ZoomConstraint::Any;
  _timerID = 0;
  _savedPreviewIsValid = false;
  _paintOriginalImage = true;
  qApp->installEventFilter(this);
  _rightClickEnabled = false;
  _originalImageSize = QSize(-1, -1);
  _movedKeypointOrigin = QPoint(-1, -1);
  _movedKeypointIndex = -1;
  _previewType = PreviewType::Full;
  setMouseTracking(false);
  _xPreviewSplit = 0.5f;
  _yPreviewSplit = 0.5f;
  _draggingMode = DraggingMode::Inactive;
}

PreviewWidget::~PreviewWidget()
{
  delete _image;
  delete _savedPreview;
}

const gmic_library::gmic_image<float> & PreviewWidget::image() const
{
  return *_image;
}

void PreviewWidget::setPreviewImage(const gmic_library::gmic_image<float> & image)
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
  CroppedActiveLayerProxy::clear();
  updateVisibleRect();
  saveVisibleCenter();
}

void PreviewWidget::updateFullImageSizeIfDifferent(const QSize & size)
{
  if (size != _fullImageSize) {
    setFullImageSize(size);
  } else {
    CroppedActiveLayerProxy::clear();
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
    _originalImageScaledSize = QSize(0, 0);
    _imagePosition = rect();
    return;
  }
  _originalImageSize = originalImageCropSize();

  if (isAtFullZoom()) {
    double correctZoomFactor = std::min(width() / (double)_originalImageSize.width(), height() / (double)_originalImageSize.height());
    if (correctZoomFactor != _currentZoomFactor) {
      _currentZoomFactor = correctZoomFactor;
      emit zoomChanged(_currentZoomFactor);
    }
  }

  if (_currentZoomFactor > 1.0) {
    _originalImageScaledSize = _originalImageSize;
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
    _originalImageScaledSize = QSize(static_cast<int>(std::round(_originalImageSize.width() * _currentZoomFactor)), //
                                     static_cast<int>(std::round(_originalImageSize.height() * _currentZoomFactor)));
    _imagePosition = QRect(QPoint(std::max(0, (width() - _originalImageScaledSize.width()) / 2), //
                                  std::max(0, (height() - _originalImageScaledSize.height()) / 2)),
                           _originalImageScaledSize);
  }
}

void PreviewWidget::updatePreviewImagePosition()
{
  /*  If preview image has a size different from the original image crop, or
   *  we are at "full image" zoom of an image smaller than the widget,
   *  then the image should fit the widget size.
   */
  const QSize previewImageSize(_image->width(), _image->height());
  if ((previewImageSize != _originalImageScaledSize) || (isAtFullZoom() && (_currentZoomFactor > 1.0))) {
    QSize imageSize;
    if (previewImageSize != _originalImageScaledSize) {
      imageSize = previewImageSize.scaled(width(), height(), Qt::KeepAspectRatio);
    } else {
      imageSize = QSize(static_cast<int>(std::round(_originalImageSize.width() * _currentZoomFactor)), //
                        static_cast<int>(std::round(_originalImageSize.height() * _currentZoomFactor)));
    }
    _imagePosition = QRect(QPoint(std::max(0, (width() - imageSize.width()) / 2), //
                                  std::max(0, (height() - imageSize.height()) / 2)),
                           imageSize);
    _originalImageScaledSize = QSize(-1, -1); // Make sure next preview update will not consider originaImageScaledSize
  }
  /*
   *  Otherwise : Preview size == Original scaled size and image position is therefore unchanged
   */
}

QRect PreviewWidget::splittedPreviewPosition()
{
  updateOriginalImagePosition();
  QRect original = _imagePosition;
  updatePreviewImagePosition();
  QRect preview = _imagePosition;
  int x1 = std::max(0, std::min(original.left(), preview.left()));
  int y1 = std::max(0, std::min(original.top(), preview.top()));
  int x2 = std::min(width() - 1, std::max(original.left() + original.width(), preview.left() + preview.width()));
  int y2 = std::min(height() - 1, std::max(original.top() + original.height(), preview.top() + preview.height()));
  return QRect(x1, y1, 1 + x2 - x1, 1 + y2 - y1);
}

void PreviewWidget::updateErrorImage()
{
  gmic_library::gmic_list<float> images;
  gmic_library::gmic_list<char> imageNames;
  images.assign();
  imageNames.assign();
  gmic_image<float> image;
  getOriginalImageCrop(image);
  image.move_to(images);
  QString fullCommandLine = commandFromOutputMessageMode(Settings::outputMessageMode());
  fullCommandLine += QString(" _host=%1 _tk=qt").arg(GmicQtHost::ApplicationShortname);
  fullCommandLine += QString(" _preview_area_width=%1").arg(width());
  fullCommandLine += QString(" _preview_area_height=%1").arg(height());
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
  convertGmicImageToQImage(images.front(), qimage);
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

  QRect visibleRect = rect() & _imagePosition;
  KeypointList::reverse_iterator it = _keypoints.rbegin();
  int index = static_cast<int>(_keypoints.size() - 1);
  while (it != _keypoints.rend()) {
    if (!it->isNaN()) {
      const KeypointList::Keypoint & kp = *it;
      const int & radius = kp.actualRadiusFromPreviewSize(_imagePosition.size());
      QPoint visibleCenter = keypointToVisiblePointInWidget(kp);
      QPoint realCenter = keypointToPointInWidget(kp);
      QRect r(visibleCenter.x() - radius, visibleCenter.y() - radius, 2 * radius, 2 * radius);
      QColor brushColor = kp.color;
      if ((index == _movedKeypointIndex) && !kp.keepOpacityWhenSelected) {
        brushColor.setAlpha(255);
      }
      if (visibleRect.contains(realCenter, false)) {
        painter.setBrush(brushColor);
        pen.setStyle(Qt::SolidLine);
      } else {
        painter.setBrush(brushColor.darker(150));
        pen.setStyle(Qt::DotLine);
      }
      pen.setColor(QColor(0, 0, 0, brushColor.alpha()));
      painter.setPen(pen);
      painter.drawEllipse(r);
    }
    ++it;
    --index;
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
      if (roundedDistance(center, p) <= (kp.actualRadiusFromPreviewSize(_imagePosition.size()) + 2)) {
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
  return QPoint(static_cast<int>(std::round(_imagePosition.left() + (_imagePosition.width() - 1) * (kp.x / 100.0f))),
                static_cast<int>(std::round(_imagePosition.top() + (_imagePosition.height() - 1) * (kp.y / 100.0f))));
}

QPoint PreviewWidget::keypointToVisiblePointInWidget(const KeypointList::Keypoint & kp) const
{
  QPoint p = keypointToPointInWidget(kp);
  p.rx() = std::max(std::max(_imagePosition.left(), 0), std::min(p.x(), std::min(rect().left() + rect().width(), _imagePosition.left() + _imagePosition.width())));
  p.ry() = std::max(std::max(_imagePosition.top(), 0), std::min(p.y(), std::min(rect().top() + rect().height(), _imagePosition.top() + _imagePosition.height())));
  return p;
}

QPointF PreviewWidget::pointInWidgetToKeypointPosition(const QPoint & p) const
{
  QPointF result(100.0 * (p.x() - _imagePosition.left()) / (float)(_imagePosition.width() - 1), 100.0 * (p.y() - _imagePosition.top()) / (float)(_imagePosition.height() - 1));
  result.rx() = std::min(300.0, std::max(-200.0, result.x()));
  result.ry() = std::min(300.0, std::max(-200.0, result.y()));
  return result;
}

PreviewWidget::DraggingMode PreviewWidget::splitterDraggingModeFromMousePosition(const QPoint & p)
{
  if (_previewType == PreviewType::Full) {
    return DraggingMode::Inactive;
  }
  int x = (_imagePosition.left() <= 0) ? (_xPreviewSplit * width()) : (_imagePosition.left() + _xPreviewSplit * _imagePosition.width());
  int y = (_imagePosition.top() <= 0) ? (_yPreviewSplit * height()) : (_imagePosition.top() + _yPreviewSplit * _imagePosition.height());
  switch (_previewType) {
  case PreviewType::ForwardHorizontal:
  case PreviewType::BackwardHorizontal:
  case PreviewType::DuplicateTop:
  case PreviewType::DuplicateBottom:
  case PreviewType::DuplicateHorizontal:
    return (std::abs(p.y() - y) < (2 * _SplitterButtonWidth + _SplitterButtonMargin)) ? DraggingMode::Y : DraggingMode::Inactive;
    break;
  case PreviewType::ForwardVertical:
  case PreviewType::BackwardVertical:
  case PreviewType::DuplicateLeft:
  case PreviewType::DuplicateRight:
  case PreviewType::DuplicateVertical:
    return (std::abs(p.x() - x) < (2 * _SplitterButtonWidth + _SplitterButtonMargin)) ? DraggingMode::X : DraggingMode::Inactive;
    break;
  case PreviewType::Checkered:
  case PreviewType::CheckeredInverse: {
    int flag = 0;
    flag |= (std::abs(p.x() - x) < (2 * _SplitterButtonWidth + _SplitterButtonMargin)) ? DraggingMode::X : 0;
    flag |= (std::abs(p.y() - y) < (2 * _SplitterButtonWidth + _SplitterButtonMargin)) ? DraggingMode::Y : 0;
    return DraggingMode(flag);
  } break;
  case PreviewType::Full:
    return DraggingMode::Inactive;
    break;
  }
  return DraggingMode::Inactive;
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
    paintKeypoints(painter);
    return;
  }

  if (!_image->width() && !_image->height()) {
    painter.fillRect(rect(), QBrush(_transparency));
    paintKeypoints(painter);
    return;
  }

  updatePreviewImagePosition();

  if (hasAlphaChannel(*_image)) {
    painter.fillRect(_imagePosition, QBrush(_transparency));
  }
  QImage qimage;
  convertGmicImageToQImage(_image->get_resize(_imagePosition.width(), _imagePosition.height(), 1, -100, 1), qimage);
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
    convertGmicImageToQImage(image, qimage);
    painter.drawImage(_imagePosition, qimage);
    paintKeypoints(painter);
  }
}

void PreviewWidget::paintSplittedPreview(QPainter & painter)
{
  QRect position = splittedPreviewPosition();
  // int x = (_imagePosition.left() <= 0) ? (_xPreviewSplit * width()) : (_imagePosition.left() + _xPreviewSplit * _imagePosition.width());
  // int y = (_imagePosition.top() <= 0) ? (_yPreviewSplit * height()) : (_imagePosition.top() + _yPreviewSplit * _imagePosition.height());
  int x = (position.left() + _xPreviewSplit * position.width());
  int y = (position.top() + _yPreviewSplit * position.height());

  switch (_previewType) {
  case PreviewType::Full:
    break;
  case PreviewType::ForwardHorizontal:
    paintOriginalImage(painter);
    painter.save();
    painter.setClipRect(position.left(), y, position.width(), 1 + position.bottom() - y);
    paintPreview(painter);
    painter.restore();
    break;
  case PreviewType::ForwardVertical:
    paintOriginalImage(painter);
    painter.save();
    painter.setClipRect(x, position.top(), 1 + position.right() - x, position.height());
    paintPreview(painter);
    painter.restore();
    break;
  case PreviewType::BackwardHorizontal:
    paintPreview(painter);
    painter.save();
    painter.setClipRect(position.left(), y, position.width(), 1 + position.bottom() - y);
    paintOriginalImage(painter);
    painter.restore();
    break;
  case PreviewType::BackwardVertical:
    paintPreview(painter);
    painter.save();
    painter.setClipRect(x, position.top(), 1 + position.right() - x, position.height());
    paintOriginalImage(painter);
    painter.restore();
    break;
  case PreviewType::DuplicateTop:
    paintOriginalImage(painter);
    painter.save();
    painter.setClipRect(position.left(), y, position.width(), 1 + position.bottom() - y);
    painter.translate(QPoint(0, y - position.top()));
    paintPreview(painter);
    painter.restore();
    break;
  case PreviewType::DuplicateBottom:
    paintOriginalImage(painter);
    painter.save();
    painter.setClipRect(0, position.top(), width(), y);
    painter.translate(QPoint(0, y - position.bottom()));
    paintPreview(painter);
    painter.restore();
    break;
  case PreviewType::DuplicateLeft:
    paintOriginalImage(painter);
    painter.save();
    painter.setClipRect(x, position.top(), 1 + position.right() - x, position.height());
    painter.translate(QPoint(x - position.left(), 0));
    paintPreview(painter);
    painter.restore();
    break;
  case PreviewType::DuplicateRight:
    paintOriginalImage(painter);
    painter.save();
    painter.setClipRect(position.left(), position.top(), x, position.height());
    painter.translate(QPoint(x - position.right(), 0));
    paintPreview(painter);
    painter.restore();
    break;
  case PreviewType::DuplicateHorizontal:
    // Centered crop with corresponding height
    painter.save();
    painter.save();
    painter.setClipRect(position);
    painter.translate(QPoint(0, (y - position.bottom()) / 2));
    paintOriginalImage(painter);
    painter.restore();
    painter.setClipRect(position.left(), y, position.width(), 1 + position.bottom() - y);
    painter.translate(QPoint(0, (y - position.top()) / 2));
    paintPreview(painter);
    painter.restore();
    break;
  case PreviewType::DuplicateVertical:
    // Centered crop with corresponding width
    painter.save();
    painter.save();
    painter.setClipRect(position);
    painter.translate(QPoint((x - position.right()) / 2, 0));
    paintOriginalImage(painter);
    painter.restore();
    painter.setClipRect(x, position.top(), 1 + position.right() - x, position.height());
    painter.translate(QPoint((x - position.left()) / 2, 0));
    paintPreview(painter);
    painter.restore();
    break;
  case PreviewType::Checkered:
    painter.save();
    paintOriginalImage(painter);
    painter.setClipRect(x, position.top(), 1 + position.right() - x, 1 + y - position.top());
    paintPreview(painter);
    painter.setClipRect(position.left(), y, 1 + x - position.left(), 1 + position.bottom() - y);
    paintPreview(painter);
    painter.restore();
    break;
  case PreviewType::CheckeredInverse:
    painter.save();
    paintOriginalImage(painter);
    painter.setClipRect(position.left(), position.top(), 1 + x - position.left(), 1 + y - position.top());
    paintPreview(painter);
    painter.setClipRect(x, y, 1 + position.right() - x, 1 + position.bottom() - y);
    paintPreview(painter);
    painter.restore();
    break;
  }
  painter.setClipping(false);
}

void PreviewWidget::paintEvent(QPaintEvent * e)
{
  QPainter painter(this);
  if (_paintOriginalImage) {
    paintOriginalImage(painter);
  } else {
    if ((_previewType == PreviewType::Full) || !_errorMessage.isEmpty()) {
      paintPreview(painter);
    } else {
      paintSplittedPreview(painter);
    }
  }
  if ((_previewType != PreviewType::Full) && _errorMessage.isEmpty()) {
    paintPreviewSplitter(painter);
  }
  e->accept();
}

void PreviewWidget::paintPreviewSplitter(QPainter & painter)
{
  painter.end();
  painter.begin(this);
  QPen pen(QColor(0, 0, 0, 164));
  pen.setWidth(1);
  painter.setPen(pen);

  QRect position = splittedPreviewPosition();
  int x = (position.left() + _xPreviewSplit * position.width());
  int y = (position.top() + _yPreviewSplit * position.height());
  QPolygon leftTriangle;
  QPolygon topTriangle;
  QPolygon bottomTriangle;
  QPolygon rightTriangle;

  switch (_previewType) {
  case PreviewType::Full:
    break;
  case PreviewType::ForwardHorizontal:
  case PreviewType::BackwardHorizontal:
  case PreviewType::DuplicateTop:
  case PreviewType::DuplicateBottom:
  case PreviewType::DuplicateHorizontal:
    x = width() / 2;
    // int y = (_imagePosition.top() <= 0) ? (_yPreviewSplit * height()) : (_imagePosition.top() + _yPreviewSplit * _imagePosition.height()); // FIXME Remove
    topTriangle << QPoint(x - _SplitterButtonWidth, y - _SplitterButtonMargin) << QPoint(x, y - (_SplitterButtonMargin + _SplitterButtonWidth))
                << QPoint(x + _SplitterButtonWidth, y - _SplitterButtonMargin);
    bottomTriangle << QPoint(x - _SplitterButtonWidth, y + _SplitterButtonMargin) << QPoint(x, y + (_SplitterButtonMargin + _SplitterButtonWidth))
                   << QPoint(x + _SplitterButtonWidth, y + _SplitterButtonMargin);
    painter.setBrush(QColor(255, 255, 255, 164));
    painter.drawPolygon(topTriangle);
    painter.drawPolygon(bottomTriangle);
    pen.setColor(Qt::black);
    painter.setPen(pen);
    painter.drawLine(std::max(0, _imagePosition.left()), y, _imagePosition.right(), y);
    pen.setDashPattern(QVector<qreal>() << 4.0 << 4.0);
    pen.setStyle(Qt::CustomDashLine);
    pen.setColor(Qt::white);
    painter.setPen(pen);
    painter.drawLine(std::max(0, _imagePosition.left()), y, _imagePosition.right(), y);
    break;
  case PreviewType::ForwardVertical:
  case PreviewType::BackwardVertical:
  case PreviewType::DuplicateLeft:
  case PreviewType::DuplicateRight:
  case PreviewType::DuplicateVertical:
    // int x = (_imagePosition.left() <= 0) ? (_xPreviewSplit * width()) : (_imagePosition.left() + _xPreviewSplit * _imagePosition.width());
    y = height() / 2;
    leftTriangle << QPoint(x - _SplitterButtonMargin, y - _SplitterButtonWidth) << QPoint(x - (_SplitterButtonMargin + _SplitterButtonWidth), y)
                 << QPoint(x - _SplitterButtonMargin, y + _SplitterButtonWidth);
    rightTriangle << QPoint(x + _SplitterButtonMargin, y - _SplitterButtonWidth) << QPoint(x + (_SplitterButtonMargin + _SplitterButtonWidth), y)
                  << QPoint(x + _SplitterButtonMargin, y + _SplitterButtonWidth);
    painter.setBrush(QColor(255, 255, 255, 164));
    painter.drawPolygon(leftTriangle);
    painter.drawPolygon(rightTriangle);
    pen.setColor(Qt::black);
    painter.setPen(pen);
    painter.drawLine(x, std::max(0, _imagePosition.top()), x, _imagePosition.bottom());
    pen.setDashPattern(QVector<qreal>() << 4.0 << 4.0);
    pen.setStyle(Qt::CustomDashLine);
    pen.setColor(Qt::white);
    painter.setPen(pen);
    painter.drawLine(x, std::max(0, _imagePosition.top()), x, _imagePosition.bottom());
    break;
  case PreviewType::Checkered:
  case PreviewType::CheckeredInverse:
    // int x = (_imagePosition.left() <= 0) ? (_xPreviewSplit * width()) : (_imagePosition.left() + _xPreviewSplit * _imagePosition.width());
    // int y = (_imagePosition.top() <= 0) ? (_yPreviewSplit * height()) : (_imagePosition.top() + _yPreviewSplit * _imagePosition.height());
    pen.setColor(Qt::black);
    painter.setPen(pen);
    painter.drawLine(std::max(0, _imagePosition.left()), y, _imagePosition.right(), y);
    painter.drawLine(x, std::max(0, _imagePosition.top()), x, _imagePosition.bottom());
    pen.setDashPattern(QVector<qreal>() << 4.0 << 4.0);
    pen.setStyle(Qt::CustomDashLine);
    pen.setColor(Qt::white);
    painter.setPen(pen);
    painter.drawLine(std::max(0, _imagePosition.left()), y, _imagePosition.right(), y);
    painter.drawLine(x, std::max(0, _imagePosition.top()), x, _imagePosition.bottom());
    topTriangle << QPoint(x - _SplitterButtonWidth, y - _SplitterButtonMargin) << QPoint(x, y - (_SplitterButtonMargin + _SplitterButtonWidth))
                << QPoint(x + _SplitterButtonWidth, y - _SplitterButtonMargin);
    bottomTriangle << QPoint(x - _SplitterButtonWidth, y + _SplitterButtonMargin) << QPoint(x, y + (_SplitterButtonMargin + _SplitterButtonWidth))
                   << QPoint(x + _SplitterButtonWidth, y + _SplitterButtonMargin);
    pen.setColor(QColor(0, 0, 0, 164));
    pen.setStyle(Qt::SolidLine);
    painter.setPen(pen);
    painter.setBrush(QColor(255, 255, 255, 164));
    painter.drawPolygon(topTriangle);
    painter.drawPolygon(bottomTriangle);
    break;
  }
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
  if (!QApplication::topLevelWidgets().isEmpty() && QApplication::topLevelWidgets().at(0)->isMaximized()) {
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
  return (_previewFactor == PreviewFactorAny) || (std::abs(_currentZoomFactor - defaultZoomFactor()) < 0.05) || ((_previewFactor == PreviewFactorActualSize) && (_currentZoomFactor >= 1.0));
}

double PreviewWidget::currentZoomFactor() const
{
  return _currentZoomFactor;
}

bool PreviewWidget::event(QEvent * event)
{
  if ((event->type() == QEvent::WindowActivate) && _pendingResize) {
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

void PreviewWidget::leaveEvent(QEvent *) {}

#if QT_VERSION_GTE(6, 0, 0)
void PreviewWidget::enterEvent(QEnterEvent *) {}
#else
void PreviewWidget::enterEvent(QEvent *) {}
#endif

void PreviewWidget::wheelEvent(QWheelEvent * event)
{
  double degrees = event->angleDelta().y() / 8.0;
  int steps = static_cast<int>(std::fabs(degrees) / 15.0);
#if QT_VERSION_GTE(5, 14, 0)
  const QPoint position = event->position().toPoint();
#else
  const QPoint position = event->pos();
#endif
  if (degrees > 0.0) {
    zoomIn(position - _imagePosition.topLeft(), steps);
  } else {
    zoomOut(position - _imagePosition.topLeft(), steps);
  }
  event->accept();
}

void PreviewWidget::mousePressEvent(QMouseEvent * e)
{
  if (e->button() == Qt::LeftButton || e->button() == Qt::MiddleButton) {
    int index = keypointUnderMouse(e->pos());
    if (index != -1) {
      _movedKeypointIndex = index;
      _keypointTimestamp = e->timestamp();
      abortUpdateTimer();
      _mousePosition = QPoint(-1, -1);
      if (!_keypoints[index].keepOpacityWhenSelected) {
        update();
      }
    } else if ((_draggingMode = splitterDraggingModeFromMousePosition(e->pos())) != DraggingMode::Inactive) {
    } else if (_imagePosition.contains(e->pos())) {
      _mousePosition = e->pos();
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
    if (_draggingMode != DraggingMode::Inactive) {
      _draggingMode = DraggingMode::Inactive;
    } else if (!isAtFullZoom() && _mousePosition != QPoint(-1, -1)) {
      QPoint move = _mousePosition - e->pos();
      onMouseTranslationInImage(move);
      sendUpdateRequest();
      _mousePosition = QPoint(-1, -1);
    } else if (_movedKeypointIndex != -1) {
      QPointF p = pointInWidgetToKeypointPosition(e->pos());
      KeypointList::Keypoint & kp = _keypoints[_movedKeypointIndex];
      kp.setPosition(p);
      _movedKeypointIndex = -1;
      const unsigned char flags = KeypointMouseReleaseEvent | (kp.burst ? KeypointBurstEvent : 0);
      emit keypointPositionsChanged(flags, e->timestamp());
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
} // namespace GmicQt

void PreviewWidget::mouseMoveEvent(QMouseEvent * e)
{
  if (hasMouseTracking() && (_movedKeypointIndex == -1)) {
    DraggingMode mode = splitterDraggingModeFromMousePosition(e->pos());
    if ((_mousePosition == QPoint(-1, -1)) && (keypointUnderMouse(e->pos()) != -1)) {
      OverrideCursor::set(Qt::PointingHandCursor);
    } else if (mode == DraggingMode::X) {
      OverrideCursor::set(Qt::SplitHCursor);
    } else if (mode == DraggingMode::Y) {
      OverrideCursor::set(Qt::SplitVCursor);
    } else if (mode == DraggingMode::XY) {
      OverrideCursor::set(Qt::SizeAllCursor);
    } else {
      OverrideCursor::setNormal();
    }
  }
  if (e->buttons() & (Qt::LeftButton | Qt::MiddleButton)) {
    if (_draggingMode != DraggingMode::Inactive) {
      if (_draggingMode & DraggingMode::X) {
        if (_imagePosition.left() <= 0) {
          _xPreviewSplit = clamped(e->pos().x() / float(width()), 0.0f, 1.0f);
        } else {
          _xPreviewSplit = clamped((e->pos().x() - _imagePosition.left()) / float(_imagePosition.width()), 0.0f, 1.0f);
        }
      }
      if (_draggingMode & DraggingMode::Y) {
        if (_imagePosition.top() <= 0) {
          _yPreviewSplit = clamped(e->pos().y() / float(height()), 0.0f, 1.0f);
        } else {
          _yPreviewSplit = clamped((e->pos().y() - _imagePosition.top()) / float(_imagePosition.height()), 0.0f, 1.0f);
        }
      }
      update();
    } else if (!isAtFullZoom() && (_mousePosition != QPoint(-1, -1))) {
      QPoint move = _mousePosition - e->pos();
      if (move.manhattanLength()) {
        onMouseTranslationInImage(move);
        _mousePosition = e->pos();
      }
    } else if (_movedKeypointIndex != -1) {
      QPointF p = pointInWidgetToKeypointPosition(e->pos());
      KeypointList::Keypoint & kp = _keypoints[_movedKeypointIndex];
      kp.setPosition(p);
      repaint();
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
  if (_fullImageSize.isNull() || (_zoomConstraint == ZoomConstraint::Fixed)) {
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
  if ((_zoomConstraint == ZoomConstraint::Fixed) || ((_zoomConstraint == ZoomConstraint::OneOrMore) && (_currentZoomFactor <= 1.0)) || isAtFullZoom() || _fullImageSize.isNull()) {
    return;
  }
  double mouseX = p.x() / (_currentZoomFactor * _fullImageSize.width()) + _visibleRect.x;
  double mouseY = p.y() / (_currentZoomFactor * _fullImageSize.height()) + _visibleRect.y;
  while (steps--) {
    _currentZoomFactor /= 1.2;
  }
  if ((_zoomConstraint == ZoomConstraint::OneOrMore) && (_currentZoomFactor <= 1.0)) {
    _currentZoomFactor = 1.0;
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
  if ((_zoomConstraint == ZoomConstraint::OneOrMore) && (zoom <= 1.0)) {
    zoom = 1.0;
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
  if (_previewFactor == PreviewFactorFullImage) {
    return std::min(width() / static_cast<double>(_fullImageSize.width()), height() / static_cast<double>(_fullImageSize.height()));
  }
  if (_previewFactor > 1.0f) {
    return _previewFactor * std::min(width() / (double)_fullImageSize.width(), height() / (double)_fullImageSize.height());
  }
  return 1.0; // We suppose PreviewFactorActualSize
}

void PreviewWidget::saveVisibleCenter()
{
  _savedVisibleCenter = _visibleRect.center();
}

int PreviewWidget::roundedDistance(const QPoint & p1, const QPoint & p2)
{
  const double dx = p1.x() - p2.x();
  const double dy = p1.y() - p2.y();
  return static_cast<int>(std::round(std::sqrt(dx * dx + dy * dy)));
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
  if ((_previewFactor == PreviewFactorFullImage) || ((_previewFactor == PreviewFactorAny) && reset)) {
    _currentZoomFactor = std::min(width() / (double)_fullImageSize.width(), height() / (double)_fullImageSize.height());
    _visibleRect = PreviewRect::Full;
    if (reset) {
      saveVisibleCenter();
    }
  } else if ((_previewFactor == PreviewFactorAny) && !reset) {
    updateVisibleRect();
    _visibleRect.moveCenter(_savedVisibleCenter);
  } else {
    _currentZoomFactor = defaultZoomFactor();
    updateVisibleRect();
    if (reset) {
      _visibleRect.moveToCenter();
      saveVisibleCenter();
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
  return CroppedActiveLayerProxy::getSize(_visibleRect.x, _visibleRect.y, _visibleRect.w, _visibleRect.h);
}

void PreviewWidget::getOriginalImageCrop(gmic_library::gmic_image<float> & image)
{
  CroppedActiveLayerProxy::get(image, _visibleRect.x, _visibleRect.y, _visibleRect.w, _visibleRect.h);
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

void PreviewWidget::setPreviewType(PreviewType previewType)
{
  _previewType = previewType;
  setMouseTracking(previewType != PreviewType::Full);
  update();
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
  setMouseTracking(_keypoints.size());
  update();
}

void PreviewWidget::setZoomConstraint(const ZoomConstraint & constraint)
{
  _zoomConstraint = constraint;
}

ZoomConstraint PreviewWidget::zoomConstraint() const
{
  return _zoomConstraint;
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
  return {x + w / 2.0, y + h / 2.0};
}

PreviewWidget::PreviewPoint PreviewWidget::PreviewRect::topLeft() const
{
  return {x, y};
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

} // namespace GmicQt

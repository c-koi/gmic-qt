/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file PreviewWidget.h
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
#ifndef GMIC_QT_PREVIEWWIDGET_H
#define GMIC_QT_PREVIEWWIDGET_H

#include <QFocusEvent>
#include <QImage>
#include <QMutex>
#include <QPixmap>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QWidget>
#include <memory>
#include "Host/host.h"
#include "KeypointList.h"
#include "ZoomConstraint.h"

namespace cimg_library
{
template <typename T> struct CImgList;
template <typename T> struct CImg;
} // namespace cimg_library

class PreviewWidget : public QWidget {
  Q_OBJECT

public:
  explicit PreviewWidget(QWidget * parent = nullptr);
  ~PreviewWidget() override;
  void setFullImageSize(const QSize &);
  void updateFullImageSizeIfDifferent(const QSize &);
  void normalizedVisibleRect(double & x, double & y, double & width, double & height) const;
  bool isAtDefaultZoom() const;
  bool isAtFullZoom() const;
  void getPositionStringCorrection(double & xFactor, double & yFactor) const;
  double currentZoomFactor() const;
  double defaultZoomFactor() const;
  void updateVisibleRect();
  void centerVisibleRect();
  void setPreviewImage(const cimg_library::CImg<float> & image);
  void setOverlayMessage(const QString &);
  void clearOverlayMessage();
  void setPreviewErrorMessage(const QString &);
  const cimg_library::CImg<float> & image() const;
  void translateNormalized(double dx, double dy);
  void translateFullImage(double dx, double dy);
  void setPreviewEnabled(bool on);

  const KeypointList & keypoints() const;
  void setKeypoints(const KeypointList &);

  enum KeypointMotionFlags
  {
    KeypointBurstEvent = 1,
    KeypointMouseReleaseEvent = 2
  };

  void setZoomConstraint(const ZoomConstraint & constraint);
  ZoomConstraint zoomConstraint() const;

protected:
  void resizeEvent(QResizeEvent *) override;
  void timerEvent(QTimerEvent *) override;
  bool event(QEvent * event) override;
  void wheelEvent(QWheelEvent * event) override;
  void mouseMoveEvent(QMouseEvent * e) override;
  void mouseReleaseEvent(QMouseEvent * e) override;
  void mousePressEvent(QMouseEvent * e) override;
  void paintEvent(QPaintEvent * e) override;
  bool eventFilter(QObject *, QEvent * event) override;
  void leaveEvent(QEvent *) override;

signals:
  void previewVisibleRectIsChanging();
  void previewUpdateRequested();
  void keypointPositionsChanged(unsigned int flags, unsigned long time);
  void zoomChanged(double zoom);

public slots:
  void abortUpdateTimer();
  void sendUpdateRequest();
  void onMouseTranslationInImage(QPoint shift);
  void zoomIn();
  void zoomOut();
  void zoomFullImage();
  void zoomIn(QPoint, int steps);
  void zoomOut(QPoint, int steps);
  void setZoomLevel(double zoom);
  /**
   * @brief setPreviewFactor
   * @param filterFactor
   * @param reset If true, zoomFactor is set to "Full Image" if
   *              filterFactor is GmicQt::PreviewFactorAny
   */
  void setPreviewFactor(float filterFactor, bool reset);
  void displayOriginalImage();
  void onPreviewParametersChanged();
  void invalidateSavedPreview();
  void restorePreview();
  void enableRightClick();
  void disableRightClick();
  void onPreviewToggled(bool on);

private:
  void paintPreview(QPainter &);
  void paintOriginalImage(QPainter &);
  void getOriginalImageCrop(cimg_library::CImg<float> & image);
  void updateOriginalImagePosition();
  void updateErrorImage();

  void paintKeypoints(QPainter & painter);
  int keypointUnderMouse(const QPoint & p);
  QPoint keypointToPointInWidget(const KeypointList::Keypoint & kp) const;
  QPoint keypointToVisiblePointInWidget(const KeypointList::Keypoint & kp) const;
  QPointF pointInWidgetToKeypointPosition(const QPoint &) const;

  QSize originalImageCropSize();
  void saveVisibleCenter();
  cimg_library::CImg<float> * _image;
  cimg_library::CImg<float> * _savedPreview;
  QSize _fullImageSize;
  double _currentZoomFactor;
  ZoomConstraint _zoomConstraint;

  /*
   * (0) for a 1:1 preview (GmicQt::PreviewFactorActualSize)
   * (1) for previewing the whole image (GmicQt::PreviewFactorFullImage)
   * (2) for 1/2 image
   * GmigQt::PreviewFactorAny
   */
  float _previewFactor;
  int _timerID;
  bool _previewEnabled;

  struct PreviewPoint {
    double x, y;
    bool isValid() const;
    bool operator!=(const PreviewPoint &) const;
    bool operator==(const PreviewPoint &) const;
    QPointF toPointF() const { return QPointF((qreal)x, (qreal)y); }
  };

  struct PreviewRect {
    double x;
    double y;
    double w;
    double h;
    bool operator!=(const PreviewRect &) const;
    bool operator==(const PreviewRect &) const;
    bool isFull() const;
    PreviewPoint center() const;
    PreviewPoint topLeft() const;
    void moveCenter(const PreviewPoint & p);
    void moveToCenter();
    QRectF toRectF() const { return QRectF((qreal)x, (qreal)y, (qreal)w, (qreal)h); }
    static const PreviewRect Full;
  };

  static const int RESIZE_DELAY = 400;
  static int roundedDistance(const QPoint & p1, const QPoint & p2);
  PreviewRect _visibleRect;
  PreviewPoint _savedVisibleCenter;

  bool _pendingResize;
  QPixmap _transparency;
  bool _savedPreviewIsValid;
  QRect _imagePosition;
  QPoint _mousePosition;
  QPixmap _transparentBackground;
  bool _paintOriginalImage;
  QSize _originalImageSize;
  QSize _originalImageScaledSize;
  bool _rightClickEnabled;
  QString _errorMessage;
  QString _overlayMessage;
  QImage _errorImage;
  KeypointList _keypoints;
  int _movedKeypointIndex;
  QPoint _movedKeypointOrigin;
  unsigned long _keypointTimestamp;
};

#endif // GMIC_QT_PREVIEWWIDGET_H

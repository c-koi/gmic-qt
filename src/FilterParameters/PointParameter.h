/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file PointParameter.h
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
#ifndef GMIC_QT_POINTPARAMETER_H
#define GMIC_QT_POINTPARAMETER_H

#include <QColor>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QPixmap>
#include <QPointF>
#include <QString>
#include <QToolButton>
#include "AbstractParameter.h"
class QSpinBox;
class QSlider;
class QLabel;
class QPushButton;

class PointParameter : public AbstractParameter {
  Q_OBJECT
public:
  PointParameter(QObject * parent = nullptr);
  ~PointParameter() override;
  bool addTo(QWidget *, int row) override;
  void addToKeypointList(KeypointList &) const override;
  void extractPositionFromKeypointList(KeypointList &) override;
  QString textValue() const override;
  void setValue(const QString & value) override;
  void reset() override;
  bool initFromText(const char * text, int & textLength) override;
  void setRemoved(bool on);

  static void resetDefaultColorIndex();

  void setVisibilityState(AbstractParameter::VisibilityState state) override;

public slots:
  void enableNotifications(bool);

private slots:
  void onSpinBoxChanged();
  void onRemoveButtonToggled(bool);

private:
  static int randomChannel();
  void connectSpinboxes();
  void disconnectSpinboxes();
  void pickColorFromDefaultColormap();
  void updateView();
  QString _name;
  QPointF _defaultPosition;
  bool _defaultRemovedStatus;
  QPointF _position;
  QColor _color;
  bool _removable;
  bool _burst;
  float _radius;
  bool _keepOpacityWhenSelected;

  QLabel * _label;
  QLabel * _colorLabel;
  QLabel * _labelX;
  QLabel * _labelY;
  QDoubleSpinBox * _spinBoxX;
  QDoubleSpinBox * _spinBoxY;
  QToolButton * _removeButton;
  bool _connected;
  bool _removed;
  QWidget * _rowCell;
  bool _notificationEnabled;
  static int _defaultColorNextIndex;
  static unsigned long _randomSeed;
};

#endif // GMIC_QT_POINTPARAMETER_H

/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FloatParameter.h
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
#ifndef _GMIC_QT_FLOATPARAMETER_H_
#define _GMIC_QT_FLOATPARAMETER_H_

#include <QString>
#include "AbstractParameter.h"
class QDoubleSpinBox;
class QSlider;
class QLabel;

class FloatParameter : public AbstractParameter {
  Q_OBJECT
public:
  FloatParameter(QObject * parent = 0);
  ~FloatParameter();
  void addTo(QWidget *, int row) override;
  QString textValue() const override;
  void setValue(const QString & value) override;
  void reset() override;
  bool initFromText(const char * text, int & textLength) override;

protected:
  void timerEvent(QTimerEvent * event) override;
public slots:
  void onSliderMoved(int);
  void onSliderValueChanged(int);
  void onSpinBoxChanged(double);

private:
  void connectSliderSpinBox();
  void disconnectSliderSpinBox();
  QString _name;
  float _min;
  float _max;
  float _default;
  float _value;
  QLabel * _label;
  QSlider * _slider;
  QDoubleSpinBox * _spinBox;
  int _timerId;
  static const int UPDATE_DELAY = 300;
  bool _connected;
};

#endif // _GMIC_QT_FLOATPARAMETER_H_

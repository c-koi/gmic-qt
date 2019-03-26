/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FloatParameter.cpp
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
#include "FilterParameters/FloatParameter.h"
#include <QDebug>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QLocale>
#include <QPalette>
#include <QSlider>
#include <QString>
#include <QTimerEvent>
#include <QWidget>
#include "DialogSettings.h"
#include "Globals.h"
#include "HtmlTranslator.h"

FloatParameter::FloatParameter(QObject * parent) : AbstractParameter(parent, true), _min(0), _max(0), _default(0), _value(0), _label(nullptr), _slider(nullptr), _spinBox(nullptr)
{
  _timerId = 0;
  _connected = false;
}

FloatParameter::~FloatParameter()
{
  delete _spinBox;
  delete _slider;
  delete _label;
}

bool FloatParameter::addTo(QWidget * widget, int row)
{
  _grid = dynamic_cast<QGridLayout *>(widget->layout());
  Q_ASSERT_X(_grid, __PRETTY_FUNCTION__, "No grid layout in widget");
  _row = row;
  delete _spinBox;
  delete _slider;
  delete _label;
  _slider = new QSlider(Qt::Horizontal, widget);
  _slider->setMinimumWidth(SLIDER_MIN_WIDTH);
  _slider->setRange(0, 1000);
  _slider->setValue(static_cast<int>(1000 * (_value - _min) / (_max - _min)));
  if (DialogSettings::darkThemeEnabled()) {
    QPalette p = _slider->palette();
    p.setColor(QPalette::Button, QColor(100, 100, 100));
    p.setColor(QPalette::Highlight, QColor(130, 130, 130));
    _slider->setPalette(p);
  }
  _spinBox = new QDoubleSpinBox(widget);
  _spinBox->setRange(_min, _max);
  _spinBox->setValue(_value);
  _spinBox->setDecimals(2);
  _spinBox->setSingleStep((_max - _min) / 100.0);
  _grid->addWidget(_label = new QLabel(_name, widget), row, 0, 1, 1);
  _grid->addWidget(_slider, row, 1, 1, 1);
  _grid->addWidget(_spinBox, row, 2, 1, 1);
  connectSliderSpinBox();
  return true;
}

QString FloatParameter::textValue() const
{
  QLocale currentLocale;
  QLocale::setDefault(QLocale::c());
  QString value = QString("%1").arg(_spinBox->value());
  QLocale::setDefault(currentLocale);
  return value;
}

void FloatParameter::setValue(const QString & value)
{
  _value = value.toFloat();
  if (_slider) {
    disconnectSliderSpinBox();
    _slider->setValue(static_cast<int>(1000 * (_value - _min) / (_max - _min)));
    _spinBox->setValue(_value);
    connectSliderSpinBox();
  }
}

void FloatParameter::reset()
{
  disconnectSliderSpinBox();
  _value = _default;
  _slider->setValue(static_cast<int>(1000 * (_value - _min) / (_max - _min)));
  _spinBox->setValue(_default);
  connectSliderSpinBox();
}

bool FloatParameter::initFromText(const char * text, int & textLength)
{
  textLength = 0;
  QList<QString> list = parseText("float", text, textLength);
  _name = HtmlTranslator::html2txt(list[0]);
  QList<QString> values = list[1].split(QChar(','));
  if (values.size() != 3) {
    return false;
  }
  bool ok1, ok2, ok3;
  _default = values[0].toFloat(&ok1);
  _min = values[1].toFloat(&ok2);
  _max = values[2].toFloat(&ok3);
  _value = _default;
  return ok1 && ok2 && ok3;
}

void FloatParameter::timerEvent(QTimerEvent * event)
{
  killTimer(event->timerId());
  _timerId = 0;
  notifyIfRelevant();
}

void FloatParameter::onSliderMoved(int value)
{
  float fValue = _min + (value / 1000.0) * (_max - _min);
  if (fValue != _value) {
    _spinBox->setValue(_value = fValue);
  }
}

void FloatParameter::onSliderValueChanged(int value)
{
  float fValue = _min + (value / 1000.0) * (_max - _min);
  if (fValue != _value) {
    _spinBox->setValue(_value = fValue);
  }
}

void FloatParameter::onSpinBoxChanged(double x)
{
  _value = x;
  disconnectSliderSpinBox();
  _slider->setValue(static_cast<int>(1000 * (_value - _min) / (_max - _min)));
  connectSliderSpinBox();
  if (_timerId) {
    killTimer(_timerId);
  }
  _timerId = startTimer(UPDATE_DELAY);
}

void FloatParameter::connectSliderSpinBox()
{
  if (_connected) {
    return;
  }
  connect(_slider, SIGNAL(sliderMoved(int)), this, SLOT(onSliderMoved(int)));
  connect(_slider, SIGNAL(valueChanged(int)), this, SLOT(onSliderValueChanged(int)));
  connect(_spinBox, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxChanged(double)));
  _connected = true;
}

void FloatParameter::disconnectSliderSpinBox()
{
  if (!_connected) {
    return;
  }
  _slider->disconnect(this);
  _spinBox->disconnect(this);
  _connected = false;
}

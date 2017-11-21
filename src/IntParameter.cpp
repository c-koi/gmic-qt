/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file IntParameter.cpp
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
#include "IntParameter.h"
#include "Common.h"
#include <QWidget>
#include <QGridLayout>
#include <QSpinBox>
#include <QSlider>
#include <QLabel>
#include <QTimerEvent>
#include <QPalette>
#include <DialogSettings.h>
#include "HtmlTranslator.h"

IntParameter::IntParameter(QObject * parent)
  : AbstractParameter(parent,true),
    _min(0),
    _max(0),
    _default(0),
    _value(0),
    _label(0),
    _slider(0),
    _spinBox(0)
{
  _timerId = 0;
  _connected = false;
}

IntParameter::~IntParameter()
{
  delete _spinBox;
  delete _slider;
  delete _label;
}

void
IntParameter::addTo(QWidget * widget, int row)
{
  QGridLayout * grid = dynamic_cast<QGridLayout*>(widget->layout());
  if (! grid) return;
  delete _spinBox;
  delete _slider;
  delete _label;
  _slider = new QSlider(Qt::Horizontal,widget);
  _slider->setMinimumWidth(SLIDER_MIN_WIDTH);
  _slider->setRange(_min,_max);
  _slider->setValue(_value);
  _spinBox = new QSpinBox(widget);
  _spinBox->setRange(_min,_max);
  _spinBox->setValue(_value);
  if ( DialogSettings::darkThemeEnabled() ) {
    QPalette p = _slider->palette();
    p.setColor(QPalette::Button, QColor(100,100,100));
    p.setColor(QPalette::Highlight, QColor(130,130,130));
    _slider->setPalette(p);
  }
  grid->addWidget(_label = new QLabel(_name,widget),row,0,1,1);
  grid->addWidget(_slider,row,1,1,1);
  grid->addWidget(_spinBox,row,2,1,1);
  connectSliderSpinBox();
}

QString
IntParameter::textValue() const
{
  return _spinBox->text();
}

void
IntParameter::setValue(const QString & value)
{
  _value = value.toInt();
  if (_spinBox) {
    disconnectSliderSpinBox();
    _spinBox->setValue(_value);
    _slider->setValue(_value);
    connectSliderSpinBox();
  }
}

void
IntParameter::reset()
{
  disconnectSliderSpinBox();
  _slider->setValue(_default);
  _spinBox->setValue(_default);
  _value = _default;
  connectSliderSpinBox();
}

bool
IntParameter::initFromText(const char * text, int & textLength)
{
  QList<QString> list = parseText("int",text,textLength);
  _name = HtmlTranslator::html2txt(list[0]);
  QList<QString> values = list[1].split(QChar(','));
  if ( values.size() != 3 ) {
    return false;
  }
  bool ok1, ok2, ok3;
  _default = values[0].toInt(&ok1);
  _min = values[1].toInt(&ok2);
  _max = values[2].toInt(&ok3);
  _value = _default;
  return ok1 && ok2 && ok3;
}

void
IntParameter::timerEvent(QTimerEvent * e)
{
  killTimer(e->timerId());
  _timerId = 0;
  notifyIfRelevant();
}

void IntParameter::onSliderMoved(int value)
{
  if (value != _value) {
    _spinBox->setValue(_value = value);
  }
}

void IntParameter::onSliderValueChanged(int value)
{
  if (value != _value) {
    _spinBox->setValue(_value = value);
  }
}

void
IntParameter::onSpinBoxChanged(int i)
{
  _value = i;
  _slider->setValue(i);
  if ( _timerId ) {
    killTimer(_timerId);
  }
  _timerId = startTimer(UPDATE_DELAY);
}

void IntParameter::connectSliderSpinBox()
{
  if ( _connected ) {
    return;
  }
  connect(_slider, SIGNAL(sliderMoved(int)),
          this, SLOT(onSliderMoved(int)));
  connect(_slider, SIGNAL(valueChanged(int)),
          this, SLOT(onSliderValueChanged(int)));
  connect(_spinBox, SIGNAL(valueChanged(int)),
          this, SLOT(onSpinBoxChanged(int)));
  _connected = true;
}

void IntParameter::disconnectSliderSpinBox()
{
  if ( !_connected ) {
    return;
  }
  _slider->disconnect(this);
  _spinBox->disconnect(this);
  _connected = false;
}

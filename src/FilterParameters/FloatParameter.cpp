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
#include <QGridLayout>
#include <QLabel>
#include <QLocale>
#include <QPalette>
#include <QSlider>
#include <QString>
#include <QTimerEvent>
#include <QWidget>
#include "FilterParameters/CustomDoubleSpinBox.h"
#include "FilterTextTranslator.h"
#include "Globals.h"
#include "HtmlTranslator.h"
#include "Logger.h"
#include "Settings.h"

namespace GmicQt
{

FloatParameter::FloatParameter(QObject * parent) : AbstractParameter(parent), _min(0), _max(0), _default(0), _value(0), _label(nullptr), _slider(nullptr), _spinBox(nullptr)
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

int FloatParameter::size() const
{
  return 1;
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
  _slider->setRange(0, SLIDER_MAX_RANGE);
  _slider->setValue(static_cast<int>(SLIDER_MAX_RANGE * (_value - _min) / (_max - _min)));
  if (Settings::darkThemeEnabled()) {
    QPalette p = _slider->palette();
    p.setColor(QPalette::Button, QColor(100, 100, 100));
    p.setColor(QPalette::Highlight, QColor(130, 130, 130));
    _slider->setPalette(p);
  }

  _spinBox = new CustomDoubleSpinBox(widget, _min, _max);
  _spinBox->setSingleStep(double(_max - _min) / 100.0);
  _spinBox->setValue((double)_value);
  _grid->addWidget(_label = new QLabel(_name, widget), row, 0, 1, 1);
  setTextSelectable(_label);
  _grid->addWidget(_slider, row, 1, 1, 1);
  _grid->addWidget(_spinBox, row, 2, 1, 1);

  connectSliderSpinBox();

  connect(_spinBox, &CustomDoubleSpinBox::editingFinished, [this]() { notifyIfRelevant(); });

  return true;
}

QString FloatParameter::value() const
{
  QLocale currentLocale;
  QLocale::setDefault(QLocale::c());
  QString value = QString("%1").arg(_spinBox->value());
  QLocale::setDefault(currentLocale);
  return value;
}

QString FloatParameter::defaultValue() const
{
  QLocale currentLocale;
  QLocale::setDefault(QLocale::c());
  QString value = QString("%1").arg(static_cast<double>(_default));
  QLocale::setDefault(currentLocale);
  return value;
}

void FloatParameter::setValue(const QString & value)
{
  bool ok = true;
  const float x = value.toFloat(&ok);
  if (!ok) {
    Logger::warning(QString("FloatParameter::setValue(\"%1\"): bad value").arg(value));
    return;
  }
  _value = x;
  if (_slider) {
    disconnectSliderSpinBox();
    _slider->setValue(static_cast<int>(SLIDER_MAX_RANGE * (_value - _min) / (_max - _min)));
    _spinBox->setValue((double)_value);
    connectSliderSpinBox();
  }
}

void FloatParameter::reset()
{
  disconnectSliderSpinBox();
  _value = _default;
  _slider->setValue(static_cast<int>(SLIDER_MAX_RANGE * (_value - _min) / (_max - _min)));
  _spinBox->setValue((double)_default);
  connectSliderSpinBox();
}

bool FloatParameter::initFromText(const QString & filterName, const char * text, int & textLength)
{
  textLength = 0;
  QList<QString> list = parseText("float", text, textLength);
  if (list.isEmpty()) {
    return false;
  }
  _name = HtmlTranslator::html2txt(FilterTextTranslator::translate(list[0], filterName));
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
  if (!_spinBox->unfinishedKeyboardEditing()) {
    notifyIfRelevant();
  }
}

void FloatParameter::onSliderMoved(int value)
{
  const float fValue = _min + (float(value) / static_cast<float>(SLIDER_MAX_RANGE)) * (_max - _min);
  if (fValue != _value) {
    _spinBox->setValue(_value = fValue);
  }
}

void FloatParameter::onSliderValueChanged(int value)
{
  const float fValue = _min + (float(value) / static_cast<float>(SLIDER_MAX_RANGE)) * (_max - _min);
  if (fValue != _value) {
    _spinBox->setValue(_value = fValue);
  }
}

void FloatParameter::onSpinBoxChanged(double x)
{
  _value = float(x);
  disconnectSliderSpinBox();
  _slider->setValue(static_cast<int>(SLIDER_MAX_RANGE * (_value - _min) / (_max - _min)));
  connectSliderSpinBox();
  if (_timerId) {
    killTimer(_timerId);
  }
  if (_spinBox->unfinishedKeyboardEditing()) {
    _timerId = 0;
  } else {
    _timerId = startTimer(UPDATE_DELAY);
  }
}

void FloatParameter::connectSliderSpinBox()
{
  if (_connected) {
    return;
  }
  connect(_slider, &QSlider::sliderMoved, this, &FloatParameter::onSliderMoved);
  connect(_slider, &QSlider::valueChanged, this, &FloatParameter::onSliderValueChanged);
  connect(_spinBox, QOverload<double>::of(&CustomDoubleSpinBox::valueChanged), this, &FloatParameter::onSpinBoxChanged);
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

} // namespace GmicQt

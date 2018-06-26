/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file BoolParameter.cpp
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
#include "FilterParameters/BoolParameter.h"
#include <QCheckBox>
#include <QDebug>
#include <QGridLayout>
#include <QLabel>
#include <QPalette>
#include <QWidget>
#include "Common.h"
#include "DialogSettings.h"
#include "HtmlTranslator.h"

BoolParameter::BoolParameter(QObject * parent) : AbstractParameter(parent, true), _default(false), _value(false), _label(0), _checkBox(0), _connected(false) {}

BoolParameter::~BoolParameter()
{
  delete _checkBox;
  delete _label;
}

void BoolParameter::addTo(QWidget * widget, int row)
{
  QGridLayout * grid = dynamic_cast<QGridLayout *>(widget->layout());
  if (!grid)
    return;
  delete _checkBox;
  delete _label;
  _checkBox = new QCheckBox(_name, widget);
  _checkBox->setChecked(_value);
  if (DialogSettings::darkThemeEnabled()) {
    QPalette p = _checkBox->palette();
    p.setColor(QPalette::Text, DialogSettings::CheckBoxTextColor);
    p.setColor(QPalette::Base, DialogSettings::CheckBoxBaseColor);
    _checkBox->setPalette(p);
  }
  grid->addWidget(_checkBox, row, 0, 1, 3);
  connectCheckBox();
}

QString BoolParameter::textValue() const
{
  return _value ? "1" : "0";
}

void BoolParameter::setValue(const QString & value)
{
  _value = (value == "1");
  if (_checkBox) {
    _checkBox->setChecked(_value);
  }
}

void BoolParameter::reset()
{
  _checkBox->setChecked(_default);
  _value = _default;
}

void BoolParameter::onCheckBoxChanged(bool on)
{
  _value = on;
  notifyIfRelevant();
}

void BoolParameter::connectCheckBox()
{
  if (_connected) {
    return;
  }
  connect(_checkBox, SIGNAL(toggled(bool)), this, SLOT(onCheckBoxChanged(bool)));
  _connected = true;
}

void BoolParameter::disconnectCheckBox()
{
  if (!_connected) {
    return;
  }
  _checkBox->disconnect(this);
  _connected = false;
}

bool BoolParameter::initFromText(const char * text, int & textLength)
{
  QList<QString> list = parseText("bool", text, textLength);
  _name = HtmlTranslator::html2txt(list[0]);
  _value = _default = (list[1].startsWith("true") || list[1].startsWith("1"));
  return true;
}

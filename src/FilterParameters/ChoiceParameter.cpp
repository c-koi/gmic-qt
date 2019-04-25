/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file ChoiceParameter.cpp
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
#include "FilterParameters/ChoiceParameter.h"
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QWidget>
#include "Common.h"
#include "HtmlTranslator.h"

ChoiceParameter::ChoiceParameter(QObject * parent) : AbstractParameter(parent, true), _default(0), _value(0), _label(nullptr), _comboBox(nullptr), _connected(false) {}

ChoiceParameter::~ChoiceParameter()
{
  delete _comboBox;
  delete _label;
}

bool ChoiceParameter::addTo(QWidget * widget, int row)
{
  _grid = dynamic_cast<QGridLayout *>(widget->layout());
  Q_ASSERT_X(_grid, __PRETTY_FUNCTION__, "No grid layout in widget");
  _row = row;
  delete _comboBox;
  delete _label;

  _comboBox = new QComboBox(widget);
  _comboBox->addItems(_choices);
  _comboBox->setCurrentIndex(_value);

  _grid->addWidget(_label = new QLabel(_name, widget), row, 0, 1, 1);
  _grid->addWidget(_comboBox, row, 1, 1, 2);
  connectComboBox();
  return true;
}

QString ChoiceParameter::textValue() const
{
  return QString("%1").arg(_comboBox->currentIndex());
}

void ChoiceParameter::setValue(const QString & value)
{
  _value = value.toInt();
  if (_comboBox) {
    disconnectComboBox();
    _comboBox->setCurrentIndex(_value);
    connectComboBox();
  }
}

void ChoiceParameter::reset()
{
  disconnectComboBox();
  _comboBox->setCurrentIndex(_default);
  _value = _default;
  connectComboBox();
}

bool ChoiceParameter::initFromText(const char * text, int & textLength)
{
  QStringList list = parseText("choice", text, textLength);
  if (list.isEmpty()) {
    return false;
  }
  _name = HtmlTranslator::html2txt(list[0]);
  _choices = list[1].split(QChar(','));
  bool ok;
  if (_choices.isEmpty()) {
    return false;
  }
  _default = _choices[0].toInt(&ok);
  if (!ok) {
    _default = 0;
  } else {
    _choices.pop_front();
  }
  QList<QString>::iterator it = _choices.begin();
  while (it != _choices.end()) {
    *it = it->trimmed().remove(QRegExp("^\"")).remove(QRegExp("\"$"));
    *it = HtmlTranslator::html2txt(*it);
    ++it;
  }
  _value = _default;
  return true;
}

void ChoiceParameter::onComboBoxIndexChanged(int i)
{
  _value = i;
  notifyIfRelevant();
}

void ChoiceParameter::connectComboBox()
{
  if (_connected) {
    return;
  }
  connect(_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBoxIndexChanged(int)));
  _connected = true;
}

void ChoiceParameter::disconnectComboBox()
{
  if (!_connected) {
    return;
  }
  _comboBox->disconnect(this);
  _connected = false;
}

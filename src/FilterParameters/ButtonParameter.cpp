/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file ButtonParameter.cpp
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
#include "FilterParameters/ButtonParameter.h"
#include <QDebug>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include "Common.h"
#include "HtmlTranslator.h"

ButtonParameter::ButtonParameter(QObject * parent) : AbstractParameter(parent, true), _value(false), _pushButton(nullptr), _alignment(Qt::AlignHCenter) {}

ButtonParameter::~ButtonParameter()
{
  delete _pushButton;
}

bool ButtonParameter::addTo(QWidget * widget, int row)
{
  _grid = dynamic_cast<QGridLayout *>(widget->layout());
  Q_ASSERT_X(_grid, __PRETTY_FUNCTION__, "No grid layout in widget");
  _row = row;
  delete _pushButton;
  _pushButton = new QPushButton(_text, widget);
  _pushButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  _grid->addWidget(_pushButton, row, 0, 1, 3, _alignment);
  connect(_pushButton, SIGNAL(clicked(bool)), this, SLOT(onPushButtonClicked(bool)));
  return true;
}

QString ButtonParameter::textValue() const
{
  return _value ? QString("1") : QString("0");
}

void ButtonParameter::setValue(const QString & s)
{
  _value = (s == "1");
}

void ButtonParameter::clear()
{
  _value = false;
}

void ButtonParameter::reset() {}

void ButtonParameter::onPushButtonClicked(bool)
{
  _value = true;
  notifyIfRelevant();
}

bool ButtonParameter::initFromText(const char * text, int & textLength)
{
  QList<QString> list = parseText("button", text, textLength);
  _text = HtmlTranslator::html2txt(list[0]);
  QString & alignment = list[1];
  if (alignment.isEmpty()) {
    return true;
  }
  float a = alignment.toFloat();
  if (a == 0.0f) {
    _alignment = Qt::AlignLeft;
  } else if (a == 1.0f) {
    _alignment = Qt::AlignRight;
  } else {
    _alignment = Qt::AlignCenter;
  }
  return true;
}

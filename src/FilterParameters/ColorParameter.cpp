/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file ColorParameter.cpp
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
#include "FilterParameters/ColorParameter.h"
#include <QApplication>
#include <QColorDialog>
#include <QDebug>
#include <QFont>
#include <QFontMetrics>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QWidget>
#include <cstdio>
#include "Common.h"
#include "DialogSettings.h"
#include "HtmlTranslator.h"

ColorParameter::ColorParameter(QObject * parent) : AbstractParameter(parent, true), _default(0, 0, 0, 0), _value(_default), _alphaChannel(false), _label(nullptr), _button(nullptr), _dialog(nullptr) {}

ColorParameter::~ColorParameter()
{
  delete _button;
  delete _label;
  delete _dialog;
}

bool ColorParameter::addTo(QWidget * widget, int row)
{
  _grid = dynamic_cast<QGridLayout *>(widget->layout());
  Q_ASSERT_X(_grid, __PRETTY_FUNCTION__, "No grid layout in widget");
  _row = row;
  delete _button;
  delete _label;

  _button = new QPushButton(widget);
  _button->setText("");

  QFontMetrics fm(widget->font());
  QRect r = fm.boundingRect("CLR");
  _pixmap = QPixmap(r.width(), r.height());
  _button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

  _button->setIconSize(_pixmap.size());

  updateButtonColor();

  _grid->addWidget(_label = new QLabel(_name, widget), row, 0, 1, 1);
  _grid->addWidget(_button, row, 1, 1, 1);
  connect(_button, SIGNAL(clicked()), this, SLOT(onButtonPressed()));
  return true;
}

QString ColorParameter::textValue() const
{
  const QColor & c = _value;
  if (_alphaChannel) {
    return QString("%1,%2,%3,%4").arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha());
  }
  return QString("%1,%2,%3").arg(c.red()).arg(c.green()).arg(c.blue());
}

void ColorParameter::setValue(const QString & value)
{
  QStringList list = value.split(",");
  const int red = list[0].toInt();
  const int green = list[1].toInt();
  const int blue = list[2].toInt();
  if ((list.size() == 4) && _alphaChannel) {
    const int alpha = list[3].toInt();
    _value = QColor(red, green, blue, alpha);
  } else {
    _value = QColor(red, green, blue);
  }
  if (_button) {
    updateButtonColor();
  }
}

void ColorParameter::reset()
{
  _value = _default;
  updateButtonColor();
}

bool ColorParameter::initFromText(const char * text, int & textLength)
{
  QList<QString> list = parseText("color", text, textLength);
  if (list.isEmpty()) {
    return false;
  }
  _name = HtmlTranslator::html2txt(list[0]);

  // color(#e9cc00) and color(#e9cc00ff)
  const QString trimmed = list[1].trimmed();
  QRegExp colorRE("#[0-9a-fA-F]{6,8}");
  if (colorRE.exactMatch(trimmed)) {
    _default = QColor(trimmed.left(7));
    if (trimmed.length() == 9) {
      _alphaChannel = true;
      _default.setAlpha(trimmed.right(2).toInt(nullptr, 16));
    } else {
      _alphaChannel = false;
    }
    _value = _default;
    return true;
  }

  // color(120,100,10) and color(120,100,10,128)
  QList<QString> channels = list[1].split(",");
  const int n = channels.size();
  bool okR = true, okG = true, okB = true, okA = true;
  int r = (n > 0) ? channels[0].toInt(&okR) : 0;
  int g = (n >= 2) ? channels[1].toInt(&okG) : r;
  int b = (n >= 3) ? channels[2].toInt(&okB) : (n == 1) ? r : 0;
  if (channels.size() == 4) {
    int a = channels[3].toInt(&okA);
    _default = _value = QColor(r, g, b, a);
    _alphaChannel = true;
  } else {
    _default = _value = QColor(r, g, b);
  }
  return okR && okG && okB && okA;
}

void ColorParameter::onButtonPressed()
{
  QColor color = QColorDialog::getColor(_value, QApplication::activeWindow(), tr("Select color"),
                                        (DialogSettings::nativeColorDialogs() ? QColorDialog::ColorDialogOptions() : QColorDialog::DontUseNativeDialog) |
                                            (_alphaChannel ? QColorDialog::ShowAlphaChannel : QColorDialog::ColorDialogOptions()));
  if (color.isValid()) {
    _value = color;
    updateButtonColor();
    notifyIfRelevant();
  }
}

void ColorParameter::updateButtonColor()
{
  QPainter painter(&_pixmap);
  QColor color(_value);
  if (_alphaChannel) {
    painter.drawImage(0, 0, QImage(":resources/transparency.png"));
  }
  painter.setBrush(color);
  painter.setPen(Qt::black);
  painter.drawRect(0, 0, _pixmap.width() - 1, _pixmap.height() - 1);
  _button->setIcon(_pixmap);
}

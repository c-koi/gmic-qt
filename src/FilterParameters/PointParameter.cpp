/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file PointParameter.cpp
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
#include "FilterParameters/PointParameter.h"
#include <QApplication>
#include <QColorDialog>
#include <QDebug>
#include <QFont>
#include <QFontMetrics>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QSpacerItem>
#include <QWidget>
#include <cstdio>
#include "Common.h"
#include "DialogSettings.h"
#include "HtmlTranslator.h"
#include "KeypointList.h"

PointParameter::PointParameter(QObject * parent) : AbstractParameter(parent, true), _defaultPosition(0, 0), _position(0, 0), _removable(false), _burst(false)
{
  _label = nullptr;
  _colorLabel = nullptr;
  _labelX = nullptr;
  _labelY = nullptr;
  _spinBoxX = nullptr;
  _spinBoxY = nullptr;
  _removeButton = nullptr;
  _rowCell = nullptr;
  _notificationEnabled = false;
}

PointParameter::~PointParameter()
{
  delete _label;
  delete _rowCell;
}

void PointParameter::addTo(QWidget * widget, int row)
{
  QGridLayout * grid = dynamic_cast<QGridLayout *>(widget->layout());
  if (!grid) {
    return;
  }
  delete _label;
  delete _rowCell;

  _rowCell = new QWidget(widget);
  QHBoxLayout * hbox = new QHBoxLayout(_rowCell);
  hbox->setMargin(0);
  hbox->addWidget(_colorLabel = new QLabel(_rowCell));

  QFontMetrics fm(widget->font());
  QRect r = fm.boundingRect("CLR");
  _colorLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  QPixmap pixmap(r.width(), r.height());
  QPainter painter(&pixmap);
  painter.setBrush(_color);
  painter.setPen(Qt::black);
  if (_color.alpha() != 255) {
    painter.drawImage(0, 0, QImage(":resources/transparency.png"));
  }
  painter.drawRect(0, 0, pixmap.width() - 1, pixmap.height() - 1);
  _colorLabel->setPixmap(pixmap);

  hbox->addWidget(_labelX = new QLabel("X", _rowCell));
  hbox->addWidget(_spinBoxX = new QDoubleSpinBox(_rowCell));
  hbox->addWidget(_labelY = new QLabel("Y", _rowCell));
  hbox->addWidget(_spinBoxY = new QDoubleSpinBox(_rowCell));
  if (_removable) {
    hbox->addWidget(_removeButton = new QToolButton(_rowCell));
    _removeButton->setCheckable(true);
    _removeButton->setChecked(false);
    _removeButton->setIcon(DialogSettings::RemoveIcon);
    connect(_removeButton, SIGNAL(toggled(bool)), this, SLOT(onRemoveButtonToggled(bool)));
  } else {
    _removeButton = nullptr;
    hbox->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Fixed));
  }
  _spinBoxX->setRange(0, 100);
  _spinBoxY->setRange(0, 100);
  _spinBoxX->setValue(_position.x());
  _spinBoxY->setValue(_position.y());

  grid->addWidget(_label = new QLabel(_name, widget), row, 0, 1, 1);
  grid->addWidget(_rowCell, row, 1, 1, 2);

  connect(_spinBoxX, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxChanged()));
  connect(_spinBoxY, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxChanged()));
}

void PointParameter::addToKeypointList(KeypointList & list) const
{
  list.add(KeypointList::Keypoint(_position.x(), _position.y(), _color, _removable, _burst));
}

void PointParameter::extractPositionFromKeypointList(KeypointList & list)
{
  Q_ASSERT_X(!list.isEmpty(), __PRETTY_FUNCTION__, "Keypoint list is empty");
  enableNotifications(false);
  KeypointList::Keypoint kp = list.front();
  _position.setX(kp.x);
  _position.setY(kp.y);
  if (_spinBoxX) {
    _spinBoxX->setValue(kp.x);
    _spinBoxY->setValue(kp.y);
  }
  list.pop_front();
  enableNotifications(true);
}

QString PointParameter::textValue() const
{
  if (_removable && _removeButton->isChecked()) {
    return "nan,nan";
  } else {
    return QString("%1,%2").arg(_position.x()).arg(_position.y());
  }
}

void PointParameter::setValue(const QString & value)
{
  QStringList list = value.split(",");
  if (list.size() == 2) {
    bool ok;
    float x = list[0].toFloat(&ok);
    bool xNaN = (list[0].toUpper() == "NAN");
    if (ok && !xNaN) {
      _position.setX(x);
    }
    float y = list[1].toFloat(&ok);
    bool yNaN = (list[1].toUpper() == "NAN");
    if (ok && !yNaN) {
      _position.setY(y);
    }
    if (_removable && _removeButton) {
      if (_removeButton->isChecked() && !xNaN && !yNaN) {
        _removeButton->setChecked(false);
      }
      if (!_removeButton->isChecked() && xNaN && yNaN) {
        _removeButton->setChecked(true);
      }
    }
  }
}

void PointParameter::reset()
{
  _position = _defaultPosition;
}

// P = point(x,y,removable{0,1},burst{0,1},r,g,b,a)
bool PointParameter::initFromText(const char * text, int & textLength)
{
  QList<QString> list = parseText("point", text, textLength);
  _name = HtmlTranslator::html2txt(list[0]);
  QList<QString> params = list[1].split(",");

  bool ok = true;

  _defaultPosition.setX(50.0);
  _defaultPosition.setY(50.0);
  _defaultPosition = _position;
  _color.setRgb(255, 255, 255, 255);
  _burst = false;
  _removable = false;

  if (params.size() >= 1) {
    float x = params[0].toFloat(&ok);
    if (!ok) {
      return false;
    }
    _defaultPosition.setX(x);
  }

  if (params.size() >= 2) {
    float y = params[1].toFloat(&ok);
    if (!ok) {
      return false;
    }
    _defaultPosition.setY(y);
  }

  if (params.size() >= 3) {
    bool r = params[2].toInt(&ok);
    if (!ok) {
      return false;
    }
    _removable = r;
  }

  if (params.size() >= 4) {
    bool b = params[3].toInt(&ok);
    if (!ok) {
      return false;
    }
    _burst = b;
  }

  if (params.size() >= 5) {
    int r = params[4].toInt(&ok);
    if (!ok) {
      return false;
    }
    _color.setRed(r);
  }

  if (params.size() >= 6) {
    int g = params[5].toInt(&ok);
    if (!ok) {
      return false;
    }
    _color.setGreen(g);
  }

  if (params.size() >= 7) {
    int b = params[6].toInt(&ok);
    if (!ok) {
      return false;
    }
    _color.setBlue(b);
  }

  if (params.size() >= 8) {
    int alpha = params[7].toInt(&ok);
    if (!ok) {
      return false;
    }
    _color.setAlpha(alpha);
  }
  _position = _defaultPosition;
  return true;
}

void PointParameter::enableNotifications(bool on)
{
  _notificationEnabled = on;
}

void PointParameter::onSpinBoxChanged()
{
  _position = QPointF(_spinBoxX->value(), _spinBoxY->value());
  if (_notificationEnabled) {
    notifyIfRelevant();
  }
}

void PointParameter::onRemoveButtonToggled(bool on)
{
  _spinBoxX->setDisabled(on);
  _spinBoxY->setDisabled(on);
  _labelX->setDisabled(on);
  _labelY->setDisabled(on);
  _removeButton->setIcon(on ? DialogSettings::AddIcon : DialogSettings::RemoveIcon);
  notifyIfRelevant();
}

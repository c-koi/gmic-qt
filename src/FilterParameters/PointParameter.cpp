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
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "Common.h"
#include "DialogSettings.h"
#include "FilterTextTranslator.h"
#include "HtmlTranslator.h"
#include "KeypointList.h"

int PointParameter::_defaultColorNextIndex = 0;
unsigned long PointParameter::_randomSeed = 12345;

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
  _notificationEnabled = true;
  _connected = false;
  _defaultRemovedStatus = false;
  _radius = KeypointList::Keypoint::DefaultRadius;
  _keepOpacityWhenSelected = false;
  _removed = false;
  setRemoved(false);
}

PointParameter::~PointParameter()
{
  delete _label;
  delete _rowCell;
}

bool PointParameter::addTo(QWidget * widget, int row)
{
  _grid = dynamic_cast<QGridLayout *>(widget->layout());
  Q_ASSERT_X(_grid, __PRETTY_FUNCTION__, "No grid layout in widget");
  _row = row;
  delete _label;
  delete _rowCell;

  _rowCell = new QWidget(widget);
  auto hbox = new QHBoxLayout(_rowCell);
  hbox->setMargin(0);
  hbox->addWidget(_colorLabel = new QLabel(_rowCell));

  QFontMetrics fm(widget->font());
  QRect r = fm.boundingRect("CLR");
  _colorLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  QPixmap pixmap(r.width(), r.height());
  QPainter painter(&pixmap);
  painter.setBrush(QColor(_color.red(), _color.green(), _color.blue()));
  painter.setPen(Qt::black);
  painter.drawRect(0, 0, pixmap.width() - 1, pixmap.height() - 1);
  _colorLabel->setPixmap(pixmap);

  hbox->addWidget(_labelX = new QLabel("X", _rowCell));
  hbox->addWidget(_spinBoxX = new QDoubleSpinBox(_rowCell));
  hbox->addWidget(_labelY = new QLabel("Y", _rowCell));
  hbox->addWidget(_spinBoxY = new QDoubleSpinBox(_rowCell));
  if (_removable) {
    hbox->addWidget(_removeButton = new QToolButton(_rowCell));
    _removeButton->setCheckable(true);
    _removeButton->setChecked(_removed);
    _removeButton->setIcon(DialogSettings::RemoveIcon);
  } else {
    _removeButton = nullptr;
  }
  hbox->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Fixed));
  _spinBoxX->setRange(-200.0, 300.0);
  _spinBoxY->setRange(-200.0, 300.0);
  _spinBoxX->setValue(_position.x());
  _spinBoxY->setValue(_position.y());
  _grid->addWidget(_label = new QLabel(_name, widget), row, 0, 1, 1);
  _grid->addWidget(_rowCell, row, 1, 1, 2);

#ifdef _GMIC_QT_DEBUG_
  _label->setToolTip(QString("Burst: %1").arg(_burst ? "on" : "off"));
#endif

  setRemoved(_removed);
  connectSpinboxes();
  return true;
}

void PointParameter::addToKeypointList(KeypointList & list) const
{
  if (_removable && _removed) {
    list.add(KeypointList::Keypoint(_color, _removable, _burst, _radius, _keepOpacityWhenSelected));
  } else {
    list.add(KeypointList::Keypoint(_position.x(), _position.y(), _color, _removable, _burst, _radius, _keepOpacityWhenSelected));
  }
}

void PointParameter::extractPositionFromKeypointList(KeypointList & list)
{
  Q_ASSERT_X(!list.isEmpty(), __PRETTY_FUNCTION__, "Keypoint list is empty");
  enableNotifications(false);
  KeypointList::Keypoint kp = list.front();
  if (!kp.isNaN()) {
    _position.setX(kp.x);
    _position.setY(kp.y);
    if (_spinBoxX) {
      _spinBoxX->setValue(kp.x);
      _spinBoxY->setValue(kp.y);
    }
  }
  list.pop_front();
  enableNotifications(true);
}

QString PointParameter::textValue() const
{
  if (_removed) {
    return "nan,nan";
  }
  return QString("%1,%2").arg(_position.x()).arg(_position.y());
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
    _removed = (_removable && xNaN && yNaN);

    updateView();
  }
}

void PointParameter::setVisibilityState(AbstractParameter::VisibilityState state)
{
  AbstractParameter::setVisibilityState(state);
  if (state & VisibleParameter) {
    updateView();
  }
}

void PointParameter::updateView()
{
  if (not _spinBoxX) {
    return;
  }
  disconnectSpinboxes();
  if (_removeButton) {
    setRemoved(_removed);
    _removeButton->setChecked(_removed);
  }
  if (!_removed) {
    _spinBoxX->setValue(_position.x());
    _spinBoxY->setValue(_position.y());
  }
  connectSpinboxes();
}

void PointParameter::reset()
{
  _position = _defaultPosition;
  enableNotifications(false);
  if (_spinBoxX) {
    _spinBoxX->setValue(_defaultPosition.rx());
    _spinBoxY->setValue(_defaultPosition.ry());
  }
  if (_removeButton && _removable) {
    _removeButton->setChecked((_removed = _defaultRemovedStatus));
  }
  enableNotifications(true);
}

// P = point(x,y,removable{(0),1},burst{(0),1},r,g,b,a{negative->keepOpacityWhenSelected},radius,widget_visible{0|(1)})
bool PointParameter::initFromText(const char * text, int & textLength)
{
  QList<QString> list = parseText("point", text, textLength);
  if (list.isEmpty()) {
    return false;
  }
  _name = HtmlTranslator::html2txt(FilterTextTranslator::translate(list[0]));
  QList<QString> params = list[1].split(",");

  bool ok = true;

  _defaultPosition.setX(50.0);
  _defaultPosition.setY(50.0);
  _defaultPosition = _position;
  _color.setRgb(255, 255, 255, 255);
  _burst = false;
  _removable = false;
  _radius = KeypointList::Keypoint::DefaultRadius;
  _keepOpacityWhenSelected = false;

  float x = 50.0f;
  float y = 50.0f;
  _removed = false;
  bool xNaN = true;
  bool yNaN = true;

  if (!params.isEmpty()) {
    x = params[0].toFloat(&ok);
    xNaN = (params[0].toUpper() == "NAN");
    if (!ok) {
      return false;
    }
    if (xNaN) {
      x = 50.0;
    }
  }

  if (params.size() >= 2) {
    y = params[1].toFloat(&ok);
    yNaN = (params[1].toUpper() == "NAN");
    if (!ok) {
      return false;
    }
    if (yNaN) {
      y = 50.0;
    }
  }

  _defaultPosition.setX(static_cast<qreal>(x));
  _defaultPosition.setY(static_cast<qreal>(y));
  _removed = _defaultRemovedStatus = (xNaN || yNaN);

  if (params.size() >= 3) {
    int removable = params[2].toInt(&ok);
    if (!ok) {
      return false;
    }
    switch (removable) {
    case -1:
      _removable = _removed = _defaultRemovedStatus = true;
      break;
    case 0:
      _removable = _removed = false;
      break;
    case 1:
      _removable = true;
      _defaultRemovedStatus = _removed = (xNaN && yNaN);
      break;
    default:
      return false;
    }
  }

  if (params.size() >= 4) {
    bool burst = params[3].toInt(&ok);
    if (!ok) {
      return false;
    }
    _burst = burst;
  }

  if (params.size() >= 5) {
    int red = params[4].toInt(&ok);
    if (!ok) {
      return false;
    }
    _color.setRed(red);
    _color.setGreen(red);
    _color.setBlue(red);
  } else {
    pickColorFromDefaultColormap();
  }

  if (params.size() >= 6) {
    int green = params[5].toInt(&ok);
    if (!ok) {
      return false;
    }
    _color.setGreen(green);
    _color.setBlue(0);
  }

  if (params.size() >= 7) {
    int blue = params[6].toInt(&ok);
    if (!ok) {
      return false;
    }
    _color.setBlue(blue);
  }

  if (params.size() >= 8) {
    int alpha = params[7].toInt(&ok);
    if (!ok) {
      return false;
    }
    if (params[7].trimmed().startsWith("-") || (alpha < 0)) {
      _keepOpacityWhenSelected = true;
    }
    _color.setAlpha(std::abs(alpha));
  }

  if (params.size() >= 9) {
    QString s = params[8].trimmed();
    if (s.endsWith("%")) {
      s.chop(1);
      _radius = -s.toFloat(&ok);
    } else {
      _radius = s.toFloat(&ok);
    }
    if (!ok) {
      return false;
    }
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

void PointParameter::setRemoved(bool on)
{
  _removed = on;
  if (_spinBoxX) {
    _spinBoxX->setDisabled(on);
    _spinBoxY->setDisabled(on);
    _labelX->setDisabled(on);
    _labelY->setDisabled(on);
    if (_removeButton) {
      _removeButton->setIcon(on ? DialogSettings::AddIcon : DialogSettings::RemoveIcon);
    }
  }
}

void PointParameter::resetDefaultColorIndex()
{
  _defaultColorNextIndex = 0;
  _randomSeed = 12345;
}

void PointParameter::onRemoveButtonToggled(bool on)
{
  setRemoved(on);
  notifyIfRelevant();
}

int PointParameter::randomChannel()
{
  int value = (_randomSeed / 65536) % 256;
  _randomSeed = _randomSeed * 1103515245 + 12345;
  return value;
}

void PointParameter::connectSpinboxes()
{
  if (_connected || !_spinBoxX) {
    return;
  }
  connect(_spinBoxX, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxChanged()));
  connect(_spinBoxY, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxChanged()));
  if (_removable && _removeButton) {
    connect(_removeButton, SIGNAL(toggled(bool)), this, SLOT(onRemoveButtonToggled(bool)));
  }
  _connected = true;
}

void PointParameter::disconnectSpinboxes()
{
  if (!_connected || !_spinBoxX) {
    return;
  }
  _spinBoxX->disconnect(this);
  _spinBoxY->disconnect(this);
  if (_removable && _removeButton) {
    _removeButton->disconnect(this);
  }
  _connected = false;
}

void PointParameter::pickColorFromDefaultColormap()
{
  switch (_defaultColorNextIndex) {
  case 0:
    _color.setRgb(255, 255, 255, 255);
    break;
  case 1:
    _color = Qt::red;
    break;
  case 2:
    _color = Qt::green;
    break;
  case 3:
    _color.setRgb(64, 64, 255, 255);
    break;
  case 4:
    _color = Qt::cyan;
    break;
  case 5:
    _color = Qt::magenta;
    break;
  case 6:
    _color = Qt::yellow;
    break;
  default:
    _color.setRgb(randomChannel(), randomChannel(), randomChannel());
  }
  ++_defaultColorNextIndex;
}

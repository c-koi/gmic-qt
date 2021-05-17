/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file AbstractParameter.cpp
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
#include "FilterParameters/AbstractParameter.h"
#include <QDebug>
#include <QGridLayout>
#include <cstring>
#include "Common.h"
#include "FilterParameters/BoolParameter.h"
#include "FilterParameters/ButtonParameter.h"
#include "FilterParameters/ChoiceParameter.h"
#include "FilterParameters/ColorParameter.h"
#include "FilterParameters/ConstParameter.h"
#include "FilterParameters/FileParameter.h"
#include "FilterParameters/FloatParameter.h"
#include "FilterParameters/FolderParameter.h"
#include "FilterParameters/IntParameter.h"
#include "FilterParameters/LinkParameter.h"
#include "FilterParameters/NoteParameter.h"
#include "FilterParameters/PointParameter.h"
#include "FilterParameters/SeparatorParameter.h"
#include "FilterParameters/TextParameter.h"
#include "Logger.h"

namespace GmicQt
{

const QStringList AbstractParameter::NoValueParameters = {"link", "note", "separator"};

AbstractParameter::AbstractParameter(QObject * parent) : QObject(parent)
{
  _update = true;
  _defaultVisibilityState = VisibleParameter;
  _visibilityState = VisibleParameter;
  _visibilityPropagation = PropagateNone;
  _row = -1;
  _grid = nullptr;
}

AbstractParameter::~AbstractParameter() {}

bool AbstractParameter::isActualParameter() const
{
  return size() > 0;
}

bool AbstractParameter::isQuoted() const
{
  return false;
}

void AbstractParameter::clear()
{
  // Used to clear the value of a ButtonParameter
}

void AbstractParameter::addToKeypointList(KeypointList &) const {}

void AbstractParameter::extractPositionFromKeypointList(KeypointList &) {}

AbstractParameter * AbstractParameter::createFromText(const char * text, int & length, QString & error, QObject * parent)
{
  AbstractParameter * result = nullptr;
  QString line = text;
  error.clear();

#define IS_OF_TYPE(ptype) (QRegExp("^[^=]*\\s*=\\s*_?" ptype, Qt::CaseInsensitive).indexIn(line) == 0)

#define PREFIX "^[^=]*\\s*=\\s*_?"
  if (IS_OF_TYPE("int")) {
    result = new IntParameter(parent);
  } else if (IS_OF_TYPE("float")) {
    result = new FloatParameter(parent);
  } else if (IS_OF_TYPE("bool")) {
    result = new BoolParameter(parent);
  } else if (IS_OF_TYPE("choice")) {
    result = new ChoiceParameter(parent);
  } else if (IS_OF_TYPE("color")) {
    result = new ColorParameter(parent);
  } else if (IS_OF_TYPE("separator")) {
    result = new SeparatorParameter(parent);
  } else if (IS_OF_TYPE("note")) {
    result = new NoteParameter(parent);
  } else if (IS_OF_TYPE("file") || IS_OF_TYPE("filein") || IS_OF_TYPE("fileout")) {
    result = new FileParameter(parent);
  } else if (IS_OF_TYPE("folder")) {
    result = new FolderParameter(parent);
  } else if (IS_OF_TYPE("text")) {
    result = new TextParameter(parent);
  } else if (IS_OF_TYPE("link")) {
    result = new LinkParameter(parent);
  } else if (IS_OF_TYPE("value")) {
    result = new ConstParameter(parent);
  } else if (IS_OF_TYPE("button")) {
    result = new ButtonParameter(parent);
  } else if (IS_OF_TYPE("point")) {
    result = new PointParameter(parent);
  }
  if (result) {
    if (!result->initFromText(text, length)) {
      delete result;
      result = nullptr;
      if (!line.isEmpty()) {
        QRegExp nameRegExp("^[^=]*\\s*=");
        if (nameRegExp.indexIn(line) == 0) {
          QString name = nameRegExp.cap(0).remove(QRegExp("=$"));
          error = "Parameter name: " + name + "\n" + error;
        }
      }
    }
  } else {
    if (!line.isEmpty()) {
      QRegExp nameRegExp("^[^=]*\\s*=");
      if (nameRegExp.indexIn(line) == 0) {
        QString name = nameRegExp.cap(0).remove(QRegExp("=$"));
        QRegExp typeRegExp("^[^=]*\\s*=\\s*_?([^\\( ]*)\\s*\\(");
        if (typeRegExp.indexIn(line) == 0) {
          error = "Parameter name: " + name + "\n" + "Type <" + typeRegExp.cap(1) + "> is not recognized\n" + error;
        } else {
          error = "Parameter name: " + name + "\n" + error;
        }
      }
    }
  }
  return result;
}

AbstractParameter::VisibilityState AbstractParameter::defaultVisibilityState() const
{
  return _defaultVisibilityState;
}

void AbstractParameter::setVisibilityState(AbstractParameter::VisibilityState state)
{
  if (state == UnspecifiedVisibilityState) {
    setVisibilityState(defaultVisibilityState());
    return;
  }
  _visibilityState = state;
  if (!_grid || _row == -1) {
    return;
  }
  for (int col = 0; col < 5; ++col) {
    QLayoutItem * item = _grid->itemAtPosition(_row, col);
    if (item) {
      auto widget = item->widget();
      switch (state & 3) {
      case VisibleParameter:
        widget->setEnabled(true);
        widget->show();
        break;
      case DisabledParameter:
        widget->setEnabled(false);
        widget->show();
        break;
      case HiddenParameter:
        widget->hide();
        break;
      case UnspecifiedVisibilityState:
        // Taken care above (if)
        break;
      }
    }
  }
}

AbstractParameter::VisibilityState AbstractParameter::visibilityState() const
{
  return _visibilityState;
}

AbstractParameter::VisibilityPropagation AbstractParameter::visibilityPropagation() const
{
  return _visibilityPropagation;
}

QStringList AbstractParameter::parseText(const QString & type, const char * text, int & length)
{
  QStringList result;
  const QString str = text;
  result << str.left(str.indexOf("=")).trimmed();
#ifdef _GMIC_QT_DEBUG_
  _debugName = result.back();
#endif

  QRegExp re(QString("^[^=]*\\s*=\\s*(_?)%1\\s*(.)").arg(type), Qt::CaseInsensitive);
  re.indexIn(str);
  const int prefixLength = re.cap(0).toUtf8().size();

  if (re.cap(1) == "_") {
    _update = false;
  }

  QString open = re.cap(2);
  const char * end = nullptr;
  const char * closing = (open == "(") ? ")" : (open == "{") ? "}" : (open == "[") ? "]" : nullptr;
  if (!closing) {
    Logger::error(QString("Parse error in %1 parameter (invalid opening character '%2').").arg(type).arg(open));
    length = 1 + prefixLength;
    return QStringList();
  }
  end = strstr(text + prefixLength, closing);
  if (!end) {
    Logger::error(QString("Parse error in %1 parameter (cannot find closing '%2').").arg(type).arg(closing));
    length = 1 + prefixLength;
    return QStringList();
  }

  // QString values = str.mid(prefixLength, -1).left(end - (text + prefixLength)).trimmed();
  QString values = QString::fromUtf8(text + prefixLength, end - (text + prefixLength)).trimmed();
  length = 1 + end - text;

  if (text[length] == '_' && text[length + 1] >= '0' && text[length + 1] <= '2') {
    _defaultVisibilityState = static_cast<VisibilityState>(text[length + 1] - '0');
    _visibilityPropagation = PropagateNone;
    switch (text[length + 2]) {
    case '-':
      _visibilityPropagation = PropagateUp;
      length += 3;
      break;
    case '+':
      _visibilityPropagation = PropagateDown;
      length += 3;
      break;
    case '*':
      _visibilityPropagation = PropagateUpDown;
      length += 3;
      break;
    default:
      length += 2;
      break;
    }
    if (NoValueParameters.contains(type)) {
      Logger::warning(QString("Warning: %1 parameter should not define visibility. Ignored.").arg(result.first()));
      _defaultVisibilityState = AbstractParameter::VisibleParameter;
      _visibilityPropagation = PropagateNone;
    }
  }
  while (text[length] && (text[length] == ',' || QChar(text[length]).isSpace())) {
    ++length;
  }
  result << values;
  return result;
}

bool AbstractParameter::matchType(const QString & type, const char * text) const
{
  return QString(text).contains(QRegExp(QString("^[^=]*\\s*=\\s*_?%1\\s*.").arg(type), Qt::CaseInsensitive));
}

void AbstractParameter::notifyIfRelevant()
{
  if (_update) {
    emit valueChanged();
  }
}

} // namespace GmicQt

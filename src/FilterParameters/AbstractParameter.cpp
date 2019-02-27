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

AbstractParameter::AbstractParameter(QObject * parent, bool actualParameter) : QObject(parent), _actualParameter(actualParameter)
{
  _update = true;
  _defaultVisibilityState = VisibleParameter;
}

AbstractParameter::~AbstractParameter() {}

bool AbstractParameter::isVisible() const
{
  return true;
}

bool AbstractParameter::isActualParameter() const
{
  return _actualParameter;
}

QString AbstractParameter::unquotedTextValue() const
{
  return textValue();
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

AbstractParameter * AbstractParameter::createFromText(const char * text, int & length, QString & error, QWidget * parent)
{
  AbstractParameter * result = 0;
  QString line = text;
  error.clear();

#define PREFIX "^[^=]*\\s*=\\s*_?"
  if (QRegExp(PREFIX "int", Qt::CaseInsensitive).indexIn(line) == 0) {
    result = new IntParameter(parent);
  } else if (QRegExp(PREFIX "float", Qt::CaseInsensitive).indexIn(line) == 0) {
    result = new FloatParameter(parent);
  } else if (QRegExp(PREFIX "bool", Qt::CaseInsensitive).indexIn(line) == 0) {
    result = new BoolParameter(parent);
  } else if (QRegExp(PREFIX "choice", Qt::CaseInsensitive).indexIn(line) == 0) {
    result = new ChoiceParameter(parent);
  } else if (QRegExp(PREFIX "color", Qt::CaseInsensitive).indexIn(line) == 0) {
    result = new ColorParameter(parent);
  } else if (QRegExp(PREFIX "separator", Qt::CaseInsensitive).indexIn(line) == 0) {
    result = new SeparatorParameter(parent);
  } else if (QRegExp(PREFIX "note", Qt::CaseInsensitive).indexIn(line) == 0) {
    result = new NoteParameter(parent);
  } else if (QRegExp(PREFIX "file", Qt::CaseInsensitive).indexIn(line) == 0 || QRegExp(PREFIX "filein", Qt::CaseInsensitive).indexIn(line) == 0 ||
             QRegExp(PREFIX "fileout", Qt::CaseInsensitive).indexIn(line) == 0) {
    result = new FileParameter(parent);
  } else if (QRegExp(PREFIX "folder", Qt::CaseInsensitive).indexIn(line) == 0) {
    result = new FolderParameter(parent);
  } else if (QRegExp(PREFIX "text", Qt::CaseInsensitive).indexIn(line) == 0) {
    result = new TextParameter(parent);
  } else if (QRegExp(PREFIX "link", Qt::CaseInsensitive).indexIn(line) == 0) {
    result = new LinkParameter(parent);
  } else if (QRegExp(PREFIX "value", Qt::CaseInsensitive).indexIn(line) == 0) {
    result = new ConstParameter(parent);
  } else if (QRegExp(PREFIX "button", Qt::CaseInsensitive).indexIn(line) == 0) {
    result = new ButtonParameter(parent);
  } else if (QRegExp(PREFIX "point", Qt::CaseInsensitive).indexIn(line) == 0) {
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

QStringList AbstractParameter::parseText(const QString & type, const char * text, int & length)
{
  QStringList result;
  const QString str = text;
  result << str.left(str.indexOf("=")).trimmed();

  QRegExp re(QString("^[^=]*\\s*=\\s*(_?)%1\\s*(.)").arg(type), Qt::CaseInsensitive);
  re.indexIn(str);
  int prefixLength = re.matchedLength();

  if (re.cap(1) == "_") {
    _update = false;
  }

  QString open = re.cap(2);
  const char * end = 0;
  if (open == "(") {
    end = strstr(text + prefixLength, ")");
  } else if (open == "{") {
    end = strstr(text + prefixLength, "}");
  } else if (open == "[") {
    end = strstr(text + prefixLength, "]");
  }
  QString values = str.mid(prefixLength, -1).left(end - (text + prefixLength)).trimmed();
  length = 1 + end - text;

  if (text[length] == '_' && text[length + 1] >= '0' && text[length + 1] <= '2') {
    _defaultVisibilityState = static_cast<VisibilityState>(text[length + 1] - '0');
    length += 2;
  }

  while (text[length] && (text[length] == ',' || str[length].isSpace())) {
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

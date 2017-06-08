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
#include <cstring>
#include "Common.h"
#include <QDebug>
#include "AbstractParameter.h"
#include "IntParameter.h"
#include "FloatParameter.h"
#include "BoolParameter.h"
#include "ChoiceParameter.h"
#include "ColorParameter.h"
#include "SeparatorParameter.h"
#include "NoteParameter.h"
#include "FileParameter.h"
#include "FolderParameter.h"
#include "TextParameter.h"
#include "LinkParameter.h"
#include "ConstParameter.h"
#include "ButtonParameter.h"

AbstractParameter::AbstractParameter(QObject *parent, bool actualParameter)
  : QObject(parent),
    _update(true),
    _actualParameter(actualParameter)
{
}

AbstractParameter::~AbstractParameter()
{
}

bool
AbstractParameter::isVisible() const
{
  return true;
}

bool AbstractParameter::isActualParameter() const
{
  return _actualParameter;
}

QString
AbstractParameter::unquotedTextValue() const
{
  return textValue();
}

void
AbstractParameter::clear()
{
  // Do nothing except for ButtonParameter::clear()
}

AbstractParameter *
AbstractParameter::createFromText(const char *text, int & length, QString & error, QObject * parent)
{
  AbstractParameter * result = 0;
  QString line = text;
  error.clear();

#define PREFIX "^[^=]*\\s*=\\s*_?"
  if ( QRegExp( PREFIX "[iI]nt").indexIn(line) == 0 ) {
    result = new IntParameter(parent);
  } else if ( QRegExp(PREFIX "[fF]loat").indexIn(line) == 0 ) {
    result = new FloatParameter(parent);
  } else if ( QRegExp(PREFIX "[bB]ool").indexIn(line) == 0 ) {
    result = new BoolParameter(parent);
  } else if ( QRegExp(PREFIX "[cC]hoice").indexIn(line) == 0 ) {
    result = new ChoiceParameter(parent);
  } else if ( QRegExp(PREFIX "[cC]olor").indexIn(line) == 0 ) {
    result = new ColorParameter(parent);
  } else if ( QRegExp(PREFIX "[sS]eparator").indexIn(line) == 0 ) {
    result = new SeparatorParameter(parent);
  } else if ( QRegExp(PREFIX "[nN]ote").indexIn(line) == 0 ) {
    result = new NoteParameter(parent);
  } else if ( QRegExp(PREFIX "[fF]ile").indexIn(line) == 0
              || QRegExp(PREFIX "[fF]ilein").indexIn(line) == 0
              || QRegExp(PREFIX "[fF]ileout").indexIn(line) == 0 ) {
    result = new FileParameter(parent);
  } else if ( QRegExp(PREFIX "[fF]older").indexIn(line) == 0 ) {
    result = new FolderParameter(parent);
  } else if ( QRegExp(PREFIX "[tT]ext").indexIn(line) == 0 ) {
    result = new TextParameter(parent);
  } else if ( QRegExp(PREFIX "[lL]ink").indexIn(line) == 0 ) {
    result = new LinkParameter(parent);
  } else if ( QRegExp(PREFIX "[vV]alue").indexIn(line) == 0 ) {
    result = new ConstParameter(parent);
  } else if ( QRegExp(PREFIX "[bB]utton").indexIn(line) == 0 ) {
    result = new ButtonParameter(parent);
  }
  if ( result ) {
    result->initFromText(text,length);
  } else {
    if ( !line.isEmpty() ) {
      error = QString("AbstractParameter::createFromtext(): Parse error: %1").arg(line);
    }
  }
  return result;
}

QStringList
AbstractParameter::parseText(const QString & type, const char * text, int & length)
{
  QStringList result;
  QString str = text;
  result << str.left(str.indexOf("=")).trimmed();

  QRegExp re( QString("^[^=]*\\s*=\\s*(_?)%1\\s*(.)").arg(type) );
  re.indexIn(str);
  int prefixLength = re.matchedLength();

  if ( re.cap(1) == "_" ) {
    _update = false;
  }

  QString open = re.cap(2);
  const char * end = 0;
  if ( open == "(" ) {
    end = strstr(text + prefixLength,")");
  } else if ( open == "{") {
    end = strstr(text + prefixLength,"}");
  } else if ( open == "[") {
    end = strstr(text + prefixLength,"]");
  }
  QString values = str.mid(prefixLength,-1).left(end-(text+prefixLength)).trimmed();
  length = 1 + end - text;
  while ( text[length] && (text[length] == ',' || str[length].isSpace() ) ) {
    ++length;
  }
  result << values;
  return result;
}

bool AbstractParameter::matchType(const QString & typeRegExp, const char * text) const
{
  return QString(text).contains(QRegExp(QString("^[^=]*\\s*=\\s*_?%1\\s*.").arg(typeRegExp)));
}

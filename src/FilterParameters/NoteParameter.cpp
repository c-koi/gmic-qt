/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file NoteParameter.cpp
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
#include "FilterParameters/NoteParameter.h"
#include <QDebug>
#include <QDesktopServices>
#include <QGridLayout>
#include <QLabel>
#include <QUrl>
#include "Common.h"
#include "DialogSettings.h"
#include "HtmlTranslator.h"

NoteParameter::NoteParameter(QObject * parent) : AbstractParameter(parent, false), _label(0) {}

NoteParameter::~NoteParameter()
{
  delete _label;
}

void NoteParameter::addTo(QWidget * widget, int row)
{
  QGridLayout * grid = dynamic_cast<QGridLayout *>(widget->layout());
  if (!grid)
    return;
  delete _label;
  _label = new QLabel(_text, widget);
  _label->setTextFormat(Qt::RichText);
  _label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  _label->setWordWrap(true);
  connect(_label, SIGNAL(linkActivated(QString)), this, SLOT(onLinkActivated(QString)));
  grid->addWidget(_label, row, 0, 1, 3);
}

void NoteParameter::addToKeypointList(KeypointList &) const {}

void NoteParameter::extractPositionFromKeypointList(KeypointList &) {}

QString NoteParameter::textValue() const
{
  return QString::null;
}

void NoteParameter::setValue(const QString &) {}

void NoteParameter::reset() {}

bool NoteParameter::initFromText(const char * text, int & textLength)
{
  QList<QString> list = parseText("note", text, textLength);
  _text = list[1].trimmed().remove(QRegExp("^\"")).remove(QRegExp("\"$")).replace(QString("\\\""), "\"");
  _text.replace(QString("\\n"), "<br/>");

  if (DialogSettings::darkThemeEnabled()) {
    _text.replace(QRegExp("color\\s*=\\s*\"purple\""), QString("color=\"#ff00ff\""));
    _text.replace(QRegExp("foreground\\s*=\\s*\"purple\""), QString("foreground=\"#ff00ff\""));
    _text.replace(QRegExp("color\\s*=\\s*\"blue\""), QString("color=\"#9b9bff\""));
    _text.replace(QRegExp("foreground\\s*=\\s*\"blue\""), QString("foreground=\"#9b9bff\""));
  }

  _text.replace(QRegExp("color\\s*=\\s*\""), QString("style=\"color:"));
  _text.replace(QRegExp("foreground\\s*=\\s*\""), QString("style=\"color:"));
  _text = HtmlTranslator::fromUtf8Escapes(_text);
  return true;
}

void NoteParameter::onLinkActivated(const QString & link)
{
  QDesktopServices::openUrl(QUrl(link));
}

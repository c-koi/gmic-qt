/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file LinkParameter.h
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
#ifndef _GMIC_QT_LINKPARAMETER_H_
#define _GMIC_QT_LINKPARAMETER_H_

#include "AbstractParameter.h"
#include <Qt>
class QLabel;

class LinkParameter : public AbstractParameter {
  Q_OBJECT

public:
  LinkParameter(QObject * parent = 0);
  ~LinkParameter();
  void addTo(QWidget *, int row);
  QString textValue() const;
  void setValue(const QString & value);
  void reset();
  void initFromText(const char * text, int & textLength);

public slots:
  void onLinkActivated(const QString & link);

signals:
  void valueChanged();

private:
  QLabel * _label;
  QString _text;
  QString _url;
  Qt::AlignmentFlag _alignment;
};

#endif // _GMIC_QT_LINKPARAMETER_H_

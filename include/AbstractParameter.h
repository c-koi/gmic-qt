/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file AbstractParameter.h
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
#ifndef _GMIC_QT_ABSTRACTPARAMETER_H_
#define _GMIC_QT_ABSTRACTPARAMETER_H_

#include <QObject>

class AbstractParameter : public QObject {
  Q_OBJECT

public:
  AbstractParameter(QObject * parent, bool actualParameter);
  virtual ~AbstractParameter();
  virtual bool isVisible() const;
  bool isActualParameter() const;
  virtual void addTo(QWidget *, int row) = 0;
  virtual QString textValue() const = 0;
  virtual QString unquotedTextValue() const;
  virtual void clear();
  virtual void setValue(const QString & value) = 0;
  virtual void reset() = 0;
  static AbstractParameter * createFromText(const char * text, int & length, QString & error, QObject * parent = 0);
  virtual void initFromText(const char * text, int & textLength) = 0;

signals:
  void valueChanged();

protected:
  QStringList parseText(const QString & type, const char * text, int & length);
  bool _update;
  const bool _actualParameter;
};

#endif // _GMIC_QT_ABSTRACTPARAMETER_H_

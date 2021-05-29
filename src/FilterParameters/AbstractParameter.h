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
#ifndef GMIC_QT_ABSTRACTPARAMETER_H
#define GMIC_QT_ABSTRACTPARAMETER_H

#include <QObject>
#include <QStringList>
class QGridLayout;

namespace GmicQt
{
class KeypointList;

class AbstractParameter : public QObject {
  Q_OBJECT

public:
  AbstractParameter(QObject * parent);
  virtual ~AbstractParameter() override;
  bool isActualParameter() const;
  virtual int size() const = 0;
  virtual bool addTo(QWidget *, int row) = 0;
  virtual QString value() const = 0;
  virtual QString defaultValue() const = 0;
  virtual bool isQuoted() const;
  virtual void setValue(const QString & value) = 0;
  virtual void clear();
  virtual void reset() = 0;

  virtual void addToKeypointList(KeypointList &) const;
  virtual void extractPositionFromKeypointList(KeypointList &);

  static AbstractParameter * createFromText(const char * text, int & length, QString & error, QObject * parent = nullptr);
  virtual bool initFromText(const char * text, int & textLength) = 0;

  enum class VisibilityState
  {
    Unspecified = -1,
    Hidden = 0,
    Disabled = 1,
    Visible = 2,
  };
  enum class VisibilityPropagation
  {
    NoPropagation = 0,
    Up = 1,
    Down = 2,
    UpDown = 3
  };

  static const QStringList NoValueParameters;

  virtual VisibilityState defaultVisibilityState() const;
  virtual void setVisibilityState(VisibilityState state);
  VisibilityState visibilityState() const;
  VisibilityPropagation visibilityPropagation() const;

signals:
  void valueChanged();

protected:
  QStringList parseText(const QString & type, const char * text, int & length);
  bool matchType(const QString & type, const char * text) const;
  void notifyIfRelevant();
  VisibilityState _defaultVisibilityState;
  QGridLayout * _grid;
  int _row;
#ifdef _GMIC_QT_DEBUG_
  QString _debugName;
#endif

private:
  bool _update;
  VisibilityState _visibilityState;
  VisibilityPropagation _visibilityPropagation;
};

} // namespace GmicQt

#endif // GMIC_QT_ABSTRACTPARAMETER_H

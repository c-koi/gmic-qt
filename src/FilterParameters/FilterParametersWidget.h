/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FilterParametersWidget.h
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
#ifndef GMIC_QT_FILTERPARAMSWIDGET_H
#define GMIC_QT_FILTERPARAMSWIDGET_H

#include <QGroupBox>
#include <QList>
#include <QModelIndex>
#include <QPushButton>
#include <QStringList>
#include <QVector>
#include <QWidget>
#include "KeypointList.h"
class QLabel;

namespace GmicQt
{
class AbstractParameter;

class FilterParametersWidget : public QWidget {
  Q_OBJECT

public:
  FilterParametersWidget(QWidget * parent);
  bool build(const QString & name, const QString & hash, const QString & parameters, const QList<QString> & values, const QList<int> & visibilityStates);
  void setNoFilter(const QString & message = QString());
  ~FilterParametersWidget() override;
  const QString & valueString() const;
  QStringList valueStringList() const;
  void setValues(const QStringList &, bool notify);
  void setVisibilityStates(QList<int> states);
  QList<int> visibilityStates();
  QList<int> defaultVisibilityStates();

  void applyDefaultVisibilityStates();
  void reset(bool notify);
  void randomize();
  QString filterName() const;
  int actualParametersCount() const;
  int acceptRandom() const;
  QString filterHash() const;
  void clearButtonParameters();
  KeypointList keypoints() const;
  void setKeypoints(KeypointList list, bool notify);
  bool hasKeypoints() const;
  const QVector<bool> & quotedParameters() const;

  static QString defaultValueString(const QVector<AbstractParameter *> & parameters);
  static QStringList defaultParameterList(const QString & parameters,       //
                                          QString * error,                  //
                                          QVector<bool> * quoted = nullptr, //
                                          QVector<int> * size = nullptr);

public slots:
  void updateValueString(bool notify);
  void updateValueStringAndNotify();

signals:
  void valueChanged();

private:
  static QString valueString(const QVector<AbstractParameter *> & parameters);
  static QVector<AbstractParameter *> buildParameters(const QString & filterName, //
                                                      const QString & parameters, //
                                                      QObject * parent,           //
                                                      int * actualParameterCount, //
                                                      bool * acceptRandom,        //
                                                      QString * error);
  static QStringList defaultParameterList(const QVector<AbstractParameter *> & parameters, QVector<bool> * quoted);
  static QVector<bool> quotedParameters(const QVector<AbstractParameter *> & parameters);
  static QVector<int> parameterSizes(const QVector<AbstractParameter *> & parameters);

protected:
  void clear();
  QVector<AbstractParameter *> _parameters;
  int _actualParametersCount;
  bool _acceptRandom;
  QString _valueString;
  QLabel * _labelNoParams;
  QWidget * _paddingWidget;
  QString _filterName;
  QString _filterHash;
  bool _hasKeypoints;
  QVector<bool> _quotedParameters;
};

} // namespace GmicQt

#endif // GMIC_QT_FILTERPARAMSWIDGET_H

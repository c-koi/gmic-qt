/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file KeypointList.h
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
#ifndef GMIC_QT_KEYPOINTLIST_H
#define GMIC_QT_KEYPOINTLIST_H

#include <QColor>
#include <QDebug>
#include <QPointF>
#include <QSize>
#include <cmath>
#include <deque>
#include "Common.h"

namespace GmicQt
{

class KeypointList {
public:
  struct Keypoint {
    float x;
    float y;
    QColor color; /* A negative alpha means "Keep opacity while moved" */
    bool removable;
    bool burst;
    float radius; /* A negative value is a percentage of the preview diagonal */
    bool keepOpacityWhenSelected;

    Keypoint(float x, float y, QColor color, bool removable, bool burst, float radius, bool keepOpacityWhenSelected);
    Keypoint(QPointF point, QColor color, bool removable, bool burst, float radius, bool keepOpacityWhenSelected);
    Keypoint(QColor color, bool removable, bool burst, float radius, bool keepOpacityWhenSelected);
    bool isNaN() const;
    Keypoint & setNaN();
    inline void setPosition(float x, float y);
    inline void setPosition(const QPointF & p);
    static const float DefaultRadius;

    inline int actualRadiusFromPreviewSize(const QSize & size) const;
  };

  KeypointList();
  void add(const Keypoint & keypoint);
  bool isEmpty();
  void clear();
  QPointF position(int n) const;
  QColor color(int n) const;
  bool isRemovable(int n) const;

  typedef std::deque<Keypoint>::iterator iterator;
  typedef std::deque<Keypoint>::const_iterator const_iterator;
  typedef std::deque<Keypoint>::reverse_iterator reverse_iterator;
  typedef std::deque<Keypoint>::const_reverse_iterator const_reverse_iterator;
  typedef std::deque<Keypoint>::size_type size_type;
  typedef std::deque<Keypoint>::reference reference;
  typedef std::deque<Keypoint>::const_reference const_reference;

  size_type size() const { return _keypoints.size(); }
  reference operator[](size_type index) { return _keypoints[index]; }
  const_reference operator[](size_type index) const { return _keypoints[index]; }
  void pop_front() { _keypoints.pop_front(); }
  const Keypoint & front() { return _keypoints.front(); }
  const Keypoint & front() const { return _keypoints.front(); }
  iterator begin() { return _keypoints.begin(); }
  iterator end() { return _keypoints.end(); }
  const_iterator cbegin() const { return _keypoints.cbegin(); }
  const_iterator cend() const { return _keypoints.cend(); }
  reverse_iterator rbegin() { return _keypoints.rbegin(); }
  reverse_iterator rend() { return _keypoints.rend(); }
  const_reverse_iterator rbegin() const { return _keypoints.crbegin(); }
  const_reverse_iterator rend() const { return _keypoints.crend(); }

private:
  std::deque<Keypoint> _keypoints;
};

void KeypointList::Keypoint::setPosition(float x, float y)
{
  KeypointList::Keypoint::x = x;
  KeypointList::Keypoint::y = y;
}

void KeypointList::Keypoint::setPosition(const QPointF & point)
{
  KeypointList::Keypoint::x = (float)point.x();
  KeypointList::Keypoint::y = (float)point.y();
}

int KeypointList::Keypoint::actualRadiusFromPreviewSize(const QSize & size) const
{
  if (radius >= 0) {
    return static_cast<int>(radius);
  } else {
    return std::max(2, static_cast<int>(std::round(-static_cast<double>(radius) * (std::sqrt(size.width() * size.width() + size.height() * size.height())) / 100.0)));
  }
}

} // namespace GmicQt

#endif // GMIC_QT_KEYPOINTLIST_H

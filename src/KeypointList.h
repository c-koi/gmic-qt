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
#ifndef _GMIC_QT_KEYPOINTLIST_H_
#define _GMIC_QT_KEYPOINTLIST_H_

#include <QColor>
#include <QPointF>
#include <deque>

class KeypointList {
public:
  struct Keypoint {
    float x;
    float y;
    QColor color;
    bool removable;
    bool burst;
    int radius;

    Keypoint(float x, float y, QColor color, bool removable, bool burst, int radius);
    Keypoint(QPointF point, QColor color, bool removable, bool burst, int radius);
    Keypoint(QColor color, bool removable, bool burst, int radius);
    bool isNaN() const;
    Keypoint & setNaN();
    inline void setPosition(float x, float y);
    inline void setPosition(const QPointF & p);
    static const int DefaultRadius = 6;
  };

  KeypointList();
  void add(const Keypoint & keypoint);
  bool isEmpty();
  void clear();
  QPointF position(int n) const;
  QColor color(int n) const;
  bool isRemovable(int n) const;
  bool allowsBusrt(int n) const;

  typedef std::deque<Keypoint>::iterator iterator;
  typedef std::deque<Keypoint>::const_iterator const_iterator;
  typedef std::deque<Keypoint>::reverse_iterator reverse_iterator;
  typedef std::deque<Keypoint>::const_reverse_iterator const_reverse_iterator;
  typedef std::deque<Keypoint>::size_type size_type;

  size_type size() const { return _keypoints.size(); }
  Keypoint & operator[](size_type index) { return _keypoints[index]; }
  const Keypoint & operator[](size_type index) const { return _keypoints[index]; }
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

#endif // _GMIC_QT_KEYPOINTLIST_H_

/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file ParametersCache.cpp
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
#include <QFile>
#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include "Common.h"
#include "ParametersCache.h"
#include "gmic.h"

QHash<QString,QList<QString>> ParametersCache::_cache;

void ParametersCache::load()
{
  QString path = QString("%1%2").arg( GmicQt::path_rc(false), PARAMETERS_CACHE_FILENAME );
  QFile file(path);
  if ( file.open(QFile::ReadOnly) ) {
    QString line;
    do {
      line = file.readLine();
    } while ( line != QString("[Filter parameters (compressed)]\n") );
    QByteArray data = qUncompress(file.readAll());
    QBuffer buffer(&data);
    buffer.open(QIODevice::ReadOnly);

    QDataStream stream(&buffer);
    stream.setVersion(QDataStream::Qt_5_0);
    qint32 count;
    stream >> count;
    QString hash;
    QList<QString> values;
    while ( count-- ) {
      stream >> hash >> values;
      setValue(hash,values);
    }
  }
}

void
ParametersCache::save()
{
  QByteArray data;
  QBuffer buffer(&data);
  buffer.open(QIODevice::WriteOnly);
  QDataStream stream(&buffer);
  stream.setVersion(QDataStream::Qt_5_0);
  qint32 count = _cache.size();
  stream << count;
  QHash<QString,QList<QString>>::const_iterator it = _cache.begin();
  while ( it != _cache.end() ) {
    stream << it.key() << it.value();
    ++it;
  }

  QString path = QString("%1%2").arg( GmicQt::path_rc(true), PARAMETERS_CACHE_FILENAME );
  QFile file(path);
  if ( file.open(QFile::WriteOnly) ) {
    file.write(QString("Version=0.0.1\n").toLocal8Bit());
    file.write(QString("[Filter parameters (compressed)]\n").toLocal8Bit());
    file.write(qCompress(data));
    file.close();
  } else {
    qWarning() << "[gmic-qt] Error: Cannot write" << path;
  }
}

void
ParametersCache::setValue(const QString & hash, const QList<QString> & list)
{
  _cache[hash] = list;
}

QList<QString>
ParametersCache::getValue(const QString & hash)
{
  if ( _cache.count(hash) ) {
    return _cache[hash];
  } else {
    return QList<QString>();
  }
}

void
ParametersCache::remove(const QString & hash)
{
  _cache.remove(hash);
}

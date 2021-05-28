/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file ImageDialog.h
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
#ifndef GMIC_QT_IMAGE_DIALOG_H
#define GMIC_QT_IMAGE_DIALOG_H
#include <QApplication>
#include <QDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImage>
#include <QPainter>
#include <QPushButton>
#include <QString>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QVector>
#include "Common.h"
#include "Globals.h"
#include "ImageConverter.h"
#include "gmic_qt.h"

namespace cimg_library
{
template <typename T> struct CImg;
}

namespace gmic_qt_standalone
{

class ImageView : public QWidget {
public:
  ImageView(QWidget * parent);
  void setImage(const cimg_library::CImg<gmic_pixel_type> & image);
  void setImage(const QImage & image);
  bool save(const QString & filename, int quality);
  void paintEvent(QPaintEvent *) override;
  const QImage & image() const;

private:
  QImage _image;
};

class ImageDialog : public QDialog {
  Q_OBJECT
public:
  ImageDialog(QWidget * parent);
  void addImage(const cimg_library::CImg<gmic_pixel_type> & image, const QString & name);
  const QImage & currentImage() const;
  int currentImageIndex() const;
  static void supportedImageFormats(QStringList & extensions, QString & filters);
  void setJPEGQuality(int);
public slots:
  void onSaveAs();
  void onCloseClicked(bool);

private:
  QPushButton * _closeButton;
  QPushButton * _saveButton;
  QTabWidget * _tabWidget;
  QVector<bool> _savedTab;
  int _jpegQuality;
  static const int UNSPECIFIED_JPEG_QUALITY = -1;
};

} // namespace gmic_qt_standalone

#endif

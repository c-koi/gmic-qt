/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file ImageDialog.cpp
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
#include "Host/None/ImageDialog.h"
#include "gmic.h"

ImageView::ImageView(QWidget * parent):QWidget(parent)
{
}

void ImageView::setImage(const cimg_library::CImg<gmic_pixel_type> & image)
{
  ImageConverter::convert(image,_image);
  setMinimumSize(std::min(640,image.width()),
                 std::min(480,image.height()));
}

void ImageView::setImage(const QImage & image) {
  _image = image;
  setMinimumSize(std::min(640,image.width()),
                 std::min(480,image.height()));
}

void ImageView::save(const QString &filename)
{
  _image.save(filename);
}

ImageDialog::ImageDialog(QWidget *parent)
  : QDialog(parent)
{
  QVBoxLayout * vbox = new QVBoxLayout(this);

  _tabWidget = new QTabWidget(this);
  vbox->addWidget(_tabWidget);

  _tabWidget->setElideMode(Qt::ElideRight);

  QHBoxLayout * hbox = new QHBoxLayout;
  vbox->addLayout(hbox);
  _closeButton = new QPushButton("Close");
  connect(_closeButton,SIGNAL(clicked(bool)),
          this,SLOT(onCloseClicked(bool)));
  hbox->addWidget(_closeButton);
  _saveButton = new QPushButton("Save as...");
  connect(_saveButton,SIGNAL(clicked(bool)),
          this,SLOT(onSaveAs()));
  hbox->addWidget(_saveButton);
}

void ImageDialog::addImage(const cimg_library::CImg<float> & image, QString name)
{
  ImageView * view = new ImageView(_tabWidget);
  view->setImage(image);
  _tabWidget->addTab(view,name);
  _tabWidget->setCurrentIndex(_tabWidget->count()-1);
}

void ImageDialog::onSaveAs()
{
  QString filename = QFileDialog::getSaveFileName(this,"Save image as...",QString(),"PNG file (*.png);;JPEG file (*.jpg)");
  if ( ! filename.isEmpty() ) {
    ImageView * view = dynamic_cast<ImageView*>(_tabWidget->currentWidget());
    if ( view ) {
      view->save(filename);
    }
  }

}

void ImageDialog::onCloseClicked(bool)
{
  done(0);
}

void ImageView::paintEvent(QPaintEvent *)
{
  QPainter p(this);
  QImage displayed;
  if ( _image.width() / _image.height() > width() / height() ) {
    displayed = _image.scaledToWidth(width());
    p.drawImage(0,(height()-displayed.height())/2,displayed);
  } else {
    displayed = _image.scaledToHeight(height());
    p.drawImage((width()-displayed.width())/2,0,displayed);
  }
}

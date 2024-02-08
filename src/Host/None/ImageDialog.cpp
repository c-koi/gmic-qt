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
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageWriter>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include "Common.h"
#include "JpegQualityDialog.h"
#include "Settings.h"
#include "gmic.h"

namespace gmic_qt_standalone
{

ImageView::ImageView(QWidget * parent) : QWidget(parent) {}

void ImageView::setImage(const gmic_library::gmic_image<gmic_pixel_type> & image)
{
  GmicQt::convertGmicImageToQImage(image, _image);
  setMinimumSize(std::min(640, image.width()), std::min(480, image.height()));
}

void ImageView::setImage(const QImage & image)
{
  _image = image;
  setMinimumSize(std::min(640, image.width()), std::min(480, image.height()));
}

bool ImageView::save(const QString & filename, int quality)
{
  QString ext = QFileInfo(filename).suffix().toLower();
  if ((ext == "jpg" || ext == "jpeg") && (quality == ImageDialog::UNSPECIFIED_JPEG_QUALITY)) {
    quality = JpegQualityDialog::ask(dynamic_cast<QWidget *>(parent()), -1);
    if (quality == ImageDialog::UNSPECIFIED_JPEG_QUALITY) {
      return false;
    }
  }
  if (!_image.save(filename, nullptr, quality)) {
    QMessageBox::critical(this, tr("Error"), tr("Could not write image file %1").arg(filename));
    return false;
  }
  return true;
}

ImageDialog::ImageDialog(QWidget * parent) : QDialog(parent)
{
  setWindowTitle(tr("G'MIC-Qt filter output"));
  _jpegQuality = UNSPECIFIED_JPEG_QUALITY;
  auto vbox = new QVBoxLayout(this);

  _tabWidget = new QTabWidget(this);
  vbox->addWidget(_tabWidget);

  _tabWidget->setElideMode(Qt::ElideRight);

  auto hbox = new QHBoxLayout;
  vbox->addLayout(hbox);
  _closeButton = new QPushButton(tr("Close"));
  connect(_closeButton, &QPushButton::clicked, this, &ImageDialog::onCloseClicked);
  hbox->addWidget(_closeButton);
  _saveButton = new QPushButton(tr("Save as..."));
  connect(_saveButton, &QPushButton::clicked, this, &ImageDialog::onSaveAs);
  hbox->addWidget(_saveButton);
}

void ImageDialog::addImage(const gmic_library::gmic_image<float> & image, const QString & name)
{
  auto view = new ImageView(_tabWidget);
  view->setImage(image);
  _tabWidget->addTab(view, name + "*");
  _tabWidget->setCurrentIndex(_tabWidget->count() - 1);
  _savedTab.push_back(false);
}

const QImage & ImageDialog::currentImage() const
{
  QWidget * widget = _tabWidget->currentWidget();
  auto view = dynamic_cast<ImageView *>(widget);
  Q_ASSERT_X(view, __FUNCTION__, "Widget is not an ImageView");
  return view->image();
}

int ImageDialog::currentImageIndex() const
{
  return _tabWidget->currentIndex();
}

void ImageDialog::supportedImageFormats(QStringList & extensions, QStringList & filters)
{
  extensions.clear();
  for (const auto & ext : QImageWriter::supportedImageFormats()) {
    extensions.push_back(QString::fromLatin1(ext).toLower());
  }
  filters.clear();
  for (const auto & extension : extensions) {
    QString filter = QString(tr("%1 file (*.%2 *.%3)")).arg(extension.toUpper()).arg(extension.toUpper()).arg(extension);
    if (extension == "png" || extension == "jpg" || extension == "jpeg") {
      filters.push_front(filter);
    } else {
      filters.push_back(filter);
    }
  }
}

void ImageDialog::setJPEGQuality(int q)
{
  _jpegQuality = q;
}

void ImageDialog::onSaveAs()
{
  QSettings settings;
  QString selectedFilter;
  selectedFilter = settings.value("Standalone/SaveAsSelectedFilter", QString()).toString();
  QStringList extensions;
  QStringList filters;
  supportedImageFormats(extensions, filters);
  if (!filters.contains(selectedFilter)) {
    selectedFilter.clear();
  }
  const QFileDialog::Options options = GmicQt::Settings::nativeFileDialogs() ? QFileDialog::Options() : QFileDialog::DontUseNativeDialog;
  QString filename = QFileDialog::getSaveFileName(this, tr("Save image as..."), QString(), filters.join(";;"), &selectedFilter, options);
  settings.setValue("Standalone/SaveAsSelectedFilter", selectedFilter);
  QString extension = selectedFilter.split("*").back();
  extension.chop(1);
  if (!extensions.contains(QFileInfo(filename).suffix())) {
    filename += extension;
  }
  if (!filename.isEmpty()) {
    auto view = dynamic_cast<ImageView *>(_tabWidget->currentWidget());
    int index = _tabWidget->currentIndex();
    if (view) {
      if (view->save(filename, _jpegQuality)) {
        _tabWidget->setTabText(index, QFileInfo(filename).fileName());
        _tabWidget->setTabToolTip(index, QFileInfo(filename).filePath());
      }
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
  if ((static_cast<float>(_image.width()) / static_cast<float>(_image.height())) > (static_cast<float>(width()) / static_cast<float>(height()))) {
    displayed = _image.scaledToWidth(width());
    p.drawImage(0, (height() - displayed.height()) / 2, displayed);
  } else {
    displayed = _image.scaledToHeight(height());
    p.drawImage((width() - displayed.width()) / 2, 0, displayed);
  }
}

const QImage & ImageView::image() const
{
  return _image;
}

} // namespace gmic_qt_standalone

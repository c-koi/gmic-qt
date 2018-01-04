/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file host_none.cpp
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
#include <QApplication>
#include <QProcess>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QDesktopWidget>
#include <QRegularExpression>
#include <algorithm>
#include <iostream>
#include "Host/host.h"
#include "gmic_qt.h"
#include "Common.h"
#include "ImageConverter.h"
#include "Host/None/ImageDialog.h"
#include "gmic.h"

#ifdef _GMIC_QT_DEBUG_
//#define DEFAULT_IMAGE "local/img_3820.png"
//#define DEFAULT_IMAGE "local/Untitled.png"
//#define DEFAULT_IMAGE "local/space-shuttle.png"
//#define DEFAULT_IMAGE "local/space-shuttle-transp.png"
//#define DEFAULT_IMAGE "local/bug.jpg"
//#define DEFAULT_IMAGE "local/crop_inktober.jpg"
#define DEFAULT_IMAGE "local/lena.png"
//#define DEFAULT_IMAGE "local/small_lena.png"
//#define DEFAULT_IMAGE "local/ken.jpg"
//#define DEFAULT_IMAGE "local/ferrari.jpg"
//#define DEFAULT_IMAGE "local/audio-speakers.png"
//#define DEFAULT_IMAGE "local/audio-speakers-top.png"
#endif

namespace gmic_qt_standalone {
QImage input_image;
QString image_filename;
}

namespace GmicQt {
   const QString HostApplicationName;
   const char * HostApplicationShortname = GMIC_QT_XSTRINGIFY(GMIC_HOST);
}

void gmic_qt_get_image_size(int * x, int * y)
{
  *x = gmic_qt_standalone::input_image.width();
  *y = gmic_qt_standalone::input_image.height();
}

void gmic_qt_get_layers_extent(int * width, int * height, GmicQt::InputMode )
{
  gmic_qt_get_image_size(width,height);
}

void gmic_qt_get_cropped_images(gmic_list<float> & images,
                                gmic_list<char> & imageNames,
                                double x, double y, double width, double height,
                                GmicQt::InputMode mode)
{
  QImage & input_image = gmic_qt_standalone::input_image;
  const bool entireImage = x < 0 && y < 0 && width < 0 && height < 0;
  if (entireImage) {
    x = 0.0;
    y = 0.0;
    width = 1.0;
    height = 1.0;
  }
  if ( mode == GmicQt::NoInput ) {
    images.assign();
    imageNames.assign();
    return;
  } else {
    images.assign(1);
    imageNames.assign(1);

    QString name = QString("pos(0,0),name(%1)")
        .arg(gmic_qt_standalone::image_filename);
    QByteArray ba = name.toUtf8();
    gmic_image<char>::string(ba.constData()).move_to(imageNames[0]);

    const int ix = static_cast<int>(entireImage?0:std::floor(x * input_image.width()));
    const int iy = static_cast<int>(entireImage?0:std::floor(y * input_image.height()));
    const int iw = entireImage?input_image.width():std::min(input_image.width()-ix,static_cast<int>(1+std::ceil(width * input_image.width())));
    const int ih = entireImage?input_image.height():std::min(input_image.height()-iy,static_cast<int>(1+std::ceil(height * input_image.height())));
    ImageConverter::convert(input_image.copy(ix,iy,iw,ih),images[0]);
  }
}

void gmic_qt_output_images( gmic_list<float> & images,
                            const gmic_list<char> & imageNames,
                            GmicQt::OutputMode mode,
                            const char * verboseLayersLabel )
{
  if ( images.size() > 0 ) {
    ImageDialog * dialog = new ImageDialog(QApplication::topLevelWidgets().at(0));
    for (unsigned int i = 0; i < images.size(); ++i) {
      QString name((const char*)imageNames[i]);
      name.replace(QRegularExpression(".*name\\("),"");
      int pos = 0;
      int open = 1;
      int len = name.size();
      while ( pos < len && open > 0 ) {
        if ( name[pos] == '(' ) { ++open; }
        else if ( name[pos] == ')' ) { --open; }
        ++pos;
      }
      name.remove(pos?(pos-1):pos,len);
      dialog->addImage(images[i],name);
    }
    dialog->exec();
    delete dialog;
  }
  unused(mode);
  unused(verboseLayersLabel);
}

void gmic_qt_show_message(const char * message)
{
  std::cout << message << std::endl;
}

int main(int argc, char * argv[])
{
  QString filename;
  if ( argc == 2 ) {
    filename = argv[1];
  }
#ifdef DEFAULT_IMAGE
  if ( filename.isEmpty() && QFileInfo(DEFAULT_IMAGE).isReadable() ) {
    filename = DEFAULT_IMAGE;
  }
#endif
  if ( !filename.isEmpty() ) {
    if ( QFileInfo(filename).isReadable() && gmic_qt_standalone::input_image.load(filename) ) {
      gmic_qt_standalone::input_image = gmic_qt_standalone::input_image.convertToFormat(QImage::Format_ARGB32);
      gmic_qt_standalone::image_filename = QFileInfo(filename).fileName();
      return launchPlugin();
    } else {
      std::cerr << "Could not open file " << filename.toLocal8Bit().constData() << "\n";
      return 1;
    }
  } else {
    QApplication app(argc,argv);
    QWidget mainWidget;
    mainWidget.setWindowTitle(QString("G'MIC-Qt - %1.%2.%3").arg(gmic_version/100).arg((gmic_version/10)%10).arg(gmic_version%10));
    QRect position = mainWidget.frameGeometry();
    position.moveCenter(QDesktopWidget().availableGeometry().center());
    mainWidget.move(position.topLeft());
    mainWidget.show();
    QString filename = QFileDialog::getOpenFileName(&mainWidget,
                                                    QObject::tr("Select an image to open..."),
                                                    ".",
                                                    QObject::tr("PNG & JPG files (*.png *.jpeg *.jpg *.PNG *.JPEG *.JPG)"),
                                                    nullptr);
    mainWidget.hide();
    if ( ! filename.isEmpty() && QFileInfo(filename).isReadable() ) {
      return QProcess::execute(app.applicationFilePath(),QStringList()<<filename);
    } else {
      return 1;
    }
  }
}

void gmic_qt_apply_color_profile(cimg_library::CImg<gmic_pixel_type> & )
{

}

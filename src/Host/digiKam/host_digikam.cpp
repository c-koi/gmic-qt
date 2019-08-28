/*
*  This file is part of G'MIC-Qt, a generic plug-in for raster graphics
*  editors, offering hundreds of filters thanks to the underlying G'MIC
*  image processing framework.
*
*  Copyright (C) 2019 Gilles Caulier
*
*  G'MIC-Qt is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  G'MIC-Qt is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include <QImage>
#include <QString>
#include <QVector>
#include <QTextStream>
#include <QDebug>
#include <iostream>
#include "Common.h"
#include "Host/host.h"
#include "ImageConverter.h"
#include "gmic_qt.h"
#include "gmic.h"

// digiKam includes

#include "imageiface.h"

using namespace Digikam;

namespace GmicQt
{
    const QString HostApplicationName     = QString("digiKam");
    const char * HostApplicationShortname = GMIC_QT_XSTRINGIFY(GMIC_HOST);
    const bool DarkThemeIsDefault         = false;
}

void gmic_qt_get_image_size(int* width,
                            int* height)
{
    qDebug() << "Calling gmic_qt_get_image_size()";

    ImageIface iface;
    QSize size = iface.originalSize();

    *width     = size.width();
    *height    = size.height();
}

void gmic_qt_get_layers_extent(int* width,
                               int* height,
                               GmicQt::InputMode mode)
{
    qDebug() << "Calling gmic_qt_get_layers_extent() : InputMode=" << mode;

    gmic_qt_get_image_size(width, height);

    qDebug() << "W=" << *width;
    qDebug() << "H=" << *height;
}

void gmic_qt_get_cropped_images(gmic_list<float>& images,
                                gmic_list<char>& imageNames,
                                double x,
                                double y,
                                double width,
                                double height,
                                GmicQt::InputMode mode)
{
    qDebug() << "Calling gmic_qt_get_cropped_images()";

    ImageIface iface;
    const QImage & input_image = iface.original()->copyQImage();

    const bool entireImage = (x < 0) && (y < 0) && (width < 0) && (height < 0);

    if (entireImage)
    {
        x      = 0.0;
        y      = 0.0;
        width  = 1.0;
        height = 1.0;
    }

    if (mode == GmicQt::NoInput)
    {
        images.assign();
        imageNames.assign();
        return;
    }

    images.assign(1);
    imageNames.assign(1);

    QString name  = QString("pos(0,0),name(%1)").arg("Image Editor Canvas");
    QByteArray ba = name.toUtf8();
    gmic_image<char>::string(ba.constData()).move_to(imageNames[0]);

    const int ix = static_cast<int>(entireImage ? 0 : std::floor(x * input_image.width()));
    const int iy = static_cast<int>(entireImage ? 0 : std::floor(y * input_image.height()));
    const int iw = entireImage ? input_image.width()  : std::min(input_image.width()  - ix, static_cast<int>(1 + std::ceil(width * input_image.width())));
    const int ih = entireImage ? input_image.height() : std::min(input_image.height() - iy, static_cast<int>(1 + std::ceil(height * input_image.height())));
    ImageConverter::convert(input_image.copy(ix, iy, iw, ih), images[0]);
}

void gmic_qt_output_images(gmic_list<float>& images,
                           const gmic_list<char>& imageNames,
                           GmicQt::OutputMode mode,
                           const char* verboseLayersLabel)
{
    qDebug() << "Calling gmic_qt_output_images()";

/*
    unused(imageNames);
    unused(mode);
    unused(verboseLayersLabel);

    if (images.size() > 0)
    {
        QImage outputImage;

        ImageConverter::convert(images[0], outputImage);

        outputImage.save(host_paintdotnet::outputImagePath);
    }
*/
}

void gmic_qt_apply_color_profile(cimg_library::CImg<gmic_pixel_type>& images)
{
    qDebug() << "Calling gmic_qt_apply_color_profile()";

    unused(images);
}

void gmic_qt_show_message(const char* message)
{
    qDebug() << "Calling gmic_qt_show_message()";
    qDebug() << "Message:" << message;
}
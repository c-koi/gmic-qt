/*
*  This file is part of G'MIC-Qt, a generic plug-in for raster graphics
*  editors, offering hundreds of filters thanks to the underlying G'MIC
*  image processing framework.
*
*  Copyright (C) 2018 Nicholas Hayes
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
#include <iostream>
#include "Common.h"
#include "Host/host.h"
#include "ImageConverter.h"
#include "gmic_qt.h"
#include "gmic.h"

namespace host_paintdotnet
{
    QVector<QImage> layerImages;

    QString outputImagePath;
}

namespace GmicQt
{
    const QString HostApplicationName = QString("Paint.NET");
    const char * HostApplicationShortname = GMIC_QT_XSTRINGIFY(GMIC_HOST);
    const bool DarkThemeIsDefault = false;
}

void gmic_qt_get_layers_extent(int * width, int * height, GmicQt::InputMode)
{
    // Paint.NET layers are all the same size.
    const QImage firstLayer = host_paintdotnet::layerImages.at(0);

    *width = firstLayer.width();
    *height = firstLayer.height();
}

void gmic_qt_get_cropped_images(gmic_list<float> & images, gmic_list<char> & imageNames, double x, double y, double width, double height, GmicQt::InputMode mode)
{
    if (mode == GmicQt::NoInput)
    {
        images.assign();
        imageNames.assign();
        return;
    }

    const bool entireImage = x < 0 && y < 0 && width < 0 && height < 0;
    if (entireImage)
    {
        x = 0.0;
        y = 0.0;
        width = 1.0;
        height = 1.0;
    }

    const int layerCount = host_paintdotnet::layerImages.count();

    images.assign(layerCount);
    imageNames.assign(layerCount);

    for (int i = 0; i < layerCount; i++)
    {
        QString layerName = QString("pos(0,0),name(layer%1)").arg(i);
        QByteArray layerNameBytes = layerName.toUtf8();
        gmic_image<char>::string(layerNameBytes.constData()).move_to(imageNames[i]);
    }

    int imageWidth;
    int imageHeight;
    gmic_qt_get_layers_extent(&imageWidth, &imageHeight, mode);

    const int ix = entireImage ? 0 : static_cast<int>(std::floor(x * imageWidth));
    const int iy = entireImage ? 0 : static_cast<int>(std::floor(y * imageHeight));
    const int iw = entireImage ? imageWidth : std::min(imageWidth - ix, static_cast<int>(1 + std::ceil(width * imageWidth)));
    const int ih = entireImage ? imageHeight : std::min(imageHeight - iy, static_cast<int>(1 + std::ceil(height * imageHeight)));

    for (int i = 0; i < layerCount; i++)
    {
        ImageConverter::convert(host_paintdotnet::layerImages.at(i).copy(ix, iy, iw, ih), images[i]);
    }
}

void gmic_qt_output_images(gmic_list<float> & images, const gmic_list<char> & imageNames, GmicQt::OutputMode mode, const char * verboseLayersLabel)
{
    unused(imageNames);
    unused(mode);
    unused(verboseLayersLabel);

    if (images.size() > 0)
    {
        QImage outputImage;

        ImageConverter::convert(images[0], outputImage);

        outputImage.save(host_paintdotnet::outputImagePath);
    }
}

void gmic_qt_apply_color_profile(cimg_library::CImg<gmic_pixel_type> & images)
{
    unused(images);
}

void gmic_qt_show_message(const char * message)
{
    unused(message);
}

int main(int argc, char *argv[])
{
    QString firstLayerImagePath;
    QString secondLayerImagePath;
    bool useLastParameters = false;

    if (argc >= 4)
    {
        firstLayerImagePath = argv[1];
        secondLayerImagePath = argv[2];
        host_paintdotnet::outputImagePath = argv[3];
        if (argc == 5)
        {
            useLastParameters = strcmp(argv[4], "reapply") == 0;
        }
    }
    else
    {
        std::cerr << "Usage: gmic_paintdotnet_qt first_image second_image output_image\n";
        return 1;
    }

    if (firstLayerImagePath.isEmpty())
    {
        std::cerr << "Input filename is an empty string.\n";
        return 1;
    }

    if (host_paintdotnet::outputImagePath.isEmpty())
    {
        std::cerr << "Output filename is an empty string.\n";
        return 1;
    }

    //  disableInputMode(GmicQt::NoInput);
    disableInputMode(GmicQt::Active);
    //  disableInputMode(GmicQt::All);
    disableInputMode(GmicQt::ActiveAndBelow);
    disableInputMode(GmicQt::ActiveAndAbove);
    disableInputMode(GmicQt::AllVisible);
    disableInputMode(GmicQt::AllInvisible);

    // disableOutputMode(GmicQt::InPlace);
    disableOutputMode(GmicQt::NewImage);
    disableOutputMode(GmicQt::NewLayers);
    disableOutputMode(GmicQt::NewActiveLayers);


    QImage firstLayer(firstLayerImagePath);

    if (!firstLayer.isNull())
    {
        host_paintdotnet::layerImages.append(firstLayer.convertToFormat(QImage::Format_ARGB32));

        // Paint.NET can optionally pass a second image for the filters that require two layers.

        if (!secondLayerImagePath.isEmpty())
        {
            QImage secondLayer(secondLayerImagePath);

            if (!secondLayer.isNull())
            {
                host_paintdotnet::layerImages.append(secondLayer.convertToFormat(QImage::Format_ARGB32));
            }
        }

        if (useLastParameters)
        {
            return launchPluginHeadlessUsingLastParameters();
        }
        else
        {
            return launchPlugin();
        }
    }

    std::cerr << "Unable to load the input image" << firstLayerImagePath.toLocal8Bit().constData() << "\n";
    return 1;
}

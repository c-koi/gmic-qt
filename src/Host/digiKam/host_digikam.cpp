/*
*  This file is part of G'MIC-Qt, a generic plug-in for raster graphics
*  editors, offering hundreds of filters thanks to the underlying G'MIC
*  image processing framework.
*
*  Copyright (C) 2019 Gilles Caulier <caulier dot gilles at gmail dot com>
*
*  Description: digiKam image editor plugin for GmicQt.
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

// Qt includes

#include <QDataStream>
#include <QString>
#include <QTextStream>
#include <QDebug>

// Local includes

#include "Common.h"
#include "Host/host.h"
#include "gmic.h"

// digiKam includes

#include "imageiface.h"

using namespace Digikam;

namespace GmicQt
{
    const QString HostApplicationName    = QString("digiKam");
    const char* HostApplicationShortname = GMIC_QT_XSTRINGIFY(GMIC_HOST);
    const bool DarkThemeIsDefault        = false;
}

// Helper method for DImg to CImg container conversions

namespace
{

inline bool archIsLittleEndian()
{
    const int x = 1;
    return (*reinterpret_cast<const unsigned char *>(&x));
}

inline unsigned char float2uchar_bounded(const float& in)
{
    return (in < 0.0f) ? 0 : ((in > 255.0f) ? 255 : static_cast<unsigned char>(in));
}

inline unsigned short float2ushort_bounded(const float& in)
{
    return (in < 0.0f) ? 0 : ((in > 65535.0f) ? 65535 : static_cast<unsigned short>(in));
}

void convertCImgtoDImg(const cimg_library::CImg<float>& in, DImg& out, bool sixteenBits)
{
    Q_ASSERT_X(in.spectrum() <= 4, "ImageConverter::convert()", QString("bad input spectrum (%1)").arg(in.spectrum()).toLatin1());

    bool alpha = (in.spectrum() == 4 || in.spectrum() == 2);
    out        = DImg(in.width(), in.height(), sixteenBits, alpha);

    if (in.spectrum() == 4) // RGB + Alpha
    {
        const float* srcR = in.data(0, 0, 0, 0);
        const float* srcG = in.data(0, 0, 0, 1);
        const float* srcB = in.data(0, 0, 0, 2);
        const float* srcA = in.data(0, 0, 0, 3);
        int height        = out.height();

        for (int y = 0 ; y < height ; ++y)
        {
            int n = in.width();

            if (sixteenBits)
            {
                unsigned short* dst = (unsigned short*)out.scanLine(y);

                while (n--)
                {
                    dst[0] = float2ushort_bounded(*srcR++);
                    dst[1] = float2ushort_bounded(*srcG++);
                    dst[2] = float2ushort_bounded(*srcB++);
                    dst[3] = float2ushort_bounded(*srcA++);
                    dst   += 4;
                }
            }
            else
            {
                unsigned char* dst = out.scanLine(y);

                while (n--)
                {
                    dst[0] = float2uchar_bounded(*srcR++);
                    dst[1] = float2uchar_bounded(*srcG++);
                    dst[2] = float2uchar_bounded(*srcB++);
                    dst[3] = float2uchar_bounded(*srcA++);
                    dst   += 4;
                }
            }
        }
    }
    if (in.spectrum() == 3) // RGB
    {
        const float* srcR = in.data(0, 0, 0, 0);
        const float* srcG = in.data(0, 0, 0, 1);
        const float* srcB = in.data(0, 0, 0, 2);
        int height        = out.height();

        for (int y = 0 ; y < height ; ++y)
        {
            int n = in.width();

            if (sixteenBits)
            {
                unsigned short* dst = (unsigned short*)out.scanLine(y);

                while (n--)
                {
                    dst[0] = float2ushort_bounded(*srcR++);
                    dst[1] = float2ushort_bounded(*srcG++);
                    dst[2] = float2ushort_bounded(*srcB++);
                    dst[3] = 0;
                    dst   += 4;
                }
            }
            else
            {
                unsigned char* dst = out.scanLine(y);

                while (n--)
                {
                    dst[0] = float2uchar_bounded(*srcR++);
                    dst[1] = float2uchar_bounded(*srcG++);
                    dst[2] = float2uchar_bounded(*srcB++);
                    dst[3] = 0;
                    dst   += 4;
                }
            }
        }
    }
    else if (in.spectrum() == 2) // Gray levels + Alpha
    {
        const float* src  = in.data(0, 0, 0, 0);
        const float* srcA = in.data(0, 0, 0, 1);
        int height        = out.height();

        for (int y = 0 ; y < height ; ++y)
        {
            int n = in.width();

            if (sixteenBits)
            {
                unsigned short* dst = (unsigned short*)out.scanLine(y);

                while (n--)
                {
                    dst[0] = dst[1] = dst[2] = float2ushort_bounded(*src++);
                    dst[3] = float2ushort_bounded(*srcA++);
                    dst   += 4;
                }
            }
            else
            {
                unsigned char* dst = out.scanLine(y);

                while (n--)
                {
                    dst[0] = dst[1] = dst[2] = float2uchar_bounded(*src++);
                    dst[3] = float2uchar_bounded(*srcA++);
                    dst   += 4;
                }
            }
        }
    }
    else // Gray levels
    {
        const float* src  = in.data(0, 0, 0, 0);
        int height        = out.height();

        for (int y = 0 ; y < height ; ++y)
        {
            int n = in.width();

            if (sixteenBits)
            {
                unsigned short* dst = (unsigned short*)out.scanLine(y);

                while (n--)
                {
                    dst[0] = dst[1] = dst[2] = float2ushort_bounded(*src++);
                    dst[3] = 0;
                    dst   += 4;
                }
            }
            else
            {
                unsigned char* dst = out.scanLine(y);

                while (n--)
                {
                    dst[0] = dst[1] = dst[2] = float2uchar_bounded(*src++);
                    dst[3] = 0;
                    dst   += 4;
                }
            }
        }
    }
}

void convertDImgtoCImg(const DImg& in, cimg_library::CImg<float>& out)
{
    const int w = in.width();
    const int h = in.height();
    out.assign(w, h, 1, 4);

    float* dstR = out.data(0, 0, 0, 0);
    float* dstG = out.data(0, 0, 0, 1);
    float* dstB = out.data(0, 0, 0, 2);
    float* dstA = out.data(0, 0, 0, 3);

    if (archIsLittleEndian())
    {
        for (int y = 0 ; y < h ; ++y)
        {
            const unsigned char* src = in.scanLine(y);
            int n                    = in.width();

            while (n--)
            {
                *dstB++ = static_cast<float>(src[0]);
                *dstG++ = static_cast<float>(src[1]);
                *dstR++ = static_cast<float>(src[2]);
                *dstA++ = static_cast<float>(src[3]);
                src    += 4;
            }
        }
    }
    else
    {
        for (int y = 0 ; y < h ; ++y)
        {
            const unsigned char* src = in.scanLine(y);
            int n                    = in.width();

            while (n--)
            {
                *dstA++ = static_cast<float>(src[0]);
                *dstR++ = static_cast<float>(src[1]);
                *dstG++ = static_cast<float>(src[2]);
                *dstB++ = static_cast<float>(src[3]);
                src    += 4;
            }
        }
    }
}

} // namespace

// --- GMic-Qt plugin functions ----------------------

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

    if (mode == GmicQt::NoInput)
    {
        images.assign();
        imageNames.assign();
        return;
    }

    ImageIface iface;
    DImg* const input_image = iface.original();

    const bool entireImage = (x < 0) && (y < 0) && (width < 0) && (height < 0);

    if (entireImage)
    {
        x      = 0.0;
        y      = 0.0;
        width  = 1.0;
        height = 1.0;
    }

    images.assign(1);
    imageNames.assign(1);

    QString name  = QString("pos(0,0),name(%1)").arg("Image Editor Canvas");
    QByteArray ba = name.toUtf8();
    gmic_image<char>::string(ba.constData()).move_to(imageNames[0]);

    const int ix = static_cast<int>(entireImage ? 0 : std::floor(x * input_image->width()));
    const int iy = static_cast<int>(entireImage ? 0 : std::floor(y * input_image->height()));
    const int iw = entireImage ? input_image->width()  : std::min(static_cast<int>(input_image->width()  - ix), static_cast<int>(1 + std::ceil(width  * input_image->width())));
    const int ih = entireImage ? input_image->height() : std::min(static_cast<int>(input_image->height() - iy), static_cast<int>(1 + std::ceil(height * input_image->height())));

    convertDImgtoCImg(input_image->copy(ix, iy, iw, ih), images[0]);
}

void gmic_qt_output_images(gmic_list<float>& images,
                           const gmic_list<char>& imageNames,
                           GmicQt::OutputMode mode,
                           const char* verboseLayersLabel)
{
    qDebug() << "Calling gmic_qt_output_images()";

    if (images.size() > 0)
    {
        ImageIface iface;
        DImg dest;
        convertCImgtoDImg(images[0], dest, iface.originalSixteenBit());
        FilterAction action(QLatin1String("GMic-Qt"), 1);
        iface.setOriginal(QLatin1String("GMic-Qt"), action, dest);
    }
}

void gmic_qt_apply_color_profile(cimg_library::CImg<gmic_pixel_type>& images)
{
    qDebug() << "Calling gmic_qt_apply_color_profile()";

    Q_UNUSED(images);
}

void gmic_qt_show_message(const char* message)
{
    qDebug() << "Calling gmic_qt_show_message()";
    qDebug() << "GMic-Qt:" << message;
}

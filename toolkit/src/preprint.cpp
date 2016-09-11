#include <Magick++.h>
#include <iostream>
#include <sstream>
#include "smart_crop.hpp"


void MakeBase(Magick::Image & image, const std::string & outmask);
void MakeGrayScale(Magick::Image & image, const std::string & outmask);
void CheckCMYK(Magick::Image & image, const std::string & outmask);
void CheckAspectRatio(Magick::Image & image, const std::string & outmask);
void CheckSize(Magick::Image & image, const std::string & outmask);
void CheckBorder(Magick::Image & image, const std::string & outmask);
void ConvertToPDF(Magick::Image & image, const std::string & outmask);


int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        std::cout << "Error: bad args" << std::endl;
        return 1;
    }

    std::string command(argv[1]);
    std::string input(argv[2]);
    std::string outputmask(argv[3]);

    try
    {
        Magick::InitializeMagick(*argv);

        Magick::Image image;
        image.density("300");
        image.read(input);

        if (command == "make-base")
        {
            MakeBase(image, outputmask);
        }
        if (command == "make-grayscale")
        {
            MakeGrayScale(image, outputmask);
        }
        if (command == "check-cmyk")
        {
            CheckCMYK(image, outputmask);
        }
        else if (command == "check-aspect-ratio")
        {
            CheckAspectRatio(image, outputmask);
        }
        else if (command == "check-size")
        {
            CheckSize(image, outputmask);
        }
        else if (command == "check-border")
        {
            CheckBorder(image, outputmask);
        }
        else if (command == "convert-to-pdf")
        {
            ConvertToPDF(image, outputmask);
        }
    }
    catch (std::exception & ex)
    {
        std::cout << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}

Magick::Image ConvertToCMYK(Magick::Image & image)
{
    Magick::Image res(image);
    res.colorSpace(MagickCore::LabColorspace);
    res.colorSpace(MagickCore::CMYKColorspace);
    return res;
}

Magick::Image Rescale(Magick::Image & image, int width, int height)
{
    Magick::Geometry frame(width, height);
    frame.aspect(true);

    Magick::Image res(image);
    res.scale(frame);
    return res;
}

Magick::Image LiquidRescale(Magick::Image & image, int width, int height)
{
    Magick::Geometry frame(width, height);
    frame.aspect(true);

    Magick::Image res(image);
    res.liquidRescale(frame);
    return res;
}

Magick::Image AddBorder(Magick::Image & image, int width, int height)
{
    Magick::Image res(image);

    size_t border_width = 0;
    size_t border_height = 0;

    if (res.size().width() < width)
    {
        size_t d = width - res.size().width();
        if (d % 2)
        {
            res.crop(Magick::Geometry(res.size().width() - 1, res.size().height()));
        }
        border_width = (d + 1) / 2;
    }

    if (res.size().height() < height)
    {
        size_t d = height - res.size().height();
        if (d % 2)
        {
            res.crop(Magick::Geometry(res.size().width(), res.size().height() - 1));
        }
        border_height = (d + 1) / 2;
    }

    if (res.colorSpace() == MagickCore::CMYKColorspace)
    {
        res.borderColor(Magick::ColorCMYK(0, 0, 0, 0));
    }
    else
    {
        res.borderColor(Magick::ColorRGB(65535, 65535, 65535));
    }
    res.border(Magick::Geometry(border_width, border_height));

    return res;
}

void SaveCorrectedImage(Magick::Image & image, const std::string & outmask, const std::string & suffix, const std::string & descr)
{
    std::stringstream ss;
    ss << outmask << "_" << suffix;

    std::string outfile = ss.str();
    image.write(outfile);

    std::string preview = ss.str() + ".jpg";
    image.write(preview);

    std::cout << outfile << "\t" << preview << "\t" << descr << std::endl;
}

void MakeBase(Magick::Image & image, const std::string & outmask)
{
    // Простое ограничение на размер файлов
    if (image.size().width() > 2000 ||
        image.size().height() > 2000)
    {
        return;
    }

    SaveCorrectedImage(image, outmask, "base", "Save as is");
}

void MakeGrayScale(Magick::Image & image, const std::string & outmask)
{
    image.type(Magick::GrayscaleAlphaType);
    SaveCorrectedImage(image, outmask, "grayscale", "Convert to grayscale");
}

void CheckCMYK(Magick::Image & image, const std::string & outmask)
{
    if (image.colorSpace() != MagickCore::CMYKColorspace)
    {
        Magick::Image converted = ConvertToCMYK(image);
        SaveCorrectedImage(converted, outmask, "cmyk", "Converted to cmyk colorspace");
    }
}

void CheckAspectRatio(Magick::Image & image, const std::string & outmask)
{
    const double kAspectRatio = 9 / 5.;
    const double image_as = image.size().width() / (double)image.size().height();

    if (image_as == kAspectRatio)
    {
        return;
    }

    int width = 0;
    int height = 0;
    if (image_as > kAspectRatio)
    {
        width = image.size().height() * kAspectRatio;
        height = image.size().height();
    }
    else
    {
        width = image.size().width();
        height = image.size().width() / kAspectRatio;
    }

    Magick::Image rescaled = Rescale(image, width, height);
    SaveCorrectedImage(rescaled, outmask, "aspect_rescale", "Rescale");

    Magick::Image lq_rescaled = LiquidRescale(image, width, height);
    SaveCorrectedImage(lq_rescaled, outmask, "aspect_lq_rescale", "Liquid rescale");

    Magick::Image cropped = SmartCrop(image, width, height);
    SaveCorrectedImage(lq_rescaled, outmask, "aspect_crop", "Smart crop");
}

void CheckSize(Magick::Image & image, const std::string & outmask)
{
    const int kDpi = 300;
    const int kBorder = 5;
    const int kPhisWidth = 90;
    const int kPhisHeight = 50;

    const int width = (kPhisWidth + 2 * kBorder) / 25.4 * kDpi;
    const int height = (kPhisHeight + 2 * kBorder) / 25.4 * kDpi;
    const int width_no_border = kPhisWidth / 25.4 * kDpi;
    const int height_no_border = kPhisHeight / 25.4 * kDpi;

    Magick::Image rescaled = Rescale(image, width, height);
    SaveCorrectedImage(rescaled, outmask, "size_rescale", "Rescale");

    Magick::Image lq_rescaled = LiquidRescale(image, width, height);
    SaveCorrectedImage(lq_rescaled, outmask, "size_lq_rescale", "Liquid rescale");

    Magick::Image cropped = SmartCrop(image, width, height);
    SaveCorrectedImage(lq_rescaled, outmask, "size_crop", "Smart crop");

    Magick::Image rescaled_no_border = Rescale(image, width_no_border, height_no_border);
    Magick::Image with_bolder = AddBorder(rescaled_no_border, width, height);
    SaveCorrectedImage(with_bolder, outmask, "size_border", "Rescale and add border");
}

void CheckBorder(Magick::Image & image, const std::string & outmask)
{
    const int kDpi = 300;
    const int kBorder = 5;
    const int border_width = kBorder / 25.4 * kDpi;

    Magick::Geometry oframe = GetObjectsFrame(image);

    ssize_t x_overflow = 0;
    ssize_t x_offset = 0;
    if (oframe.xOff() < border_width)
    {
        x_overflow += border_width - oframe.xOff();
        x_offset = border_width - oframe.xOff();
    }
    if (image.size().width() - (oframe.xOff() + oframe.width()) < border_width)
    {
        x_overflow += (oframe.xOff() + oframe.width()) - (image.size().width() - border_width);
    }

    ssize_t y_overflow = 0;
    ssize_t y_offset = 0;
    if (oframe.yOff() < border_width)
    {
        y_overflow += border_width - oframe.yOff();
        y_offset = border_width - oframe.yOff();
    }
    if (image.size().height() - (oframe.yOff() + oframe.height()) < border_width)
    {
        y_overflow += (oframe.yOff() + oframe.height()) - (image.size().height() - border_width);
    }

    if (x_overflow <= 0 &&
        y_overflow <= 0)
    {
        return;
    }

    Magick::Geometry rescale_frame(image.size().width() - x_overflow,
                                   image.size().height() - y_overflow);
    rescale_frame.aspect(true);
    Magick::Image rescaled(image);
    rescaled.liquidRescale(rescale_frame);

    Magick::Geometry base_frame(image.size().width(),
                                image.size().height());
    Magick::Image white(base_frame, Magick::Color("white"));
    white.magick(image.magick());
    white.colorSpace(image.colorSpace());
    white.composite(rescaled, x_offset, y_offset);

    SaveCorrectedImage(white, outmask, "border_lq_scale", "Enclose objects into safe zone");
}

void ConvertToPDF(Magick::Image & image, const std::string & outmask)
{
    std::string outfile = outmask + ".pdf";
    image.write(outfile);

    std::cout << outfile << "\t\t" << "Converted to pdf" << std::endl;
}


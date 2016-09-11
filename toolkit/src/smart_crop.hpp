#pragma once

#include <Magick++.h>


void GetCrop(int orig, int dest, int frame_off, int frame_width, int *dl, int *dr)
{
    *dl = 0;
    *dr = 0;

    int dd = orig - dest;

    int a = std::min(frame_off, orig - frame_width - frame_off);
    if (dd <= a)
    {
        *dl += dd / 2;
        *dr += dd - dd / 2;
        return;
    }
    *dl += a / 2;
    *dr += a - a / 2;
    dd -= a;

    if (frame_off > orig - frame_width - frame_off)
    {
        int b = frame_off - (orig - frame_width - frame_off);
        if (dd <= b)
        {
            *dl += dd;
            return;
        }
        *dl += b;
        dd -= b;
    }
    else
    {
        int b = (orig - frame_width - frame_off) - frame_off;
        if (dd <= b)
        {
            *dr += dd;
            return;
        }
        *dr += b;
        dd -= b;
    }

    *dl += dd / 2;
    *dr += dd - dd / 2;
}

Magick::Geometry GetObjectsFrame(Magick::Image & image)
{
    Magick::Image temp(image);
    temp.cannyEdge();

    size_t width = temp.size().width();
    size_t height = temp.size().height();
    size_t bottom = height;
    size_t top = 0;
    size_t left = width;
    size_t right = 0;
    for (int i = 0; i < width; ++i)
    {
        for (int j = 0; j < height; ++j)
        {
            if (temp.pixelColor(i, j).quantumBlue() > 1000)
            {
                if (j < bottom) bottom = j;
                if (j > top) top = j;
                if (i < left) left = i;
                if (i > right) right = i;
            }
        }
    }
    if (left > right || bottom > top)
    {
        return Magick::Geometry(0, 0, 0, 0);
    }
    return Magick::Geometry(right - left, top - bottom, left, bottom);
}

Magick::Image SmartCrop(Magick::Image & image, int width, int height)
{
    Magick::Image res(image);
    Magick::Geometry oframe = GetObjectsFrame(res);
    Magick::Geometry cropframe(res.size().width(), res.size().height());

    size_t base_width = res.size().width();
    if (base_width > width)
    {
        int dl;
        int dr;
        GetCrop(base_width, width, oframe.xOff(), oframe.width(), &dl, &dr);
        cropframe.xOff(dl);
        cropframe.width(base_width - dl - dr);
    }

    size_t base_height = res.size().height();
    if (base_height > height)
    {
        int dl;
        int dr;
        GetCrop(base_height, height, oframe.yOff(), oframe.height(), &dl, &dr);
        cropframe.yOff(dl);
        cropframe.height(base_height - dl - dr);
    }

    res.crop(cropframe);
    return res;
}


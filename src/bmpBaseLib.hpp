#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <algorithm>


struct RGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    constexpr RGB(unsigned r, unsigned g, unsigned b)
        : r(static_cast<uint8_t>(r)),
          g(static_cast<uint8_t>(g)),
          b(static_cast<uint8_t>(b))
    {}
};


inline RGB gray(unsigned value) {
    if (value > 255)
        throw std::out_of_range("Gray value must be 0..255");

    return RGB(value, value, value);
}


namespace Color {

    inline constexpr RGB BLACK   = { 0,   0,   0   };
    inline constexpr RGB WHITE   = { 255, 255, 255 };

    inline constexpr RGB RED     = { 255, 0,   0   };
    inline constexpr RGB GREEN   = { 0,   255, 0   };
    inline constexpr RGB BLUE    = { 0,   0,   255 };

    inline constexpr RGB YELLOW  = { 255, 255, 0   };
    inline constexpr RGB CYAN    = { 0,   255, 255 };
    inline constexpr RGB MAGENTA = { 255, 0,   255 };

    inline constexpr RGB GRAY    = { 128, 128, 128 };

    inline constexpr RGB ORANGE  = { 255, 128, 0   };
    inline constexpr RGB PURPLE  = { 128, 0,   255 };
    inline constexpr RGB PINK    = { 255, 128, 192 };
}


class BMP {
private:

    int width = 0;
    int height = 0;

    // RGBRGBRGB...
    std::vector<uint8_t> pixels;


    size_t index(int x, int y) const {
        return (static_cast<size_t>(y) * width + x) * 3;
    }


public:

    BMP(int width, int height)
        : width(width),
          height(height),
          pixels(static_cast<size_t>(width) * height * 3, 0)
    {
        if (width <= 0 || height <= 0)
            throw std::runtime_error("Invalid image size");
    }


    BMP(const std::string& filename) {
        load(filename);
    }


    int getWidth() const {
        return width;
    }


    int getHeight() const {
        return height;
    }


    RGB get(int x, int y) const {

        if (x < 0 || x >= width ||
            y < 0 || y >= height)
            throw std::out_of_range(
                "Pixel coordinates out of range"
            );

        size_t i = index(x, y);

        return RGB(
            pixels[i],
            pixels[i + 1],
            pixels[i + 2]
        );
    }


    void setDirect(int x, int y, RGB color) {

        if (x < 0 || x >= width ||
            y < 0 || y >= height)
            throw std::out_of_range(
                "Pixel coordinates out of range"
            );

        size_t i = index(x, y);

        pixels[i]     = color.r;
        pixels[i + 1] = color.g;
        pixels[i + 2] = color.b;
    }


    void setByPos(int x, int y, RGB color) {

        if (x < 0 || x >= width ||
            (height-y) < 0 || (height-y) >= height)
            throw std::out_of_range(
                "Pixel coordinates out of range"
            );

        size_t i = index(x, (height-y));

        pixels[i]     = color.r;
        pixels[i + 1] = color.g;
        pixels[i + 2] = color.b;
    }

    void save(const std::string& filename) const {

        std::ofstream file(
            filename,
            std::ios::binary
        );

        if (!file)
            throw std::runtime_error(
                "Cannot open file"
            );


        int rowSize =
            ((width * 3 + 3) / 4) * 4;

        int pixelDataSize =
            rowSize * height;

        int fileSize =
            14 + 40 + pixelDataSize;


        // BITMAPFILEHEADER

        uint16_t bfType = 0x4D42;
        uint32_t bfSize = fileSize;
        uint16_t bfReserved1 = 0;
        uint16_t bfReserved2 = 0;
        uint32_t bfOffBits = 54;

        file.write((char*)&bfType, 2);
        file.write((char*)&bfSize, 4);
        file.write((char*)&bfReserved1, 2);
        file.write((char*)&bfReserved2, 2);
        file.write((char*)&bfOffBits, 4);


        // BITMAPINFOHEADER

        uint32_t biSize = 40;

        int32_t biWidth = width;
        int32_t biHeight = height;

        uint16_t biPlanes = 1;
        uint16_t biBitCount = 24;

        uint32_t biCompression = 0;
        uint32_t biSizeImage = pixelDataSize;

        int32_t biXPelsPerMeter = 2835;
        int32_t biYPelsPerMeter = 2835;

        uint32_t biClrUsed = 0;
        uint32_t biClrImportant = 0;

        file.write((char*)&biSize, 4);
        file.write((char*)&biWidth, 4);
        file.write((char*)&biHeight, 4);
        file.write((char*)&biPlanes, 2);
        file.write((char*)&biBitCount, 2);
        file.write((char*)&biCompression, 4);
        file.write((char*)&biSizeImage, 4);
        file.write((char*)&biXPelsPerMeter, 4);
        file.write((char*)&biYPelsPerMeter, 4);
        file.write((char*)&biClrUsed, 4);
        file.write((char*)&biClrImportant, 4);


        // Pixels

        std::vector<uint8_t> row(rowSize);

        for (int y = height - 1; y >= 0; y--) {

            std::fill(row.begin(), row.end(), 0);

            for (int x = 0; x < width; x++) {

                size_t i = index(x, y);

                // BMP использует BGR

                row[x * 3]     = pixels[i + 2];
                row[x * 3 + 1] = pixels[i + 1];
                row[x * 3 + 2] = pixels[i];
            }

            file.write(
                (char*)row.data(),
                rowSize
            );
        }


        if (!file)
            throw std::runtime_error(
                "Error writing BMP"
            );
    }


private:

    void load(const std::string& filename) {

        std::ifstream file(
            filename,
            std::ios::binary
        );

        if (!file)
            throw std::runtime_error(
                "Cannot open BMP"
            );


        // BITMAPFILEHEADER

        uint16_t bfType;
        uint32_t bfSize;
        uint16_t bfReserved1;
        uint16_t bfReserved2;
        uint32_t bfOffBits;

        file.read((char*)&bfType, 2);
        file.read((char*)&bfSize, 4);
        file.read((char*)&bfReserved1, 2);
        file.read((char*)&bfReserved2, 2);
        file.read((char*)&bfOffBits, 4);

        if (bfType != 0x4D42)
            throw std::runtime_error(
                "Not a BMP file"
            );


        // BITMAPINFOHEADER

        uint32_t biSize;

        int32_t biWidth;
        int32_t biHeight;

        uint16_t biPlanes;
        uint16_t biBitCount;

        uint32_t biCompression;
        uint32_t biSizeImage;

        int32_t biXPelsPerMeter;
        int32_t biYPelsPerMeter;

        uint32_t biClrUsed;
        uint32_t biClrImportant;

        file.read((char*)&biSize, 4);
        file.read((char*)&biWidth, 4);
        file.read((char*)&biHeight, 4);
        file.read((char*)&biPlanes, 2);
        file.read((char*)&biBitCount, 2);
        file.read((char*)&biCompression, 4);
        file.read((char*)&biSizeImage, 4);
        file.read((char*)&biXPelsPerMeter, 4);
        file.read((char*)&biYPelsPerMeter, 4);
        file.read((char*)&biClrUsed, 4);
        file.read((char*)&biClrImportant, 4);


        if (biSize != 40)
            throw std::runtime_error(
                "Unsupported BMP header"
            );

        if (biPlanes != 1)
            throw std::runtime_error(
                "Invalid BMP"
            );

        if (biBitCount != 24)
            throw std::runtime_error(
                "Only 24-bit BMP is supported"
            );

        if (biCompression != 0)
            throw std::runtime_error(
                "Compressed BMP is not supported"
            );


        width = biWidth;

        bool topDown = biHeight < 0;

        height = biHeight < 0
               ? -biHeight
               : biHeight;


        if (width <= 0 || height <= 0)
            throw std::runtime_error(
                "Invalid BMP size"
            );


        pixels.resize(
            static_cast<size_t>(width) *
            height * 3
        );


        file.seekg(bfOffBits);


        int rowSize =
            ((width * 3 + 3) / 4) * 4;

        std::vector<uint8_t> row(rowSize);


        for (int rowIndex = 0;
             rowIndex < height;
             rowIndex++) {

            file.read(
                (char*)row.data(),
                rowSize
            );

            if (!file)
                throw std::runtime_error(
                    "Unexpected end of BMP"
                );


            int y = topDown
                  ? rowIndex
                  : height - 1 - rowIndex;


            for (int x = 0; x < width; x++) {

                size_t i = index(x, y);

                // BGR -> RGB

                pixels[i]     = row[x * 3 + 2];
                pixels[i + 1] = row[x * 3 + 1];
                pixels[i + 2] = row[x * 3];
            }
        }
    }
};

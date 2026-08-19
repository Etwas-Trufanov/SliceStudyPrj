#include <cstddef>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <variant>
#include <vector>
#include <cmath>
#include "bmpBaseLib.hpp"


enum normalDirection {
    Inside,
    Outside
};

struct TPoint {
    float x, y;
    TPoint() : x(0), y(0) {};
    TPoint(float x, float y) : x(x), y(y) {};

    // Перегруженный оператор сравнения точек
    bool operator==(const TPoint &other) const {
        return ((x == other.x) and (y == other.y));
    }
};

struct TLine {
    TPoint pos1;
    TPoint pos2;
    normalDirection normal;

    std::size_t next = 0xFFFFFFFFFFFF;

    TLine() : pos1(TPoint()), pos2(TPoint()), normal(normalDirection::Outside) {};
    TLine(TPoint pos1, TPoint pos2) : pos1(pos1), pos2(pos2), normal(normalDirection::Outside) {};

};

void connectLines(std::vector<TLine> &outline) {
    for (std::size_t i = 0; i < outline.size(); i++) {
        for (std::size_t j = 0; j < outline.size(); j++) {
            if (i == j) continue;
            if (outline[i].pos2 == outline[j].pos1) {
                outline[i].next = j;
            }
        }
    }
}

void offsetPoints(std::vector<TLine> &outline, float distance) {

}

void loadModel(std::vector<TLine> &outline) {
    std::ifstream f;
    f.open("outline.txt");
    if (f.is_open()) {

        int lineCounter = 1;

        std::string token;

        float pos1x;
        float pos1y;
        float pos2x;
        float pos2y;

        while (std::getline(f, token)) {
            if (token.find("endl") == std::variant_npos) {

            }
            std::stringstream tokenstream;
            tokenstream << token;
            tokenstream >> pos1x >> pos1y >> pos2x >> pos2y;

            outline.emplace_back(TPoint(pos1x, pos1y), TPoint(pos2x, pos2y));
            lineCounter++;
        }
        connectLines(outline);
    }
    for (std::size_t i = 0; i < outline.size(); i++) {
        std::cout << "Line " << i << ": x1: " << outline[i].pos1.x << " y1: " << outline[i].pos1.y << " x2: " << outline[i].pos2.x << " y2: " << outline[i].pos2.y << " | Next: " << outline[i].next << std::endl;
    }

    f.close();
}

int main() {

    std::vector<TLine> outline;
    loadModel(outline);

    BMP image(128, 128);

    unsigned colorOffset = 255;

    for (std::size_t i = 0; i < outline.size(); i++) {
        float floatdy = outline[i].pos2.y - outline[i].pos1.y;
        float floatdx = outline[i].pos2.x - outline[i].pos1.x;

        if (std::abs(std::round(floatdx)) == 0) {
            if (floatdy > 0) {
                unsigned ylinestart = std::abs(std::round(outline[i].pos1.y));
                unsigned ylineend = std::abs(std::round(outline[i].pos2.y));
                for (unsigned y = ylinestart; y < ylineend+1; y++) {
                    image.setByPos(std::abs(std::round(outline[i].pos1.x)), y, gray(colorOffset));
                }
                colorOffset-=5;
            } else {
                unsigned ylinestart = std::abs(std::round(outline[i].pos2.y));
                unsigned ylineend = std::abs(std::round(outline[i].pos1.y));
                for (unsigned y = ylinestart; y < ylineend+1; y++) {
                    image.setByPos(std::abs(std::round(outline[i].pos1.x)), y, gray(colorOffset));
                }
                colorOffset-=5;
            }
        } else
        if (std::abs(std::round(floatdx)) > std::abs(std::round(floatdy))) {
            if (floatdx > 0) {
                unsigned xlinestart = std::abs(std::round(outline[i].pos1.x));
                unsigned xlineend = std::abs(std::round(outline[i].pos2.x));
                for (unsigned x = xlinestart; x < xlineend+1; x++) {
                    image.setByPos(x, std::abs(std::round(outline[i].pos1.y+((floatdy*(x-xlinestart))/floatdx))), gray(colorOffset));
                }
                colorOffset-=5;
            } else {
                unsigned xlinestart = std::abs(std::round(outline[i].pos2.x));
                unsigned xlineend = std::abs(std::round(outline[i].pos1.x));
                for (unsigned x = xlinestart; x < xlineend+1; x++) {
                    image.setByPos(x, std::abs(std::round(outline[i].pos2.y+((floatdy*(x-xlinestart))/floatdx))), gray(colorOffset));
                }
                colorOffset-=5;
            }
        } else
        if (std::abs(std::round(floatdx)) < std::abs(std::round(floatdy))) {
            if (floatdy > 0) {
                unsigned ylinestart = std::abs(std::round(outline[i].pos1.y));
                unsigned ylineend = std::abs(std::round(outline[i].pos2.y));
                for (unsigned y = ylinestart; y < ylineend+1; y++) {
                    image.setByPos(std::abs(std::round(outline[i].pos1.x+((floatdx*(y-ylinestart))/floatdy))), y, gray(colorOffset));
                }
                colorOffset-=5;
            } else {
                unsigned ylinestart = std::abs(std::round(outline[i].pos2.y));
                unsigned ylineend = std::abs(std::round(outline[i].pos1.y));
                for (unsigned y = ylinestart; y < ylineend+1; y++) {
                    image.setByPos(std::abs(std::round(outline[i].pos2.x+((floatdx*(y-ylinestart))/floatdy))), y, gray(colorOffset));
                }
                colorOffset-=5;
            }
        }

    }

    image.save("result.bmp");

    return 0;
}

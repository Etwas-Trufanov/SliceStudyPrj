#include <cstddef>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include <cmath>
#include "bmpBaseLib.hpp"

// Нормали в проекте идут наружу при обходе против часовой стрелки

// Структура точки
// Хранит x и y во float
struct TPoint {
    float x, y;
    TPoint() : x(0), y(0) {};
    TPoint(float x, float y) : x(x), y(y) {};

    // Перегруженный оператор сравнения точек
    bool operator==(const TPoint &other) const {
        return ((x == other.x) and (y == other.y));
    }
};

// Структура линии
// Хранит две точки
// id следующей линии
struct TLine {
    TPoint pos1;
    TPoint pos2;

    std::size_t next = 0xFFFFFFFFFFFF;

    TLine() : pos1(TPoint()), pos2(TPoint()) {};
    TLine(TPoint pos1, TPoint pos2) : pos1(pos1), pos2(pos2) {};

};

// Соединяет родственные по id линии, находя их точку пересечения
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

// Генерирует отдельные линии на расстоянии наружу
// id следующей линии берётся у оригинала
void offsetLines(std::vector<TLine> &outline, std::vector<TLine> &slice, float distance) {
    for (const auto& line : outline) {
        float dx = line.pos2.x - line.pos1.x;
        float dy = line.pos2.y - line.pos1.y;
        float modD = std::sqrt((dx*dx)+(dy*dy));
        slice.emplace_back(TLine(TPoint(-dy/modD*distance+line.pos1.x, dx/modD*distance+line.pos1.y), TPoint(-dy/modD*distance+line.pos2.x, dx/modD*distance+line.pos2.y)));
        slice.back().next = line.next;
    }

    for (auto& line : slice) {
        float dx1 = line.pos2.x - line.pos1.x;
        float dy1 = line.pos2.y - line.pos1.y;
        float dx2 = slice[line.next].pos2.x - slice[line.next].pos1.x;
        float dy2 = slice[line.next].pos2.y - slice[line.next].pos1.y;

        float denominator = (dx1 * dy2) - (dy1 * dx2);

        if (denominator == 0) {
            throw std::runtime_error("Линии совпадают или параллельны");
        }

        float t = ((slice[line.next].pos1.x-line.pos1.x)*dy2-(slice[line.next].pos1.y-line.pos1.y)*dx2) / denominator;
        //float u = ((slice[line.next].pos1.x-line.pos1.x)*dy1-(slice[line.next].pos1.y-line.pos1.y)*dx1) / denominator;

        float nX = line.pos1.x + (t * dx1);
        float nY = line.pos1.y + (t * dy1);

        line.pos2.x = nX;
        line.pos2.y = nY;

        slice[line.next].pos1.x = nX;
        slice[line.next].pos1.y = nY;


    }
}

// Загрузка модели с файла
// Ничего сложного
void loadModel(std::vector<TLine> &outline) {
    std::ifstream f;
    f.open("outline.txt");
    if (f.is_open()) {

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
        }
        connectLines(outline);
    }
    for (std::size_t i = 0; i < outline.size(); i++) {
        std::cout << "Line " << i << ": x1: " << outline[i].pos1.x << " y1: " << outline[i].pos1.y << " x2: " << outline[i].pos2.x << " y2: " << outline[i].pos2.y << " | Next: " << outline[i].next << std::endl;
    }

    f.close();
}

// Нарисовать периметр серым цветом
void drawOutline(BMP &image, std::vector<TLine> &drawObject) {
    for (std::size_t i = 0; i < drawObject.size(); i++) {
        float floatdy = drawObject[i].pos2.y - drawObject[i].pos1.y;
        float floatdx = drawObject[i].pos2.x - drawObject[i].pos1.x;

        if (std::abs(std::round(floatdx)) == 0) {
            if (floatdy > 0) {
                unsigned ylinestart = std::abs(std::round(drawObject[i].pos1.y));
                unsigned ylineend = std::abs(std::round(drawObject[i].pos2.y));
                for (unsigned y = ylinestart; y < ylineend+1; y++) {
                    image.setByPos(std::abs(std::round(drawObject[i].pos1.x)), y, Color::GRAY);
                }
            } else {
                unsigned ylinestart = std::abs(std::round(drawObject[i].pos2.y));
                unsigned ylineend = std::abs(std::round(drawObject[i].pos1.y));
                for (unsigned y = ylinestart; y < ylineend+1; y++) {
                    image.setByPos(std::abs(std::round(drawObject[i].pos1.x)), y, Color::GRAY);
                }
            }
        } else
        if (std::abs(std::round(floatdx)) > std::abs(std::round(floatdy))) {
            if (floatdx > 0) {
                unsigned xlinestart = std::abs(std::round(drawObject[i].pos1.x));
                unsigned xlineend = std::abs(std::round(drawObject[i].pos2.x));
                for (unsigned x = xlinestart; x < xlineend+1; x++) {
                    image.setByPos(x, std::abs(std::round(drawObject[i].pos1.y+((floatdy*(x-xlinestart))/floatdx))), Color::GRAY);
                }
            } else {
                unsigned xlinestart = std::abs(std::round(drawObject[i].pos2.x));
                unsigned xlineend = std::abs(std::round(drawObject[i].pos1.x));
                for (unsigned x = xlinestart; x < xlineend+1; x++) {
                    int posY = std::abs(std::round(drawObject[i].pos2.y+((floatdy*(x-xlinestart))/floatdx)));
                    image.setByPos(x, posY, Color::GRAY);
                }
            }
        } else
        if (std::abs(std::round(floatdx)) < std::abs(std::round(floatdy))) {
            if (floatdy > 0) {
                unsigned ylinestart = std::abs(std::round(drawObject[i].pos1.y));
                unsigned ylineend = std::abs(std::round(drawObject[i].pos2.y));
                for (unsigned y = ylinestart; y < ylineend+1; y++) {
                    image.setByPos(std::abs(std::round(drawObject[i].pos1.x+((floatdx*(y-ylinestart))/floatdy))), y, Color::GRAY);
                }
            } else {
                unsigned ylinestart = std::abs(std::round(drawObject[i].pos2.y));
                unsigned ylineend = std::abs(std::round(drawObject[i].pos1.y));
                for (unsigned y = ylinestart; y < ylineend+1; y++) {
                    image.setByPos(std::abs(std::round(drawObject[i].pos2.x+((floatdx*(y-ylinestart))/floatdy))), y, Color::GRAY);
                }
            }
        }

    }
}

// Нарисовать периметр любым цветом
void drawOutline(BMP &image, std::vector<TLine> &drawObject, RGB color) {
    for (std::size_t i = 0; i < drawObject.size(); i++) {
        float floatdy = drawObject[i].pos2.y - drawObject[i].pos1.y;
        float floatdx = drawObject[i].pos2.x - drawObject[i].pos1.x;

        if (std::abs(std::round(floatdx)) == 0) {
            if (floatdy > 0) {
                unsigned ylinestart = std::abs(std::round(drawObject[i].pos1.y));
                unsigned ylineend = std::abs(std::round(drawObject[i].pos2.y));
                for (unsigned y = ylinestart; y < ylineend+1; y++) {
                    image.setByPos(std::abs(std::round(drawObject[i].pos1.x)), y, color);
                }
            } else {
                unsigned ylinestart = std::abs(std::round(drawObject[i].pos2.y));
                unsigned ylineend = std::abs(std::round(drawObject[i].pos1.y));
                for (unsigned y = ylinestart; y < ylineend+1; y++) {
                    image.setByPos(std::abs(std::round(drawObject[i].pos1.x)), y, color);
                }
            }
        } else
        if (std::abs(std::round(floatdx)) > std::abs(std::round(floatdy))) {
            if (floatdx > 0) {
                unsigned xlinestart = std::abs(std::round(drawObject[i].pos1.x));
                unsigned xlineend = std::abs(std::round(drawObject[i].pos2.x));
                for (unsigned x = xlinestart; x < xlineend+1; x++) {
                    image.setByPos(x, std::abs(std::round(drawObject[i].pos1.y+((floatdy*(x-xlinestart))/floatdx))), color);
                }
            } else {
                unsigned xlinestart = std::abs(std::round(drawObject[i].pos2.x));
                unsigned xlineend = std::abs(std::round(drawObject[i].pos1.x));
                for (unsigned x = xlinestart; x < xlineend+1; x++) {
                    int posY = std::abs(std::round(drawObject[i].pos2.y+((floatdy*(x-xlinestart))/floatdx)));
                    image.setByPos(x, posY, color);
                }
            }
        } else
        if (std::abs(std::round(floatdx)) < std::abs(std::round(floatdy))) {
            if (floatdy > 0) {
                unsigned ylinestart = std::abs(std::round(drawObject[i].pos1.y));
                unsigned ylineend = std::abs(std::round(drawObject[i].pos2.y));
                for (unsigned y = ylinestart; y < ylineend+1; y++) {
                    image.setByPos(std::abs(std::round(drawObject[i].pos1.x+((floatdx*(y-ylinestart))/floatdy))), y, color);
                }
            } else {
                unsigned ylinestart = std::abs(std::round(drawObject[i].pos2.y));
                unsigned ylineend = std::abs(std::round(drawObject[i].pos1.y));
                for (unsigned y = ylinestart; y < ylineend+1; y++) {
                    image.setByPos(std::abs(std::round(drawObject[i].pos2.x+((floatdx*(y-ylinestart))/floatdy))), y, color);
                }
            }
        }

    }
}

// Проверка самопересечений, вернёт true при таковом
bool checkSelfCollisions(std::vector<TLine> &outline) {
    for (auto &line : outline) {

    }
    return false;
}

int main() {

    std::vector<TLine> outline;
    std::vector<TLine> slice;
    std::vector<TLine> slice2;

    loadModel(outline);

    offsetLines(outline, slice, -2);
    offsetLines(slice, slice2, -2);

    BMP image(128, 128);

    drawOutline(image, outline);
    drawOutline(image, slice, Color::RED);
    drawOutline(image, slice2, Color::RED);

    image.save("result.bmp");

    return 0;
}

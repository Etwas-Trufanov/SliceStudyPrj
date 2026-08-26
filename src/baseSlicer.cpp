#include <algorithm>
#include <cstddef>
#include <exception>
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
struct TVector2 {
  float x, y;
  TVector2() : x(0), y(0) {};
  TVector2(float x, float y) : x(x), y(y) {};

  // Перегруженный оператор сравнения точек
  bool operator==(const TVector2 &other) const {
    return ((x == other.x) and (y == other.y));
  }
};


// Структура линии
// Хранит две точки
// id следующей линии
struct TLine {
  // Точки
  TVector2 pos1;
  TVector2 pos2;

  // Указатель на следующую линию
  std::size_t next = 0xFFFFFFFFFFFF;

  // Базовый конструктор - генерация на нулевых координатах
  TLine() : pos1(TVector2()), pos2(TVector2()) {};
  // Конструктор, принимает точки
  TLine(TVector2 pos1, TVector2 pos2) : pos1(pos1), pos2(pos2) {};

};

// Соединяет линии в периметр по совпадающим координатам
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
    slice.emplace_back(TLine(TVector2(-dy/modD*distance+line.pos1.x, dx/modD*distance+line.pos1.y), TVector2(-dy/modD*distance+line.pos2.x, dx/modD*distance+line.pos2.y)));
    slice.back().next = line.next;
  }

  for (auto& line : slice) {
    float dx1 = line.pos2.x - line.pos1.x;
    float dy1 = line.pos2.y - line.pos1.y;
    float dx2 = slice[line.next].pos2.x - slice[line.next].pos1.x;
    float dy2 = slice[line.next].pos2.y - slice[line.next].pos1.y;

    float denominator = (dx1 * dy2) - (dy1 * dx2);

    if (std::abs(denominator) < 1e-6f) {
      throw std::runtime_error("Линии совпадают или параллельны");
    }

    float t = ((slice[line.next].pos1.x-line.pos1.x)*dy2-(slice[line.next].pos1.y-line.pos1.y)*dx2) / denominator;
    //float u = ((slice[line.next].pos1.x-line.pos1.x)*dy1-(slice[line.next].pos1.y-line.pos1.y)*dx1) / denominator;

    float nX = line.pos1.x + (t * dx1);
    float nY = line.pos1.y + (t * dy1);

    //std::cout << "L1 pos2: " << line.pos2.x << " " << line.pos2.y << " L2 pos1: " << slice[line.next].pos1.x << " " << slice[line.next].pos1.y << std::endl;

    line.pos2.x = nX;
    line.pos2.y = nY;

    slice[line.next].pos1.x = nX;
    slice[line.next].pos1.y = nY;
    //std::cout << "New L1 pos2: " << line.pos2.x << " " << line.pos2.y << " L2 pos1: " << slice[line.next].pos1.x << " " << slice[line.next].pos1.y << std::endl;
  }
}

// Загрузка опорных периметров с файла
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

      outline.emplace_back(TVector2(pos1x, pos1y), TVector2(pos2x, pos2y));
    }
    connectLines(outline);
  }
  for (std::size_t i = 0; i < outline.size(); i++) {
    std::cout << "Линия " << i << ": x1: " << outline[i].pos1.x << " y1: " << outline[i].pos1.y << " x2: " << outline[i].pos2.x << " y2: " << outline[i].pos2.y << " | Next: " << outline[i].next << std::endl;
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


// Проверка периметра на самопересечение
// true - при наличии таковых
bool checkSelfCollisions(std::vector<TLine> &outline) {
  for (size_t i = 0; i < outline.size(); i++) {
    TLine &line1 = outline[i];

    for (size_t j = 0; j < outline.size(); j++) {
      TLine &line2 = outline[j];
      if (j == i) {
        continue;
      };
      if (line2.next == i) {
        continue;
      };
      if (line1.next == j) {
        continue;
      };

      float dx1 = line1.pos2.x - line1.pos1.x;
      float dy1 = line1.pos2.y - line1.pos1.y;
      float dx2 = line2.pos2.x - line2.pos1.x;
      float dy2 = line2.pos2.y - line2.pos1.y;

      float denominator = (dx1 * dy2) - (dy1 * dx2);
      if (std::abs(denominator) < 1e-9f)
        continue;
      float t = ((line2.pos1.x - line1.pos1.x) * dy2 -
                 (line2.pos1.y - line1.pos1.y) * dx2) /
                denominator;
      float u = ((line2.pos1.x - line1.pos1.x) * dy1 -
                 (line2.pos1.y - line1.pos1.y) * dx1) /
                denominator;

      bool result = ((0 <= t and t <= 1) and (0 <= u and u <= 1));

      if (result) {

        return true;
      };
    }
  }
  return false;
}

// Проверка всего слоя на самоколлизии
// true - при наличии таковых
bool checkSelfLayerCollision(std::vector<std::vector<TLine>> &layer) {
  for (size_t PerimeterCounter1 = 0; PerimeterCounter1 < layer.size();
       PerimeterCounter1++) {

    for (size_t LineCounter1 = 0;
         LineCounter1 < layer[PerimeterCounter1].size(); LineCounter1++) {
      auto &line1 = layer[PerimeterCounter1][LineCounter1];

      for (size_t PerimeterCounter2 = 0; PerimeterCounter2 < layer.size();
           PerimeterCounter2++) {

        for (size_t LineCounter2 = 0;
             LineCounter2 < layer[PerimeterCounter2].size(); LineCounter2++) {

          if (PerimeterCounter1 == PerimeterCounter2) {
            if (layer[PerimeterCounter2][LineCounter2].next == LineCounter1)
              continue;
            if (line1.next == LineCounter2)
              continue;
            if (LineCounter1 == LineCounter2) continue;
          }

          auto &line2 = layer[PerimeterCounter2][LineCounter2];

          // Проверка коллинеарности точек
          if ((std::abs(((line1.pos2.x - line1.pos1.x) *
                         (line2.pos1.y - line1.pos1.y)) -
                        ((line1.pos2.y - line1.pos1.y) *
                         (line2.pos1.x - line1.pos1.x))) <= 1e-9f) &&
              (std::abs(((line1.pos2.x - line1.pos1.x) *
                         (line2.pos2.y - line1.pos1.y)) -
                        ((line1.pos2.y - line1.pos1.y) *
                         (line2.pos2.x - line1.pos1.x))) <= 1e-9f)) {
            // Проверка на перекрытие
            if ((std::max(std::min(line1.pos1.x, line1.pos2.x), std::min(line2.pos1.x, line2.pos2.x)) <= std::min(std::max(line1.pos1.x, line1.pos2.x), std::max(line2.pos1.x, line2.pos2.x)))
              &&
                (std::max(std::min(line1.pos1.y, line1.pos2.y), std::min(line2.pos1.y, line2.pos2.y)) <= std::min(std::max(line1.pos1.y, line1.pos2.y), std::max(line2.pos1.y, line2.pos2.y)))) {
                  std::cout << "При сравнении " << LineCounter1 << " и " << LineCounter2 << " обнаружено наложение" << std::endl;
                  return true;
                }
          }

          float dx1 = line1.pos2.x - line1.pos1.x;
          float dy1 = line1.pos2.y - line1.pos1.y;
          float dx2 = line2.pos2.x - line2.pos1.x;
          float dy2 = line2.pos2.y - line2.pos1.y;

          float denominator = (dx1 * dy2) - (dy1 * dx2);
          if (std::abs(denominator) < 1e-9f)
            continue;
          float t = ((line2.pos1.x - line1.pos1.x) * dy2 -
                     (line2.pos1.y - line1.pos1.y) * dx2) /
                    denominator;
          float u = ((line2.pos1.x - line1.pos1.x) * dy1 -
                     (line2.pos1.y - line1.pos1.y) * dx1) /
                    denominator;

          bool result = ((0 <= t and t <= 1) and (0 <= u and u <= 1));

          if (result) {

            std::cout << "При сравнении " << LineCounter1 << " и " << LineCounter2 << " обнаружено пересечение" << std::endl;
            return true;
          };
        }
      }
    }
  }
  return false;
}

// Аргумент - растояние отступа на шаге
int main(int argc, char **argv) {

    int offsetsNum;
    if (argc == 2) {
        std::string offsetstring = argv[1];
        offsetsNum = std::stoi(offsetstring);
    } else {
        return 1;
    }
    std::vector<TLine> outline;
    std::vector<std::vector<TLine>> slice;

    loadModel(outline);

    for (int i = 1; i < offsetsNum+1; i++) {
        try {
            std::vector<TLine> tmp;
            offsetLines(outline, tmp, -i*5);
            slice.emplace_back(tmp);
            if (checkSelfLayerCollision(slice)) {slice.pop_back(); break;};

        } catch (const std::exception &e) {
            std::cout << e.what() << std::endl;
            break;
        }
    }

    BMP image(256, 256);

    drawOutline(image, outline);

    for (std::vector<TLine> perimeter : slice) {
        try {
        drawOutline(image, perimeter, Color::ORANGE);
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
        }
    }

    image.save("result.bmp");

    return 0;
}

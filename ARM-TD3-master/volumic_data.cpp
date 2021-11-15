#include "volumic_data.h"

#include <stdexcept>

#define range(value, min, max) value >= min && value < max 

VolumicData::VolumicData()
    : width(-1), height(-1), depth(-1), pixel_width(-1), pixel_height(-1),
      slice_spacing(0) {}

VolumicData::VolumicData(int W, int H, int D, double min, double max, double I)
    : data(W * H * D), width(W), height(H), depth(D), win_min(min), win_max(max), intercept(I) {}

VolumicData::VolumicData(const VolumicData &other)
    : data(other.data), width(other.width), height(other.height),
      depth(other.depth), pixel_width(other.pixel_width),
      pixel_height(other.pixel_height), slice_spacing(other.slice_spacing) {}

VolumicData::~VolumicData() {}

unsigned char VolumicData::getValue(int col, int row, int layer) {
  return data[col + row * width + layer * width * height];
}

QVector3D VolumicData::getCoordinate(int idx) {
  int x = idx % width;
  int y = (idx/width) % height;
  int z = idx / (width*height);
  return QVector3D(x, y, z); 
}

void VolumicData::setLayer(uint16_t *layer_data, int layer) {
  if (layer >= depth)
    throw std::out_of_range(
        "Layer " + std::to_string(layer) +
        " is outside of volume (depth=" + std::to_string(depth) + ")");
  int offset = width * height * layer;
  double offset_ = pow(2, 15) - intercept;
  for (int i = 0; i < width * height; i++) {
    data[i + offset] = layer_data[i] - offset_;
  }
}

double VolumicData::manualWindowHandling(double value) {
  if(value < win_min)  return 0;
  if(value > win_max)  return 1;

  return (value - win_min) / (win_max - win_min);
}

int VolumicData::threshold(double value, double min, double max, bool colorMode) {
  
  if (!colorMode) 
  {
    if(value <= max && value >= min)
      return 1;
  }
  else 
  {
    if(range(value, 200, 1024))         return 2; // Bone
    else if(range(value, 100, 200))     return 3; // Structures faiblement calcifiées
    else if(range(value, 37, 45))       return 4; // Matière grise
    else if(range(value, 20, 30))       return 5; // Matière blanche
    else if(range(value, -5, 15))       return 6; // Eau et liquides cérébro
    else if(range(value, -1024, -10))   return 7; // Graisses, poumons, air
  }
  return 0;
}

QVector3D VolumicData::getColorSegment(int segment, double c) {
  QVector3D color;
  switch (segment)
  {
    case 1: color = QVector3D(c, c, c); break;        // blanc
    case 2: color = QVector3D(1, 1, 1); break;       // blanc
    case 3: color = QVector3D(0.5, 0.5, 0.5); break;  // gris
    case 4: color = QVector3D(0, 1, 0); break;       // vert
    case 5: color = QVector3D(1, 0.7, 0); break;       // jaune
    case 6: color = QVector3D(0.2, 0.2, 1); break;   // bleu
    case 7: color = QVector3D(1, 0, 0); break;       // rouge
    default: color = QVector3D(0, 0, 0); break;
  }
  return color;
}
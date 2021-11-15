#ifndef VOLUMIC_DATA_H
#define VOLUMIC_DATA_H

#include <vector>
#include <cstdint>
#include <memory>
#include <iostream>
#include <cmath>

#include <QVector3D>

class VolumicData {
public:
  // The data from the volume stored:
  // - column by column
  // - line by line
  // - slice by slice
  std::vector<uint16_t> data;

  int width;
  int height;
  int depth;

  double pixel_width;
  double pixel_height;
  double slice_spacing;

  double win_min;
  double win_max;
  double intercept;

  // The data provided
  VolumicData();
  VolumicData(int width, int height, int depth, double win_min, double win_max, double intercept);
  VolumicData(const VolumicData &other);
  ~VolumicData();

  unsigned char getValue(int col, int row, int layer);

  void setLayer(uint16_t *layer_data, int layer);
  double manualWindowHandling(double value);
  int threshold(double value, double min, double max, bool colorMode);
  QVector3D getColorSegment(int segment, double c);
  QVector3D getCoordinate(int idx);

};

#endif // VOLUMIC_DATA_H

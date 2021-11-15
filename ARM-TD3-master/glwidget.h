#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QMatrix4x4>
#include <QOpenGLWidget>
#include <QString>

#include <memory>

#include "volumic_data.h"

class GLWidget : public QOpenGLWidget {
public:
  Q_OBJECT
public:
  enum ViewType { ORTHO, FRUSTUM };

  GLWidget(QWidget *parent = 0);
  ~GLWidget();
  QSize sizeHint() const { return QSize(200, 200); }

  float getAlpha() const;

  void updateVolumicData(std::unique_ptr<VolumicData> new_data);

  void setWinCenter(double new_value);
  void setWinWidth(double new_value);

  void updateDisplayPoints();

  bool contours_mode;
  bool highlight;
  bool hide_below;
  bool hide_above;
  int curr_slice;
  bool color_mode;

public slots:
  void setAlpha(double new_alpha);
  void onContoursModeChange(int state);
  void highlightActiveLayer(int state);
  void hideLayersAbove(int state);
  void hideLayersBelow(int state);
  void onColorModeChange(int state);
  void saveXYZ();

protected:
  struct DrawablePoint {
    QVector3D pos;
    QVector3D color;
    double a;
  };

  void initializeGL() override;
  void paintGL() override;


  void wheelEvent(QWheelEvent *event) override;

  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;

  /**
   * If 'shift' modifier is pressed, multiplies value by 10
   */
  double modifiedDelta(double delta);

  bool connectivity(const int mode, const int idx, const int curr_segment, const double min, const double max);
  void getWinMinMax(double* min, double* max);

  QPoint lastPos;
  float alpha;
  /**
   * Zoom value using a log scale
   * - positive is zoom in
   * - negative is zoom out
   */
  float log2_zoom;
  QMatrix4x4 transform;

  ViewType view_type;

  double win_center;
  double win_width;

  /// When enabled, all points with a drawing color = 0 are hidden
  bool hide_empty_points;

  /// The data of all the slices stored in a single object
  std::unique_ptr<VolumicData> volumic_data;

  /// The points to be drawn
  std::vector<DrawablePoint> display_points;
  
};

#endif // GLWIDGET_H

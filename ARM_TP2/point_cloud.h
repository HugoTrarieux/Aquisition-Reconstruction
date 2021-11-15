#include "dicom_data.h"
#include <QOpenGLWidget>

class PointCloud : public QOpenGLWidget{
  Q_OBJECT

public:
  PointCloud(DicomData *dicom_data, QWidget *parent = 0);
  ~PointCloud();

  void initializeGL();
  void paintGL();

private:
  DicomData *dicom_data;
};
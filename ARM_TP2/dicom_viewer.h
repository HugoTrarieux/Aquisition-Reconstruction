#ifndef DICOM_VIEWER_H
#define DICOM_VIEWER_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <vector>

#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmimgle/dcmimage.h"

#include "double_slider.h"
#include "image_label.h"
#include "point_cloud.h"

class DicomViewer : public QMainWindow {
  Q_OBJECT

public:
  DicomViewer(QWidget *parent = 0);
  ~DicomViewer();

public slots:
  void openDicom();
  void showStats();
  void save();

  void onWindowCenterChange(double new_window_center);
  void onWindowWidthChange(double new_window_width);
  void onNavigationChange(int new_navigation);

private:
  QWidget *widget;
  QGridLayout *layout;
  QWidget *navigation;
  QHBoxLayout *navigation_layout;
  DoubleSlider *window_center_slider;
  DoubleSlider *window_width_slider;
  QSlider *nav_slider;
  QLabel *nav_name_label;
  QLabel *nav_value_label;
  int number_files;
  PointCloud *point_cloud;
  /// The area in which the image is shown
  ImageLabel *img_label;

  /// The current DcmFileFormat
  DcmFileFormat active_file;
  std::vector<DcmFileFormat> files;

  /// The active Dicom image
  DicomImage *image;

  /// The 8-bits image to be shown on screen
  uchar *img_data;

  /// Retrieve access to the dataset of current file
  DcmDataset *getDataset();

  /// Initialise the Navigation slider by adding name, slider and values to the navigation layout
  void initNavigationSlider();

  /// Adjust the size of the window based on file content
  void updateWindowSliders();

  /// Load the DicomImage from the active slice
  /// If there are no active slice available, set image to nullptr
  void loadDicomImage();

  /// Import the default parameters from the DicomImage
  void applyDefaultWindow();

  /// Update the image based on current status of the object
  void updateImage();

  /// Retrieve patient name from active file
  /// return 'FAIL' if no active file is found
  std::string getPatientName();

  /// Retrieve image from active file, converting to appropriate transfer syntax
  /// return nullptr on failure
  DicomImage *getDicomImage();

  /// Convert current Dicom Image to a QImage according to actual parameters
  QImage getQImage();

  void getFrameMinMax(double *min_used_value, double *max_used_value);
  void getCollectionMinMax(double *min_used_value, double *max_used_value);
  /// Extract min (and max) used (and allowed) values
  void getMinMax(double *min_used_value, double *max_used_value,
                 double *min_allowed_value = nullptr,
                 double *max_allowed_value = nullptr);

  void getWindow(double *min_value, double *max_value);

  void getNumberFiles(int *nb_files);

  double getSlope();
  double getIntercept();

  double getWindowCenter();
  double getWindowWidth();
  double getWindowMin();
  double getWindowMax();

  std::string getInstanceNumber();
  std::string getAcquisitionNumber();
};

template <typename T> T getField(DcmItem *item, const DcmTagKey &tag_key);
template <typename T>
T getField(DcmItem *item, unsigned int g, unsigned int e) {
  return getField<T>(item, DcmTagKey(g, e));
}

template <> double getField<double>(DcmItem *item, const DcmTagKey &tag_key);
template <>
short int getField<short int>(DcmItem *item, const DcmTagKey &tag_key);
template <>
std::string getField<std::string>(DcmItem *item, const DcmTagKey &tag_key);

#endif // DICOM_VIEWER_H

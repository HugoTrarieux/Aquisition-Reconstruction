#include "dicom_viewer.h"

#include <iostream>
#include <set>

#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>

#include <dcmtk/dcmdata/dcrledrg.h>
#include <dcmtk/dcmjpeg/djdecode.h>

DicomViewer::DicomViewer(QWidget *parent)
    : QMainWindow(parent), image(nullptr), img_data(nullptr), pixel_width(-1),
      pixel_height(-1), slice_spacing(0),
      collection_min(std::numeric_limits<double>::max()),
      collection_max(std::numeric_limits<double>::lowest()) {
  // Setting layout
  widget = new QWidget();
  setCentralWidget(widget);
  img_label = new ImageLabel();
  img_label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
  layout = new QGridLayout();
  slice_slider = new IntSlider("Slice", 0, 0);
  alpha_slider = new DoubleSlider("Alpha", 0.0, 1.0);
  window_center_slider = new DoubleSlider("Window center", -1000.0, 1000.0);
  window_width_slider = new DoubleSlider("Window width", 1.0, 5000.0);
  gl_widget = new GLWidget();

  hide_2d_image = new CheckBox("test", "Hide 2D image");
  hide_3d_image = new CheckBox("test", "Hide 3D image");

  highlight_active_layer = new CheckBox("test", "Highlight active layer");
  hide_layers_below = new CheckBox("test", "Hide layers below");
  hide_layers_above = new CheckBox("test", "Hide layers above");

  contours_mode = new CheckBox("test", "Contours Mode");
  color_mode = new CheckBox("test", "Color Mode");
  
  layout->addWidget(alpha_slider, 0, 0, 1, 3);
  layout->addWidget(slice_slider, 1, 0, 1, 3);
  layout->addWidget(window_center_slider, 2, 0, 1, 3);
  layout->addWidget(window_width_slider, 3, 0, 1, 3);

  layout->addWidget(img_label, 4, 1, 7, 1);
  layout->addWidget(gl_widget, 4, 2, 7, 1);

  layout->addWidget(hide_2d_image, 4, 0, 1, 1);
  layout->addWidget(hide_3d_image, 5, 0, 1, 1);

  layout->addWidget(highlight_active_layer, 6, 0, 1, 1);
  layout->addWidget(hide_layers_above, 7, 0, 1, 1);
  layout->addWidget(hide_layers_below, 8, 0, 1, 1);

  layout->addWidget(contours_mode, 9, 0, 1, 1);
  layout->addWidget(color_mode, 10, 0, 1, 1);


  widget->setLayout(layout);
  // Setting menu
  QMenu *file_menu = menuBar()->addMenu("&File");
  QAction *open_collection_action = file_menu->addAction("&Open collection");
  open_collection_action->setShortcut(QKeySequence::Open);
  QObject::connect(open_collection_action, SIGNAL(triggered()), this,
                   SLOT(openDicomCollection()));
  QAction *save_action = file_menu->addAction("&Save");
  save_action->setShortcut(QKeySequence::Save);
  QObject::connect(save_action, SIGNAL(triggered()), this, SLOT(save()));

  QAction *saveXYZ_action = file_menu->addAction("&SaveXYZ");
  saveXYZ_action->setShortcut(QKeySequence::SaveAs);
  QObject::connect(saveXYZ_action, SIGNAL(triggered()), gl_widget, SLOT(saveXYZ()));

  QAction *help_action = file_menu->addAction("&Help");
  help_action->setShortcut(QKeySequence::HelpContents);
  QObject::connect(help_action, SIGNAL(triggered()), this, SLOT(showStats()));

  // Sliders connection
  connect(alpha_slider, SIGNAL(valueChanged(double)), gl_widget,
          SLOT(setAlpha(double)));
  connect(slice_slider, SIGNAL(valueChanged(int)), this,
          SLOT(onSliceChange(int)));
  connect(window_center_slider, SIGNAL(valueChanged(double)), this,
          SLOT(onWindowCenterChange(double)));
  connect(window_width_slider, SIGNAL(valueChanged(double)), this,
          SLOT(onWindowWidthChange(double)));

  //CheckBox connection
  connect(hide_2d_image, SIGNAL(stateChanged(int)), this,
          SLOT(on2dDisplayStateChange(int)));
  connect(hide_3d_image, SIGNAL(stateChanged(int)), this,
          SLOT(on3dDisplayStateChange(int)));
  connect(highlight_active_layer, SIGNAL(stateChanged(int)), gl_widget, 
          SLOT(highlightActiveLayer(int)));
  connect(hide_layers_below, SIGNAL(stateChanged(int)), gl_widget, 
          SLOT(hideLayersBelow(int)));
  connect(hide_layers_above, SIGNAL(stateChanged(int)), gl_widget, 
          SLOT(hideLayersAbove(int)));

  //Contour connection
  connect(contours_mode, SIGNAL(stateChanged(int)), gl_widget,
          SLOT(onContoursModeChange(int)));

  //Color connection
  connect(color_mode, SIGNAL(stateChanged(int)), gl_widget,
          SLOT(onColorModeChange(int)));

  // Codec registration
  DcmRLEDecoderRegistration::registerCodecs();
  DJDecoderRegistration::registerCodecs();

  // Update basic display elements
  updateInstanceLimits();
  updateSliceSlider();
  updateWindowSliders();

  // Importing alpha value from gl_widget
  alpha_slider->setValue(gl_widget->getAlpha());
}

DicomViewer::~DicomViewer() {}

void DicomViewer::openDicomCollection() {
  QStringList files = QFileDialog::getOpenFileNames(
      this, "Select files to open", "DICOM (*.dcm)");
  // If no file has been selected, don't change anything
  if (files.size() == 0)
    return;
  // TODO: this code section clearly exhibits that it would be way better to
  // encapsulate the DicomCollection in a class with several attributes, to load
  // it and then to have a collection = std::move(new_collection)

  // First reading all the collection properties in temporary variables to
  // avoid modification of the current data if provided files are invalid
  std::map<int, std::unique_ptr<DcmFileFormat>> new_files;
  std::string new_patient;
  double new_collection_min = std::numeric_limits<double>::max();
  double new_collection_max = std::numeric_limits<double>::lowest();
  double new_pixel_width(-1);
  double new_pixel_height(-1);
  for (int file_idx = 0; file_idx < files.size(); file_idx++) {
    const QString &file = files[file_idx];
    std::string path = file.toStdString();
    std::unique_ptr<DcmFileFormat> dcm_file(new DcmFileFormat());
    OFCondition status;
    status = dcm_file->loadFile(path.c_str());
    if (status.bad()) {
      QMessageBox::critical(this, "Failed to open file", path.c_str());
      return;
    }
    DcmDataset *file_ds = dcm_file->getDataset();
    // Checking patient
    std::string file_patient = getPatientName(file_ds);
    if (new_patient == "") {
      new_patient = file_patient;
    } else if (new_patient != file_patient) {
      std::string msg =
          "At least 2 patients are present in the file collection: '" +
          new_patient + "' and '" + file_patient + "'";
      QMessageBox::critical(this, "Invalid file collection", msg.c_str());
      return;
    }

    int instance_number = getInstanceNumber(file_ds);
    // Checking that instance number is not duplicated
    if (new_files.count(instance_number) > 0) {
      std::string msg = "Instance " + std::to_string(instance_number) +
                        " is already loaded, cancelling load";
      QMessageBox::critical(this, "Duplicated instance idx", msg.c_str());
      return;
    }
    // All the Dicom file should contain loadable images
    DicomImage *img = loadDicomImage(file_ds);
    if (img == nullptr) {
      QMessageBox::critical(this, "Invalid file",
                            ("Can't read image at file " + path).c_str());
      return;
    }
    // Updating min and max of collection
    double frame_min, frame_max;
    getFrameMinMax(img, &frame_min, &frame_max);
    new_collection_min = std::min(frame_min, new_collection_min);
    new_collection_max = std::max(frame_max, new_collection_max);
    new_files[instance_number] = std::move(dcm_file);
    // Updating/checking pixel_width
    std::vector<double> pixel_spacing = getPixelSpacing(file_ds);
    double frame_pixel_height = pixel_spacing[0];
    double frame_pixel_width = pixel_spacing[1];
    if (file_idx == 0) {
      new_pixel_width = frame_pixel_width;
      new_pixel_height = frame_pixel_height;
    } else if (new_pixel_width != frame_pixel_width ||
               new_pixel_height != frame_pixel_height) {
      std::ostringstream msg_oss;
      msg_oss << "Multiple pixel sizes found: " << new_pixel_width << "*"
              << new_pixel_height << " and " << frame_pixel_width << "*"
              << frame_pixel_height;
      QMessageBox::critical(this, "Inconsistent collection",
                            msg_oss.str().c_str());
      return;
    }
  }
  // Check slice_spacing consistency
  double new_slice_offset(0);
  double new_slice_spacing(-1);
  if (new_files.size() <= 1) {
    new_slice_spacing = 0;
  } else {
    int new_min_instance = new_files.begin()->first;
    int new_max_instance = new_files.rbegin()->first;
    // Deducing layer spacing and offset from extremum layers
    std::vector<double> first_layer_position =
        getImagePosition(new_files.begin()->second->getDataset());
    std::vector<double> last_layer_position =
        getImagePosition(new_files.rbegin()->second->getDataset());
    new_slice_spacing = (last_layer_position[2] - first_layer_position[2]) /
                        (new_max_instance - new_min_instance);
    new_slice_offset =
        first_layer_position[2] - new_min_instance * new_slice_spacing;
    // Checking that all layers roughly respect the provided their expected
    // position
    double max_tol = 0.01; //[mm]
    for (const auto &entry : new_files) {
      double expected_z = new_slice_spacing * entry.first + new_slice_offset;
      double received_z = getImagePosition(entry.second->getDataset())[2];
      double error_z = fabs(expected_z - received_z);
      if (error_z > max_tol) {
        std::string msg = "Slices are not regularly spaced, error: " +
                          std::to_string(error_z);
        QMessageBox::critical(this, "Inconsistent collection", msg.c_str());
        return;
      }
    }
  }

  // Replacing current elements
  active_files.clear();
  for (auto &entry : new_files) {
    active_files[entry.first] = std::move(entry.second);
  }
  patient_name = new_patient;
  collection_min = new_collection_min;
  collection_max = new_collection_max;
  pixel_height = new_pixel_height;
  pixel_width = new_pixel_width;
  slice_spacing = new_slice_spacing;

  // Updating all the internal members based on the new data
  updateInstanceLimits();
  int expected_instances = max_instance - min_instance + 1;
  if (active_files.size() != (size_t)expected_instances) {
    std::string msg = "Expecting " + std::to_string(expected_instances) +
                      " instances, received " +
                      std::to_string(active_files.size()) + " instances";
    QMessageBox::warning(this, "Missing instances", msg.c_str());
  }
  updateSliceSlider();
  loadDicomImage();
  updateWindowSliders();
  applyDefaultWindow();
  updateImage();
  updateVolumicData();
}

void DicomViewer::save() {
  QString fileName = QFileDialog::getSaveFileName(
      this, tr("Save image to: "), "tmp.png", tr("Images (*.png *.xpm *.jpg)"));
  if (!getQImage().save(fileName))
    QMessageBox::critical(this, "Failed to save file", fileName);
}

void DicomViewer::showStats() {
  std::string html_endl("<br>");
  std::ostringstream msg_oss;
  msg_oss << "<h1>Collection Properties</h1>";
  msg_oss << "Patient: " << patient_name << html_endl;
  msg_oss << "Nb loaded slices: " << active_files.size() << html_endl;
  msg_oss << "Values used: [" << collection_min << "," << collection_max << "]"
          << html_endl;
  msg_oss << "Pixel size: " << pixel_width << "*" << pixel_height << " [mm]"
          << html_endl;
  msg_oss << "Slices spacing: " << slice_spacing << " [mm]" << html_endl;
  msg_oss << html_endl;
  msg_oss << "<h1>Frame Properties</h1>";
  DcmDataset *ds = getDataset();
  if (ds != nullptr) {
    msg_oss << "Instance number: " << getInstanceNumber(ds) << html_endl;
    msg_oss << "Acquisition number: " << getAcquisitionNumber(ds) << html_endl;
    E_TransferSyntax original_syntax = ds->getOriginalXfer();
    DcmXfer xfer(original_syntax);
    msg_oss << "Original transfer syntax: (" << original_syntax << ") "
            << xfer.getXferName() << html_endl;

    std::vector<double> img_position = getImagePosition(ds);
    msg_oss << "Image position: [" << img_position[0] << "," << img_position[1]
            << "," << img_position[2] << "]" << html_endl;

    DicomImage *image = getDicomImage();
    if (image) {
      msg_oss << "Nb frames: " << image->getFrameCount() << html_endl;
      msg_oss << "Size: " << image->getWidth() << "*" << image->getHeight()
              << "*" << image->getDepth() << html_endl;
      double min_used_value, max_used_value, min_allowed_value,
          max_allowed_value;
      getMinMax(&min_used_value, &max_used_value, &min_allowed_value,
                &max_allowed_value);
      msg_oss << "Allowed values: [" << min_allowed_value << ", "
              << max_allowed_value << "]" << html_endl;
      msg_oss << "Used values: [" << min_used_value << ", " << max_used_value
              << "]" << html_endl;
      msg_oss << "Window: [" << getWindowMin() << ", " << getWindowMax() << "]"
              << html_endl;
      msg_oss << "Slope: " << getSlope() << html_endl;
      msg_oss << "Intercept: " << getIntercept() << html_endl;
    } else {
      msg_oss << "No available image" << html_endl;
    }
  } else {
    msg_oss << "No available Dataset for current frame" << html_endl;
  }
  QMessageBox::information(this, "DCM file properties", msg_oss.str().c_str());
}

void DicomViewer::onSliceChange(int new_slice) {
  (void)new_slice;
  gl_widget->curr_slice = new_slice;
  gl_widget->updateDisplayPoints();
  gl_widget->update();
  loadDicomImage();
  updateImage();
}

void DicomViewer::onWindowCenterChange(double new_window_center) {
  //(void)new_window_center;
  updateImage();
  gl_widget->setWinCenter(new_window_center);
}

void DicomViewer::onWindowWidthChange(double new_window_width) {
  //(void)new_window_width;
  updateImage();
  gl_widget->setWinWidth(new_window_width);
}

void DicomViewer::on2dDisplayStateChange(int state) {
  if (state >= 1)
    img_label->setVisible(false);
  else
    img_label->setVisible(true);
}

void DicomViewer::on3dDisplayStateChange(int state) {
  if (state >= 1)
    gl_widget->setVisible(false);
  else
    gl_widget->setVisible(true);

  updateVolumicData();
}

DcmDataset *DicomViewer::getDataset() {
  int idx = slice_slider->value();
  if (active_files.count(idx) == 0)
    return nullptr;
  return active_files.at(idx)->getDataset();
}

void DicomViewer::updateInstanceLimits() {
  min_instance = std::numeric_limits<int>::max();
  max_instance = std::numeric_limits<int>::lowest();
  for (const auto &entry : active_files) {
    if (entry.first < min_instance)
      min_instance = entry.first;
    if (entry.first > max_instance)
      max_instance = entry.first;
  }
}

void DicomViewer::updateSliceSlider() {
  slice_slider->setRange(min_instance, max_instance);
  slice_slider->setVisible(min_instance < max_instance);
}

void DicomViewer::updateWindowSliders() {
  if (active_files.size() == 0) {
    window_center_slider->setVisible(false);
    window_width_slider->setVisible(false);
    return;
  }
  window_center_slider->setVisible(true);
  window_width_slider->setVisible(true);
  // Choice is made to use collection minMax rather than frame minMax here to
  // make sure the slider is not changing each time we change the active layer
  window_center_slider->setLimits(collection_min, collection_max);
  window_width_slider->setLimits(1.0, collection_max - collection_min);
}
void DicomViewer::loadDicomImage() {
  if (image != nullptr)
    delete (image);
  image = loadDicomImage(getDataset());
}

DicomImage *DicomViewer::loadDicomImage(DcmDataset *dataset) {
  if (dataset == nullptr) {
    return nullptr;
  }
  // Changing syntax to a common one
  E_TransferSyntax wished_ts = EXS_LittleEndianExplicit;
  OFCondition status = dataset->chooseRepresentation(wished_ts, NULL);
  if (status.bad()) {
    QMessageBox::critical(this, "Dicom Image failure", status.text());
    return nullptr;
  }
  return new DicomImage(dataset, wished_ts);
}

void DicomViewer::applyDefaultWindow() {
  window_center_slider->setValue(getWindowCenter());
  window_width_slider->setValue(getWindowWidth());
}

void DicomViewer::updateImage() {
  if (image == nullptr) {
    img_label->setText("No available image");
    return;
  }
  // Set window
  double window_center = window_center_slider->value();
  double window_width = window_width_slider->value();
  image->setWindow(window_center, window_width);
  img_label->setImg(getQImage());
}

void DicomViewer::updateVolumicData() {
  // Building a buffer to store each layer data
  int buffer_size = image->getWidth() * image->getHeight();
  uint16_t buffer[buffer_size];
  // Getting default window used
  //double window_center = window_center_slider->value();
  //double window_width = window_width_slider->value();
  // Building a VolumicData object with appropriate dimensions
  std::unique_ptr<VolumicData> new_data(new VolumicData(image->getWidth(), image->getHeight(), max_instance - min_instance + 1, getWindowMin(), getWindowMax(), getIntercept()));

  for (const auto &entry : active_files) {
    DicomImage *dicom = loadDicomImage(entry.second->getDataset());
    //dicom->setWindow(window_center, window_width);
    dicom->setNoVoiTransformation();
    int bits_per_pixel = 16;
    int layer = entry.first;
    int status = dicom->getOutputData((void *)buffer, 2*buffer_size, bits_per_pixel);
    delete (dicom);
    if (!status) {
      QMessageBox::critical(this, "Failed update volumic data",
                            "getOutputData failed");
      continue;
    }

    new_data->setLayer(buffer, layer - min_instance);
  }
  new_data->pixel_width = pixel_width;
  new_data->pixel_height = pixel_height;
  new_data->slice_spacing = slice_spacing;
  gl_widget->updateVolumicData(std::move(new_data));
  gl_widget->update();
}

std::string DicomViewer::getPatientName(DcmDataset *ds) {
  return getField<std::string>(ds, DCM_PatientName);
}

DicomImage *DicomViewer::getDicomImage() { return image; }

QImage DicomViewer::getQImage() {
  DicomImage *dicom = getDicomImage();
  int bits_per_pixel = 8;
  unsigned long data_size = dicom->getOutputDataSize(bits_per_pixel);
  if (img_data)
    free(img_data);
  img_data = new unsigned char[data_size];
  int status =
      dicom->getOutputData((void *)img_data, data_size, bits_per_pixel);
  if (!status) {
    QMessageBox::critical(this, "Fatal error",
                          "Failed to get output data when getting QImage");
    return QImage();
  }
  int width = dicom->getWidth();
  int height = dicom->getHeight();

  return QImage(img_data, width, height, QImage::Format_Grayscale8);
}

std::vector<double> DicomViewer::getPixelSpacing(DcmDataset *dataset) {
  return getFieldVector<double>(dataset, DcmTagKey(0x28, 0x30), 2);
}

std::vector<double> DicomViewer::getImagePosition(DcmDataset *dataset) {
  return getFieldVector<double>(dataset, DcmTagKey(0x20, 0x32), 3);
}

void DicomViewer::getMinMax(double *min_used_value, double *max_used_value,
                            double *min_allowed_value,
                            double *max_allowed_value) {
  getDicomImage()->getMinMaxValues(*min_used_value, *max_used_value, 0);
  if (min_allowed_value != nullptr || max_allowed_value != nullptr) {
    double tmp_min, tmp_max;
    getDicomImage()->getMinMaxValues(tmp_min, tmp_max, 1);
    if (min_allowed_value)
      *min_allowed_value = tmp_min;
    if (max_allowed_value)
      *max_allowed_value = tmp_max;
  }
}

void DicomViewer::getFrameMinMax(DicomImage *img, double *min, double *max) {
  int used_values_mode = 0;
  img->getMinMaxValues(*min, *max, used_values_mode);
}

void DicomViewer::getCollectionMinMax(double *min, double *max) {
  *min = collection_min;
  *max = collection_max;
}

void DicomViewer::getWindow(double *min_value, double *max_value) {
  double center(0), width(0);
  getDicomImage()->getWindow(center, width);
  *min_value = center - width / 2;
  *max_value = center + width / 2;
}

void DicomViewer::loadJSONdata() {
  QString file = QFileDialog::getOpenFileName( this, "Select files to open", "JSON (*.json)");
  // If no file has been selected, don't change anything
  if (file.isEmpty())
    return;
}

double DicomViewer::getSlope() {
  return getField<double>(getDataset(), DcmTagKey(0x28, 0x1053));
}

double DicomViewer::getIntercept() {
  return getField<double>(getDataset(), DcmTagKey(0x28, 0x1052));
}

double DicomViewer::getWindowCenter() {
  return getField<double>(getDataset(), DcmTagKey(0x28, 0x1050));
}

double DicomViewer::getWindowWidth() {
  return getField<double>(getDataset(), DcmTagKey(0x28, 0x1051));
}

double DicomViewer::getWindowMin() {
  return getWindowCenter() - getWindowWidth() / 2;
}
double DicomViewer::getWindowMax() {
  return getWindowCenter() + getWindowWidth() / 2;
}

int DicomViewer::getSeriesNumber(DcmDataset *dataset) {
  return getField<int>(dataset, 0x20, 0x11);
}
int DicomViewer::getInstanceNumber(DcmDataset *dataset) {
  return getField<int>(dataset, 0x20, 0x13);
}
int DicomViewer::getAcquisitionNumber(DcmDataset *dataset) {
  return getField<int>(dataset, 0x20, 0x12);
}

template <>
double getField<double>(DcmItem *item, const DcmTagKey &tag_key,
                        unsigned long pos) {
  double value;
  OFCondition status = item->findAndGetFloat64(tag_key, value, pos);
  if (status.bad())
    std::cerr << "Error on tag: " << tag_key << " -> " << status.text()
              << std::endl;
  return value;
}
template <>
short int getField<short int>(DcmItem *item, const DcmTagKey &tag_key,
                              unsigned long pos) {
  short int value;
  OFCondition status = item->findAndGetSint16(tag_key, value, pos);
  if (status.bad())
    std::cerr << "Error on tag: " << tag_key << " -> " << status.text()
              << std::endl;
  return value;
}
template <>
int getField<int>(DcmItem *item, const DcmTagKey &tag_key, unsigned long pos) {
  int value;
  OFCondition status = item->findAndGetSint32(tag_key, value, pos);
  if (status.bad())
    std::cerr << "Error on tag: " << tag_key << " -> " << status.text()
              << std::endl;
  return value;
}
template <>
std::string getField<std::string>(DcmItem *item, const DcmTagKey &tag_key,
                                  unsigned long pos) {
  OFString value;
  OFCondition status = item->findAndGetOFStringArray(tag_key, value, pos);
  if (status.bad())
    std::cerr << "Error on tag: " << tag_key << " -> " << status.text()
              << std::endl;
  return value.c_str();
}

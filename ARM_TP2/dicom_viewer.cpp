#include "dicom_viewer.h"

#include <iostream>

#include <QFileDialog>
#include <QGridLayout> 
#include <QMenuBar>
#include <QMessageBox>

#include "dcmtk/dcmdata/dcrledrg.h"
#include "dcmtk/dcmjpeg/djdecode.h"

DicomViewer::DicomViewer(QWidget *parent)
    : QMainWindow(parent), image(nullptr), img_data(nullptr) {
  // Setting layout
  widget = new QWidget();
  setCentralWidget(widget);
  img_label = new ImageLabel(this);
  img_label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
  img_label->setMinimumSize(200,200);
  img_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  layout = new QGridLayout();

  navigation = new QWidget();
  navigation->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
  navigation_layout = new QHBoxLayout(navigation);
  navigation_layout->setAlignment(Qt::AlignTop);
  // Using default values for limits, they are updated anyway once a file is loaded
  window_center_slider = new DoubleSlider("Window center", -1000.0, 1000.0);
  window_width_slider = new DoubleSlider("Window width", 1.0, 5000.0);

  layout->addWidget(window_center_slider, 0, 0, 1, 2);
  layout->addWidget(window_width_slider, 1, 0, 1, 2);
  layout->addWidget(img_label, 2, 0, 1, 1);
  layout->addWidget(navigation, 3, 0, 1, 2);
  widget->setLayout(layout);
  //layout->insertLayout(2, navigation_layout, 0);

  // Setting menu
  QMenu *file_menu = menuBar()->addMenu("&File");
  QAction *open_action = file_menu->addAction("&Open");
  open_action->setShortcut(QKeySequence::Open);
  QObject::connect(open_action, SIGNAL(triggered()), this, SLOT(openDicom()));
  QAction *save_action = file_menu->addAction("&Save");
  save_action->setShortcut(QKeySequence::Save);
  QObject::connect(save_action, SIGNAL(triggered()), this, SLOT(save()));
  QAction *help_action = file_menu->addAction("&Help");
  help_action->setShortcut(QKeySequence::HelpContents);
  QObject::connect(help_action, SIGNAL(triggered()), this, SLOT(showStats()));
  // Sliders connection
  connect(window_center_slider, SIGNAL(valueChanged(double)), this, SLOT(onWindowCenterChange(double)));
  connect(window_width_slider, SIGNAL(valueChanged(double)), this, SLOT(onWindowWidthChange(double)));
  DcmRLEDecoderRegistration::registerCodecs();
  DJDecoderRegistration::registerCodecs();
}

DicomViewer::~DicomViewer() {}

void DicomViewer::openDicom() {
  QStringList fileNames = QFileDialog::getOpenFileNames(this, "Select one file to open", ".", "DICOM (*.dcm)");
  number_files = fileNames.size() - 1;
  std::string patientName;
  int maxInstanceNumber;
  int currInstanceNumber;
  std::vector<int> listInstanceNumber;

  std::string path = fileNames[number_files].toStdString();
  OFCondition status;
  status = active_file.loadFile(path.c_str());
  if (status.bad()) {
    QMessageBox::critical(this, "Failed to open file", path.c_str());
    return;
  }
  patientName = getPatientName();
  currInstanceNumber = std::stoi(getInstanceNumber());
  maxInstanceNumber = currInstanceNumber;
  listInstanceNumber.push_back(currInstanceNumber);

  files.push_back(active_file);
  loadDicomImage();

  for (int i = number_files-1; i >= 0; i--) {
    std::string path = fileNames[i].toStdString();
    OFCondition status;
    status = active_file.loadFile(path.c_str());
    if (status.bad()) {
      QMessageBox::critical(this, "Failed to open file", path.c_str());
      return;
    }
    if(getPatientName().compare(patientName) != 0){
      QMessageBox::warning(this, "Warning", "Files are not from the same patient");
    }
    currInstanceNumber = std::stoi(getInstanceNumber());
    for(auto i = listInstanceNumber.begin(); i!=listInstanceNumber.end(); i++){
      if(currInstanceNumber == *i){
        QMessageBox::warning(this, "Warning", "A file has been duplicated");
        number_files--;   //Pour rectifier le calcul des fichiers manquant
      }
    }
    listInstanceNumber.push_back(currInstanceNumber);
    if(currInstanceNumber > maxInstanceNumber){
      maxInstanceNumber = currInstanceNumber;
    }
    files.push_back(active_file);
  }

  if(maxInstanceNumber != number_files+1){
      QMessageBox::warning(this, "Warning" ,"Some files are missing");
  }
  if(number_files > 1){
    if(navigation_layout->isEmpty() != true){
      delete nav_name_label;
      delete nav_slider;
      delete nav_value_label;
    }
    initNavigationSlider();

    DicomData *dicom_data = new DicomData(files, number_files);
    dicom_data->extractImages(number_files);
    QWidget *parent = new QWidget();
    QOpenGLWidget *openGlWidget = new QOpenGLWidget(this);
    point_cloud = new PointCloud(dicom_data, openGlWidget);
    layout->addWidget(point_cloud, 2, 1, 1, 1);
    point_cloud->initializeGL();
    point_cloud->paintGL();
  }

  updateWindowSliders();
  applyDefaultWindow();
  updateImage();
}

void DicomViewer::initNavigationSlider() {
  nav_name_label = new QLabel("Slice");

  nav_slider = new QSlider(Qt::Horizontal);
  nav_slider->setMinimum(0);
  nav_slider->setMaximum(number_files);
  nav_slider->setValue(0);

  std::ostringstream oss;
  oss << " [ 0, " << number_files << "]";
  nav_value_label = new QLabel();
  nav_value_label->setText(QString(oss.str().c_str()));

  navigation_layout->addWidget(nav_name_label);
  navigation_layout->addWidget(nav_slider);
  navigation_layout->addWidget(nav_value_label);
  connect(nav_slider, SIGNAL(valueChanged(int)), this, SLOT(onNavigationChange(int)));
}

void DicomViewer::save() {
  QString fileName = QFileDialog::getSaveFileName(
      this, tr("Save image to: "), "tmp.png", tr("Images (*.png *.xpm *.jpg)"));
  if (!getQImage().save(fileName))
    QMessageBox::critical(this, "Failed to save file", fileName);
}

void DicomViewer::showStats() {
  std::ostringstream msg_oss;
  msg_oss << "Patient: " << getPatientName() << std::endl;
  msg_oss << "Instance number: " << getInstanceNumber() << std::endl;
  msg_oss << "Acquisition number: " << getAcquisitionNumber() << std::endl;
  E_TransferSyntax original_syntax = getDataset()->getOriginalXfer();
  DcmXfer xfer(original_syntax);
  msg_oss << "Original transfer syntax: (" << original_syntax << ") " << xfer.getXferName() << std::endl;
  DicomImage *image = getDicomImage();
  if (image) {
    msg_oss << "Nb frames: " << image->getFrameCount() << std::endl;
    msg_oss << "Size: " << image->getWidth() << "*" << image->getHeight() << "*" << image->getDepth() << std::endl;
    double min_used_value, max_used_value, min_allowed_value, max_allowed_value;
    getMinMax(&min_used_value, &max_used_value, &min_allowed_value, &max_allowed_value);
    msg_oss << "Allowed values: [" << min_allowed_value << ", " << max_allowed_value << "]" << std::endl;
    msg_oss << "Used values: [" << min_used_value << ", " << max_used_value << "]" << std::endl;
    msg_oss << "Window: [" << getWindowMin() << ", " << getWindowMax() << "]" << std::endl;
    msg_oss << "Slope: " << getSlope() << std::endl;
    msg_oss << "Intercept: " << getIntercept() << std::endl;
    double min, max;
    getCollectionMinMax(&min, &max);
    msg_oss << "Used values collection: [" << min << ", " << max << "]" << std::endl;

  } else {
    msg_oss << "No available image" << std::endl;
  }
  QMessageBox::information(this, "DCM file properties", msg_oss.str().c_str());
}

void DicomViewer::onWindowCenterChange(double new_window_center) {
  (void)new_window_center;
  updateImage();
}
void DicomViewer::onWindowWidthChange(double new_window_width) {
  (void)new_window_width;
  updateImage();
}

void DicomViewer::onNavigationChange(int new_navigation) {
  (void)new_navigation;
  updateImage();
}

DcmDataset *DicomViewer::getDataset() { return active_file.getDataset(); }

void DicomViewer::updateWindowSliders() {
  double min_used_value, max_used_value;
  getMinMax(&min_used_value, &max_used_value);
  int number_files;
  getNumberFiles(&number_files);
  window_center_slider->setLimits(min_used_value, max_used_value);
  window_width_slider->setLimits(1.0, max_used_value - min_used_value);
}

void DicomViewer::loadDicomImage() {
  DcmDataset *dataset = getDataset();
  if (dataset == nullptr) {
    image = nullptr;
    return;
  }
  // Changing syntax to a common one
  E_TransferSyntax wished_ts = EXS_LittleEndianExplicit;
  OFCondition status = getDataset()->chooseRepresentation(wished_ts, NULL);
  if (status.bad()) {
    QMessageBox::critical(this, "Dicom Image failure", status.text());
    return;
  }
  if (image != nullptr)
    delete (image);
  image = new DicomImage(getDataset(), wished_ts);
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

  if(number_files > 0) {
    int selected_file = nav_slider->value();
    for (auto i = files.begin(); i != files.end(); ++i) {
      active_file = *i;
      if(std::stoi(getInstanceNumber()) == selected_file){
        loadDicomImage();
      }
    }
    std::ostringstream oss;
    oss << selected_file << " [ 0, " << number_files << "]";
    nav_value_label->setText(QString(oss.str().c_str()));
  }

  image->setWindow(window_center, window_width);
  img_label->setImg(getQImage());
}

std::string DicomViewer::getPatientName() {
  return getField<std::string>(getDataset(), DCM_PatientName);
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

void DicomViewer::getFrameMinMax(double *min_used_value, double *max_used_value) {
  getDicomImage()->getMinMaxValues(*min_used_value, *max_used_value, 0);
}

void DicomViewer::getCollectionMinMax(double *min_used_value, double *max_used_value) {
  double curr_min, curr_max;
  double min, max;
  min = 1000;
  max = 0;
  DcmFileFormat curr_file = active_file;

  for (auto i = files.begin(); i != files.end(); ++i){
    active_file = *i;
    loadDicomImage();
    getFrameMinMax(&curr_min, &curr_max);
    if(curr_min < min)
      min = curr_min;
    if(curr_max > max)
      max = curr_max;
  }
  *min_used_value = min;
  *max_used_value = max;
  active_file = curr_file;
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

void DicomViewer::getWindow(double *min_value, double *max_value) {
  double center(0), width(0);
  getDicomImage()->getWindow(center, width);
  *min_value = center - width / 2;
  *max_value = center + width / 2;
}

void DicomViewer::getNumberFiles(int *nb_files) {
  *nb_files = number_files;
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

std::string DicomViewer::getInstanceNumber() {
  return getField<std::string>(getDataset(), 0x20, 0x13);
}
std::string DicomViewer::getAcquisitionNumber() {
  return getField<std::string>(getDataset(), 0x20, 0x12);
}

template <> double getField<double>(DcmItem *item, const DcmTagKey &tag_key) {
  double value;
  OFCondition status = item->findAndGetFloat64(tag_key, value);
  if (status.bad())
    std::cerr << "Error on tag: " << tag_key << " -> " << status.text() << std::endl;
  return value;
}
template <>
short int getField<short int>(DcmItem *item, const DcmTagKey &tag_key) {
  short int value;
  OFCondition status = item->findAndGetSint16(tag_key, value);
  if (status.bad())
    std::cerr << "Error on tag: " << tag_key << " -> " << status.text() << std::endl;
  return value;
}
template <>
std::string getField<std::string>(DcmItem *item, const DcmTagKey &tag_key) {
  OFString value;
  OFCondition status = item->findAndGetOFStringArray(tag_key, value);
  if (status.bad())
    std::cerr << "Error on tag: " << tag_key << " -> " << status.text() << std::endl;
  return value.c_str();
}

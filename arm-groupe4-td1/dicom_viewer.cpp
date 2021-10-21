#include "dicom_viewer.h"

#include <iostream>
#include <vector>

#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QImage>
#include <QPixmap>
#include <QLabel>

#include <dcmtk/dcmimgle/dcmimage.h>

DicomViewer::DicomViewer(QWidget *parent) : QMainWindow(parent), imageLabel(new QLabel) {
  setCentralWidget(imageLabel);

  // Setting menu
  QMenu *file_menu = menuBar()->addMenu("&File");
  QAction *open_action = file_menu->addAction("&Open");
  open_action->setShortcut(QKeySequence::Open);
  QObject::connect(open_action, SIGNAL(triggered()), this, SLOT(openDicom()));
  QAction *help_action = file_menu->addAction("&Help");
  help_action->setShortcut(QKeySequence::HelpContents);
  QObject::connect(help_action, SIGNAL(triggered()), this, SLOT(showStats()));
}

DicomViewer::~DicomViewer() {}

void DicomViewer::loadDicomFile(QString fileName)
{
  OFCondition status = currentFileFormat.loadFile(fileName.toStdString().c_str());
  if(!status.good())
    {
    std::cerr << "Error: cannot read DICOM file (" << status.text() << ")" << std::endl;
    }

  dicomImage = new DicomImage(fileName.toStdString().c_str());
  if(dicomImage == NULL || dicomImage->getStatus() != EIS_Normal){
    std::cerr << "Error: Input not a DICOM image or did not load properly. Image status: " << dicomImage ->getStatus() << std::endl;
    return;
  }
}

void DicomViewer::transferDicomImageToImageLabel(){
  dicomImage->setMinMaxWindow();
  Uint8 * pixelData = (Uint8 *) (dicomImage->getOutputData(8));
  if (pixelData != NULL){
    QImage qImage(pixelData, dicomImage->getWidth(), dicomImage->getHeight(), QImage::Format_Indexed8);
    imageLabel->setPixmap(QPixmap::fromImage(qImage));
  }
}

void DicomViewer::openDicom() {
  QString fileName = QFileDialog::getOpenFileName(this,
    tr("Open DICOM file"), "", tr("DICOM file (*.dcm)"));
  DJDecoderRegistration::registerCodecs();
  DcmRLEDecoderRegistration::registerCodecs();
  loadDicomFile(fileName);
  transferDicomImageToImageLabel();
}

std::vector<std::tuple<DcmTagKey,OFString>> DicomViewer::constructTagKeyMessageTupleVector()
{
  return
    {
      std::make_tuple(DCM_PatientName, "Patient: "),
      std::make_tuple(DCM_InstanceNumber, "Instance number: "),
      std::make_tuple(DCM_AcquisitionNumber, "Acquisition number: "),
      std::make_tuple(DCM_RescaleSlope, "Slope: "),
      std::make_tuple(DCM_RescaleIntercept, "Intercept: "),
    };
}

DcmTagKey DicomViewer::getTagKeyInTuple(std::tuple<DcmTagKey, OFString> tuple)
{
  return std::get<0>(tuple);
}

OFString DicomViewer::getStringInTuple(std::tuple<DcmTagKey, OFString> tuple)
{
  return std::get<1>(tuple);
}

void DicomViewer::printErrorMessage(std::string tagNameNotFound, std::ostringstream * msg_oss){
  * msg_oss << tagNameNotFound << "NOT FOUND" << std::endl;
  std::cerr << "Error: " << tagNameNotFound << std::endl;
}

void DicomViewer::addTransferSyntaxToMsg(std::ostringstream * msg_oss)
{
  * msg_oss << "Original transfer syntax: " << currentFileFormat.getMetaInfo()->getOriginalXfer() << std::endl;
}

void DicomViewer::addNumberOfFramesToMsg(std::ostringstream * msg_oss)
{
  * msg_oss << "Number of frames: " << dicomImage->getNumberOfFrames() << std::endl;
}

void DicomViewer::addAllowedValuesToMsg(std::ostringstream * msg_oss)
{
  double min, max;
  if(dicomImage->getMinMaxValues(min, max, 1) != true)
  {
    printErrorMessage("Allow Values", msg_oss);
  } else
    {
      * msg_oss << "Allowed Values: [" << min << ", " << max << "]" << std::endl;
    }
}

void DicomViewer::addUsedValuesToMsg(std::ostringstream * msg_oss)
{
  double min, max;
  if(dicomImage->getMinMaxValues(min, max, 0) != true)
  {
    printErrorMessage("Used Values", msg_oss);
  } else
    {
      * msg_oss << "Used Values: [" << min << ", " << max << "]" << std::endl;
    }
}

void DicomViewer::addSizeToMsg(std::ostringstream * msg_oss)
{
  * msg_oss << "Size: " << dicomImage->getWidth() << " x " << dicomImage->getHeight() << " x " << dicomImage->getDepth() << std::endl;
}

void DicomViewer::addWindowToMsg(std::ostringstream * msg_oss)
{
  double center, width;
  if(dicomImage->getWindow(center, width) == false)
  {
    printErrorMessage("Window", msg_oss);
  } else
    {
      * msg_oss << "Window: " << "[center: " << center << ", width: " << width << "]" << std::endl;
    }
}

void DicomViewer::constructMsg(std::ostringstream * msg_oss)
{
  std::vector<std::tuple<DcmTagKey,OFString>> tagKeysAndMessageTupleVector = constructTagKeyMessageTupleVector();
  OFString currentValue;
  for (std::tuple<DcmTagKey,OFString> currentTuple : tagKeysAndMessageTupleVector)
    {
      DcmTagKey currentTagKey = getTagKeyInTuple(currentTuple);
      OFString currentOFString = getStringInTuple(currentTuple);

      if(currentFileFormat.getDataset()->findAndGetOFString(currentTagKey, currentValue).good())
        *msg_oss << currentOFString << currentValue << std::endl;
      else
        {
         * msg_oss << currentOFString << "NOT FOUND" << std::endl;
          std::cerr << "Error: " << currentOFString << std::endl;
        }        
    }
    addTransferSyntaxToMsg(msg_oss);
    addNumberOfFramesToMsg(msg_oss);
    addAllowedValuesToMsg(msg_oss);
    addUsedValuesToMsg(msg_oss);
    addSizeToMsg(msg_oss);
    addWindowToMsg(msg_oss);
}

void DicomViewer::showStats()
{
  std::ostringstream msg_oss;
  constructMsg(&msg_oss);
  QMessageBox::information(this, "DCM file properties", msg_oss.str().c_str());
}


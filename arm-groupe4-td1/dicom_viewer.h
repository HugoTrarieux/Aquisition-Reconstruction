#ifndef DICOM_VIEWER_H
#define DICOM_VIEWER_H

#include <iostream>
#include <vector>

#include <QMainWindow>
#include <QVBoxLayout>
#include <QLabel>

#include "dcmtk/dcmdata/dcrledrg.h"
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmimgle/dcmimage.h"
#include "dcmtk/dcmjpeg/djdecode.h"

class DicomViewer : public QMainWindow {
  Q_OBJECT

public:
  DicomViewer(QWidget *parent = 0);
  ~DicomViewer();

private:
  DcmFileFormat currentFileFormat;
  DicomImage *dicomImage;
  QLabel *imageLabel;

  void loadDicomFile(QString fileName);
  void transferDicomImageToImageLabel();
  DcmTagKey getTagKeyInTuple(std::tuple<DcmTagKey, OFString> tuple);
  OFString getStringInTuple(std::tuple<DcmTagKey, OFString> tuple);
  std::vector<std::tuple<DcmTagKey,OFString>> constructTagKeyMessageTupleVector();
  void printErrorMessage(std::string, std::ostringstream * msg_oss);
  void addTransferSyntaxToMsg(std::ostringstream * msg_oss);
  void addNumberOfFramesToMsg(std::ostringstream * msg_oss);
  void addAllowedValuesToMsg(std::ostringstream * msg_oss);
  void addUsedValuesToMsg(std::ostringstream * msg_oss);
  void addSizeToMsg(std::ostringstream * msg_oss);
  void addWindowToMsg(std::ostringstream * msg_oss);
  void constructMsg(std::ostringstream * msg_oss);


public slots:
  void openDicom();
  void showStats();
};

#endif // DICOM_VIEWER_H

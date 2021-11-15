#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmimgle/dcmimage.h"
#include <vector>

class DicomData {

public:
    DicomData(std::vector<DcmFileFormat> files, int nbFiles);

    void extractImages(int size);

    unsigned char* getData();
    int getWidth();
    int getHeight();
    int getNbFiles();

private:
    int width, height;
    unsigned char *data;
    int nb_files;
    DicomImage **dicom_image;
};
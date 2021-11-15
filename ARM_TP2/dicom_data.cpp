#include "dicom_data.h"

DicomData::DicomData(std::vector<DcmFileFormat> files, int nbFiles){
    this->width = width;
    this->height = height;
    DicomImage *dcmI;
    int j = 0;
    dicom_image = (DicomImage**) calloc(nbFiles+1, sizeof(DicomImage*));
    for(auto i = files.begin(); i != files.end(); ++i){
        DcmFileFormat file = *i;
        dcmI = new DicomImage(file.getDataset(), EXS_LittleEndianExplicit);
        dicom_image[j] = dcmI;
        j++;
    }
    data = (unsigned char*) calloc(dcmI->getWidth()*dcmI->getHeight()*nbFiles, sizeof(unsigned char));
    this->nb_files = nbFiles;
}

void DicomData::extractImages(int nb_images){
    DicomImage *dcmI;
    unsigned char *data_curr_file;
    int bits_per_pixel = 8;
    unsigned long data_size;
    for(int i=0; i<nb_images; i++){
        dcmI = dicom_image[i];
        data_size = dcmI->getOutputDataSize(bits_per_pixel);
        data_curr_file = new unsigned char[data_size];
        data_curr_file = (unsigned char*) dcmI->getOutputData(8, 0, 0);
        dcmI->getOutputData((void *)data_curr_file, data_size, bits_per_pixel);
        for(int j=i*data_size; j<i*data_size+data_size; j++){
            this->data[i] = data_curr_file[i];
        }
    }
}

unsigned char* DicomData::getData(){return this->data;}
int DicomData::getWidth(){return this->width;}
int DicomData::getHeight(){return this->height;}
int DicomData::getNbFiles(){return this->nb_files;}
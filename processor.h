/**
 * Class which processes an image using opencv
 */

#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <opencv2/opencv.hpp>

#include "processingvars.h"
#include "putility.h"
#include "dbmanager.h"


class Processor
{
private:
    // Fields
    DBManager *dbmanager; // reads and writes database .txt files
    cv::Mat img; // the image being processed

    /* various features of all the regions */
    std::vector<cv::Mat> regions;
    std::vector<cv::Point> centroids;
    std::vector<double> squarenesses;
    std::vector<double> hu1s;
    std::vector<double> hu2s;
    std::vector<double> hu3s;
    std::vector<double> hu4s;
    std::vector<cv::RotatedRect> bboxes;
    std::vector<double> relSizes;

    cv::Ptr<cv::ml::KNearest> knn;
    std::map<std::string, int> knn_object2labels; // map object names to corresponding labels (derived ints)
    std::map<int, std::string> knn_labels2objects; // for retrieval
    unsigned int lastknowndbsize; // last known size of db, so we don't have to retrain the knn all the time


    // Methods
    /* Various Stages of Processing which the Processor might perform in getImage(...) */
    void preprocess(const struct preVars &vars);
    void threshold(const struct threshVars &vars);
    void morphprocess(const struct morphVars &vars);
    void ccanalysis(const struct ccaVars &vars);
    void featurecompute(const struct featureVars &vars);
    std::string euclClassify(const std::vector<double> &featvector, double min_acceptable);
    std::string knnClassify(const std::vector<double> &featVector, unsigned int k);

    unsigned long int getArea(const cv::Mat &binaryImg); // add up the foreground in a binary image

public:
    Processor();
    ~Processor();

    // Fields
    /* this has a stage-dependent value and provides the correct conversion for constructing a QImage */
    cv::ColorConversionCodes recommendedConversion;
    std::multimap<std::string, std::vector<double>> *db; // the read-in database


    // Methods
    void setImage(const cv::Mat &original); // set current image to process
    cv::Mat getImage(int extent, // the extent to which the image should be processed
                     const struct preVars &preVars,
                     const struct threshVars &threshVars,
                     const struct morphVars &morphVars,
                     const struct ccaVars &ccaVars,
                     const struct featureVars &featureVars); // returns a processed image to be converted to a QImage for display
    std::vector<double> getFeatureVectors(int extent, unsigned int region);
    std::string classify(const std::vector<double> &featvector,
                         const struct classifierVars &classifierVars);
    void assignObjectName(const std::string &name, std::vector<double> featVector); // update the database with a label for a feature vector
    void changeDB(const std::string &newdb); // write the current database, then read in a new one

};


#endif // PROCESSOR_H

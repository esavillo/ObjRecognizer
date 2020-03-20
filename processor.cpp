#include "processor.h"

//#define DEBUG
#ifdef DEBUG
#define D if (true)
#else
#define D if (false)
#endif


Processor::Processor()
{
    dbmanager = new DBManager();

    dbmanager->readCSV("./../../../../proj03/testdb.txt");
    db = dbmanager->getDB();

    lastknowndbsize = 0;
}

Processor::~Processor()
{
    dbmanager->writeCSV();
    delete dbmanager;
}

void Processor::preprocess(const struct preVars &vars)
{
    if (vars.median) {
        cv::medianBlur(img, img, 13);
    }

    // anything better to do?
    if (vars.bilateral) {
        cv::bilateralFilter(img.clone(), img, 5, 100, 100, cv::BORDER_REFLECT_101);
    }

    // note: same conversion recommendation as the previous (raw) stage
}

// TODO - adpative thresholding?
void Processor::threshold(const struct threshVars &vars)
{
    if (vars.autothresh) { // just use OTSU method
        cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);
        cv::threshold(img, img, 0, 255, cv::THRESH_BINARY_INV + cv::THRESH_OTSU);
    } else {
        D LNPRINTLN("attempting threshold with values (r/g/b) "
                    << vars.rthresh << "/" << vars.gthresh << "/" << vars.bthresh);

        // Split the given image into B, G, R channels
        std::vector<cv::Mat> bgr;
        cv::split(img, bgr);

        // loop entirely unwound
        cv::threshold(bgr[0], bgr[0], vars.bthresh, 255, cv::THRESH_BINARY_INV);
        cv::threshold(bgr[1], bgr[1], vars.gthresh, 255, cv::THRESH_BINARY_INV);
        cv::threshold(bgr[2], bgr[2], vars.rthresh, 255, cv::THRESH_BINARY_INV);
        //                cv::THRESH_BINARY_INV + (vars.autothresh ? cv::THRESH_OTSU : 0));

        // Build up a mask based on all the channels
        cv::Mat mask = bgr[0].clone();
        mask &= bgr[1];
        mask &= bgr[2];

        // Update original image
        mask.copyTo(img);
    }

    D PRINTLN("threshold process finished");

    recommendedConversion = cv::COLOR_GRAY2RGB;
}

void Processor::morphprocess(const struct morphVars &vars)
{
    cv::Mat kernel = cv::getStructuringElement(vars.rect_cross ? cv::MORPH_RECT : cv::MORPH_CROSS,
                                               cv::Size(static_cast<int>(vars.kernelSize),
                                                        static_cast<int>(vars.kernelSize)));
    // depending on the setting, open-close or close-open.
    if (vars.open_close) {
        // For example, a moon of 3 will produce OCOOCCOOOCCC
        for (int i = 1; i <= static_cast<int>(vars.moons); i++) {
            cv::morphologyEx(img, img, cv::MORPH_OPEN, kernel, cv::Point(-1, -1), i);
            cv::morphologyEx(img, img, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), i);
        }
    } else {
        for (int i = 1; i <= static_cast<int>(vars.moons); i++) {
            cv::morphologyEx(img, img, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), i);
            cv::morphologyEx(img, img, cv::MORPH_OPEN, kernel, cv::Point(-1, -1), i);
        }
    }

    // note: same color conversion as the previous (threshold) stage
}

void Processor::ccanalysis(const struct ccaVars &vars)
{
    cv::Mat labels(img.size(), CV_32S);
    unsigned int numGroups = static_cast<unsigned int>(cv::connectedComponents(img, labels, 8, CV_32S));
    numGroups = MIN(numGroups, vars.numgroups);
    // at most there will be four regions detected on the image

    std::vector<cv::Vec3b> colors(numGroups);
    colors[0] = cv::Vec3b(245, 245, 245); // background is off-white
    colors[1] = cv::Vec3b(0, 0, 200);     // primary component is red
    colors[2] = cv::Vec3b(0, 200, 0);
    colors[3] = cv::Vec3b(200, 0, 0);

    D PRINTLN("type: " << img.type());
    D PRINTLN("dims: " << img.dims);
    D PRINTLN("channels: " << img.channels());

    regions = std::vector<cv::Mat>(numGroups);
    // Separate the regions into a list of separate images
    for (unsigned int g = 0; g < numGroups; g++) {

        regions[g] = cv::Mat(img.size(), CV_8U);

        for (int i = 0; i < img.rows; i++) {
            for (int j = 0; j < img.cols; j++) {

                unsigned int label = static_cast<unsigned int>(labels.at<int>(i, j));
                if (g == 0) {
                    regions[g].at<uchar>(i, j) = 0;
                } else {
                    regions[g].at<uchar>(i, j) = (label == g) ? 255 : 0;
                }
            }
        }
    }

    // make a color-coded image of all the regions

    cv::cvtColor(img, img, cv::COLOR_GRAY2BGR);
    img.convertTo(img, CV_8UC3);

    D LNPRINTLN("type: " << img.type());
    D PRINTLN("dims: " << img.dims);
    D PRINTLN("channels: " << img.channels());

    // go through the region map again but colorize img for displaying purposes
    for (int i = 0; i < img.rows; i++) {
        for (int j = 0; j < img.cols; j++) {

            int label = labels.at<int>(i, j);
            cv::Vec3b &pixel = img.at<cv::Vec3b>(i, j);
            pixel = colors[static_cast<unsigned long int>(label)];
        }
    }

    recommendedConversion = cv::COLOR_BGR2RGB;
}

void Processor::featurecompute(const struct featureVars &vars)
{
    centroids = std::vector<cv::Point>(regions.size());
    squarenesses = std::vector<double>(regions.size());
    hu1s = std::vector<double>(regions.size());
    hu2s = std::vector<double>(regions.size());
    hu3s = std::vector<double>(regions.size());
    hu4s = std::vector<double>(regions.size());
    bboxes = std::vector<cv::RotatedRect>(regions.size());
    relSizes = std::vector<double>(regions.size());

    // we calculate for all the possible regions at once, because of static image
    for (unsigned int i = 1; i < regions.size(); i++) {

        cv::Moments moments = cv::moments(regions[i], true);
        double xbar = moments.m10 / moments.m00;
        double ybar = moments.m01 / moments.m00;

        centroids[i] = cv::Point(static_cast<int>(xbar), static_cast<int>(ybar));

        double allHus[7];
        cv::HuMoments(moments, allHus); // calculates all 7, but we will only use 4

        // Calculate the first 3 Hu invariants
        hu1s[i] = allHus[0];
        D PRINTLN("hu invariant the first: " << hu1s[i]);

        hu2s[i] = allHus[1];
        D PRINTLN("hu invariant the second: " << hu2s[i]);

        hu3s[i] = allHus[2];
        D PRINTLN("hu invariant the third: " << hu3s[i]);

        hu4s[i] = allHus[3];
        D PRINTLN("hu invariant the fourth: " << hu4s[i]);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(regions[i], contours, cv::noArray(), cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);

        // draw central axis approx
        if (vars.caxis) {
            cv::Vec4f caxis;
            cv::fitLine(contours[0], caxis, cv::DIST_L2, 0, 0.01, 0.01);
            int ly = static_cast<int>((-caxis[2] * caxis[1] / caxis[0]) + caxis[3]);
            int ry = static_cast<int>(((img.cols - caxis[2]) * caxis[1] / caxis[0]) + caxis[3]);
            cv::line(img, cv::Point(img.cols - 1, ry), cv::Point(0, ly), cv::Scalar(255, 255, 0), 2);
            // repurposed from some python tutorial @opencv.org
        }

        bboxes[i] = cv::minAreaRect(contours[0]);


        // originally a mystery, later changed to be measure of squareness
        float max = MAX(bboxes[i].size.height, bboxes[i].size.width);
        float min = MIN(bboxes[i].size.height, bboxes[i].size.width);
        squarenesses[i] = static_cast<double>(max / min);

        cv::Point2f vertices[4];
        bboxes[i].points(vertices);

        // draw the min bounding box
        if (vars.boundbox) {
            for (int i = 0; i < 4; i++) {
                cv::line(img, vertices[i], vertices[(i + 1) % 4], cv::Scalar(200, 0, 200), 2, cv::LINE_AA);
            }
        }

        // get the relative size of the object to its minimum bounding box
        relSizes[i] = static_cast<double>(getArea(regions[i]) / bboxes[i].size.area()) * 100.0;
        D PRINTLN("relative size: " << relSizes[i]);

        // draw centroid
        if (vars.centroid) {
            cv::circle(img, centroids[i], 3, cv::Scalar(0, 0, 0), cv::FILLED, cv::LINE_AA);
        }
    }

    // recommended image remains the same as previous (ccanalysis) stage
}

std::string Processor::classify(const std::vector<double> &featvector, const struct classifierVars &classifierVars)
{
    if (db->size() < 1) {
        return "unknown";
    }

    if (classifierVars.knn) {
        return knnClassify(featvector, classifierVars.knn_k);
    } else {
        return euclClassify(featvector, classifierVars.min_acceptable);
    }
}

std::string Processor::euclClassify(const std::vector<double> &featVector, double min_acceptable)
{
    double weights[6] = {2.0, 1.0, 1.0, 1.0, 1.0, 1.0};
    std::vector<double> sumScaledDists((*db).size());
    std::vector<std::string> objList;

    for (unsigned int i = 0; i < featVector.size(); i++) // Go through the features of an unknown object
    {
        std::vector<double> rawDists, existingVals;

        // Go through all the known objects
        for (const auto &object : (*db)) {
            std::vector<double> objFVector = object.second;

            // Calculate distances between the new feature value and that of all the existing ones
            double rawDist = featVector[i] - objFVector[i];

            rawDists.push_back(rawDist);
            existingVals.push_back(objFVector[i]);

            if (i == 0) {
                objList.push_back(object.first);
            }
        }

        // Then scale Euclidean distance by std. dev
        cv::Scalar mean, stddev;
        cv::meanStdDev(existingVals, mean, stddev);

        // add it to the count (squared)
        for (unsigned int j = 0; j < db->size(); j++) {
            double sum = (rawDists[j] / stddev[0] * weights[i]);
            sumScaledDists[j] += (sum * sum);
        }
    }

    // Find the index with the lowest total distance across all features
    double bestDist = static_cast<double>(INT_MAX);
    unsigned int bestObj = 0;
    D PRINTLN("");
    for (unsigned int i = 0; i < sumScaledDists.size(); i++) {
        D PRINTLN(sumScaledDists[i]);
        if (sumScaledDists[i] < bestDist) {
            bestDist = sumScaledDists[i];
            bestObj = i;
        }
    }

    if (bestDist < min_acceptable) {
        return objList[bestObj];
    } else {
        return "unknown";
    }
}


// TODO - only retrain when necessary
std::string Processor::knnClassify(const std::vector<double> &featVector, unsigned int k)
{
    const int cols = 6;

    // train classifier if db has changed
    if (lastknowndbsize != db->size()) {
        lastknowndbsize = static_cast<unsigned int>(db->size());
        knn = cv::ml::KNearest::create();
        knn->setIsClassifier(true);

        knn_object2labels.clear();
        knn_labels2objects.clear();
        int labelcount = 0;
        for (const auto &sample : (*db)) {
            // If the object has yet to be assigned an label enumeration, give it one
            if (knn_object2labels.find(sample.first) == knn_object2labels.end()) {
                knn_object2labels[sample.first] = labelcount;
                knn_labels2objects[labelcount++] = sample.first;
            }
        }

        // load all the various training vectors into a Mat
        cv::Mat samples(static_cast<int>(db->size()), cols, CV_32F);
        cv::Mat responses(static_cast<int>(db->size()), 1, CV_32F);
        unsigned int i = 0;
        for (const auto &sample : (*db)) {
            // give the response for a row the correct label
            responses.at<float>(static_cast<int>(i), 0) = knn_object2labels[sample.first];

            for (unsigned int j = 0; j < cols; j++) {
                samples.at<float>(static_cast<int>(i), static_cast<int>(j)) =
                        static_cast<float>(sample.second[j]);
            }

            i++;
        }

        D LNPRINTLN("samples");
        D PRINTLN(samples);
        D LNPRINTLN("responses");
        D PRINTLN(responses);

        // train the knn classifier on the samples and responses
        knn->train(samples, cv::ml::ROW_SAMPLE, responses);
    }

    cv::Mat unknown(1, cols, CV_32F);
    // construct mat from feature vector
    for (unsigned int i = 0; i < cols; i++) {
        unknown.at<float>(0, static_cast<int>(i)) = static_cast<float>(featVector[i]);
    }

    // run unknown feature vector through trained knn
    float result = knn->findNearest(unknown, static_cast<int>(k), cv::noArray());

    return knn_labels2objects[static_cast<int>(result)];
}

void Processor::setImage(const cv::Mat &original)
{
    img = original.clone(); // must clone in case of reloading
    recommendedConversion = cv::COLOR_BGR2RGB;
}

cv::Mat Processor::getImage(int extent,
                            const struct preVars &preVars,
                            const struct threshVars &threshVars,
                            const struct morphVars &morphVars,
                            const struct ccaVars &ccaVars,
                            const struct featureVars &featureVars)
{
    if (extent < 1)
        goto END;

    preprocess(preVars);

    if (extent < 2)
        goto END;

    threshold(threshVars);

    if (extent < 3)
        goto END;

    morphprocess(morphVars);

    if (extent < 4)
        goto END;

    ccanalysis(ccaVars);

    if (extent < 5)
        goto END;

    featurecompute(featureVars);

END:
    return img;
}

unsigned long int Processor::getArea(const cv::Mat &binaryImg)
{
    unsigned long int count = 0;

    for (int i = 0; i < binaryImg.rows; i++) {
        for (int j = 0; j < binaryImg.cols; j++) {
            count += (binaryImg.at<uchar>(i, j) == 0) ? 0 : 1;
        }
    }

    return count;
}

std::vector<double> Processor::getFeatureVectors(int extent, unsigned int region)
{
    if (region > regions.size() - 1) {
        region = static_cast<unsigned int>(regions.size() - 1);
    }

    if (extent < 5) {
        return std::vector<double>(0);
    }

    std::vector<double> features(6);
    features.shrink_to_fit();

    features[0] = relSizes[region];
    features[1] = squarenesses[region];
    features[2] = hu1s[region];
    features[3] = hu2s[region];
    features[4] = hu3s[region];
    features[5] = hu4s[region];

    return features;
}

void Processor::assignObjectName(const std::string &name, std::vector<double> featVector)
{
    db->insert(std::pair<std::string, std::vector<double>>(name, featVector));
}

void Processor::changeDB(const std::string &newdb)
{
    dbmanager->writeCSV();
    dbmanager->readCSV(newdb);
}

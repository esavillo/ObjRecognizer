/**
 * Evan Savillo
 * Spring 2019
 * Main window class which interfaces with underlying opencv actions
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QImage>

#include <opencv2/opencv.hpp>

#include "processingvars.h"
#include "processor.h"

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public slots:
    void beginStreaming(); // start the qtimer, so a new frame is grabbed every period of timer
    void endStreaming(); // stop the qtimer
    void displayNewFrame(); // grab a new frame from the img sequence or camera and process it, then convert it to a QImage for display
    void displayNewFrame(int i); // display new frame, and increment the img_i counter
    void displayCurrentFrame(); // reprocess the current frame, so what's on screen is updated without changing current image
    void setTimerPeriod(int value); // set how fast the timer dings, i.e. how quickly new frames should be grabbed

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    enum class INPUT_MODE { ERROR, VIDEO, IMG_SEQUENCE };
    std::string inputSource; // the video file or folder of images
    std::string dbFile; // the datbase txt file

    // Fields
    // Qt
    Ui::MainWindow *ui;
    QTimer *qtimer; // dings repeatedly causing new frames to be grabbed from camera or image sequence
    int timerPeriod = -1;
    unsigned int currentRegion = 1; // the current region of a connected components analysis to inspect the feature info from

    // opencv-related
    cv::Mat img;
    cv::VideoCapture *cap;
    std::vector<std::string> *imgSequence; // vector of pathnames of images
    std::string path; // path to folder of images
    unsigned int img_i = 0; // current index in folder of images

    // personal
    INPUT_MODE input_mode = INPUT_MODE::ERROR;
    Processor *processor; // processess a read image
    /* structs containing relevant variables, mostly user-set, for each processing stage */
    struct preVars preVars;
    struct threshVars threshVars;
    struct morphVars morphVars;
    struct ccaVars ccaVars;
    struct featureVars featureVars;
    struct classifierVars classifierVars;


    // Methods
    void initializeImageStream(const std::string &source = ""); // read a new folder or start a new VideoCapture
    void grabNewFrame();
    void updateFeatures(std::vector<double> featureVectors); // Updates the values of the feature information on display
    void startup(); // tiny helper to get things going
    void denouement(); // wrap things up, classification stuff

};

#endif // MAINWINDOW_H

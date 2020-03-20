#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "processor.h"
#include "putility.h"

#include "QFileDialog"

#define DEBUG
#ifdef DEBUG
#define D if (true)
#else
#define D if (false)
#endif

namespace pu = putility;


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    /* DYNAMIC ALLOCATIONS */
    qtimer = new QTimer(this);
    processor = new Processor();
    cap = new cv::VideoCapture();


    /* SIGNALS<->SLOTS CONNECTIONs */
    // advance image sequence on spinbox advancement
    connect(ui->advanceFrameSpinBox, SIGNAL(valueChanged(int)), this, SLOT(displayNewFrame(int)));

    // connect the value of the spinbox to the timer interval
    connect(ui->fpsSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setTimerPeriod(int)));

    // play the video, or the remaining images, after the user has clicked the initiate Button
    connect(ui->initiateButton, SIGNAL(pressed()), this, SLOT(beginStreaming()));

    // stop the stream of images.
    connect(ui->stopButton, SIGNAL(pressed()), this, SLOT(endStreaming()));

    // display a new frame at the completion of every timer interval
    connect(qtimer, SIGNAL(timeout()), this, SLOT(displayNewFrame()));

    // reprocess the current frame
    connect(ui->reloadButton, SIGNAL(pressed()), this, SLOT(displayCurrentFrame()));

    // enable training mode (allows user to edit name)
    connect(ui->trainingCheckBox, &QCheckBox::toggled,
            [this](bool b) { ui->objNameText->setReadOnly(!b); });

    // disable training mode after classifying an object
    connect(ui->objNameText, &QLineEdit::returnPressed, [this]() {
        if (ui->trainingCheckBox->isChecked() && ui->stageSlider->value() > 4) {
            ui->trainingCheckBox->setChecked(false);

            processor->assignObjectName(ui->objNameText->text().toStdString(),
                                        processor->getFeatureVectors(ui->stageSlider->value(), currentRegion));
        }
    });

    // set the radio buttons to toggle the display of the appropriate regions
    connect(ui->r1RadioButton, &QRadioButton::toggled, [this](bool b) {
        if (b) {
            this->currentRegion = 1;
            displayCurrentFrame();
        }
    });
    connect(ui->r2RadioButton, &QRadioButton::toggled, [this](bool b) {
        if (b) {
            this->currentRegion = 2;
            displayCurrentFrame();
        }
    });
    connect(ui->r3RadioButton, &QRadioButton::toggled, [this](bool b) {
        if (b) {
            this->currentRegion = 3;
            displayCurrentFrame();
        }
    });

    // Set the file dialogs
    connect(ui->videoSourceToolButton, &QToolButton::pressed, [this]() {
        this->endStreaming();
        inputSource =
        QFileDialog::getOpenFileName(this, tr("Select Input Source"), "./../../../../.").toStdString();
        D LNPRINTLN("Changed to input source to '" << inputSource << "'");
        this->initializeImageStream(inputSource);
    });
    connect(ui->imagesSourceToolButton, &QToolButton::pressed, [this]() {
        this->endStreaming();
        inputSource =
        QFileDialog::getExistingDirectory(this, tr("Select Input Source"), "./../../../../.").toStdString();
        D LNPRINTLN("Changed to input source to '" << inputSource << "'");
        this->initializeImageStream(inputSource);
    });
    connect(ui->databaseToolButton, &QToolButton::pressed, [this]() {
        dbFile = QFileDialog::getOpenFileName(this, tr("Select Database to Load"), "./../../../../.", tr("*.txt"))
                 .toStdString();
        if (dbFile != "") {
            this->processor->changeDB(dbFile);
        }
    });


    /* PROCESSING VARIABLES */
    // Preprocessing Stage
    preVars = {true, false};
    connect(ui->bilateralCheckBox, &QCheckBox::toggled,
            [this](bool b) { this->preVars.bilateral = b; });
    connect(ui->medianCheckBox, &QCheckBox::toggled, [this](bool b) { this->preVars.median = b; });

    // Thresholding Stage
    threshVars = {100.0, 100.0, 100.0, true};
    connect(ui->redThreshSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double d) { this->threshVars.rthresh = d; });
    connect(ui->greenThreshSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double d) { this->threshVars.gthresh = d; });
    connect(ui->blueThreshSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double d) { this->threshVars.bthresh = d; });
    connect(ui->autothreshCheckBox, &QCheckBox::toggled,
            [this](bool b) { this->threshVars.autothresh = b; });

    // Morphological Stage
    morphVars = {true, 3, false, 2};
    connect(ui->rectButton, &QRadioButton::toggled,
            [this](bool b) { this->morphVars.rect_cross = b; });
    connect(ui->kernelSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int i) { this->morphVars.kernelSize = static_cast<unsigned int>(i); });
    connect(ui->erosionButton, &QRadioButton::toggled,
            [this](bool b) { this->morphVars.open_close = b; });
    connect(ui->moonSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int i) { this->morphVars.moons = static_cast<unsigned int>(i); });

    // Connected Component Analysis Stage
    ccaVars = {4};
    connect(ui->regionSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int i) { this->ccaVars.numgroups = static_cast<unsigned int>(i); });

    // Feature Computation Stage
    featureVars = {true, true, true};
    connect(ui->centroidCheckBox, &QCheckBox::toggled,
            [this](bool b) { this->featureVars.centroid = b; });
    connect(ui->boundBoxCheckBox, &QCheckBox::toggled,
            [this](bool b) { this->featureVars.boundbox = b; });
    connect(ui->cAxisCheckBox, &QCheckBox::toggled, [this](bool b) { this->featureVars.caxis = b; });

    // Classification Stage
    classifierVars = {true, 2.0, false, 5};
    connect(ui->euclRadioButton, &QRadioButton::toggled,
            [this](bool b) { this->classifierVars.eucl = b; });
    connect(ui->knnRadioButton, &QRadioButton::toggled,
            [this](bool b) { this->classifierVars.knn = b; });
    connect(ui->kSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int i) { this->classifierVars.knn_k = static_cast<unsigned int>(i); });


    // Establish a video feed, from a camera, video file, or sequence of images in a dir.
    inputSource = "/Users/slave/Desktop/Senior-Year/Spring-2019/cs365/projects/proj03/training";
    //        inputSource = "/Users/slave/Desktop/Videos/tmp.mp4";
    //    inputSource = "";

    startup();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete qtimer;
    delete processor;

    switch (input_mode) {
    case INPUT_MODE::VIDEO:
        delete cap;
        break;

    case INPUT_MODE::IMG_SEQUENCE:
        delete imgSequence;
        break;

    default:
        break;
    }
}

void MainWindow::startup()
{
    initializeImageStream(inputSource);
    //    initializeImageStream("/Users/Shared/IMG_7473.mp4");

    // get first image
    displayNewFrame();

    if (input_mode == INPUT_MODE::VIDEO) {
        beginStreaming();
    }
}

void MainWindow::initializeImageStream(const std::string &source)
{
    // Tidy up in case source is respecified during runtime
    disconnect(ui->advanceFrameSpinBox, SIGNAL(valueChanged(int)), this, SLOT(displayNewFrame(int)));
    img_i = 0;
    ui->advanceFrameSpinBox->setValue(0);
    ui->advanceFrameSpinBox->setMinimum(0);
    connect(ui->advanceFrameSpinBox, SIGNAL(valueChanged(int)), this, SLOT(displayNewFrame(int)));
    if (input_mode == INPUT_MODE::IMG_SEQUENCE) {
        delete imgSequence;
    }

    // Determine the source of the stream of input
    if (source == "") {
        LNPRINTLN("Input specified as default Camera");
        input_mode = INPUT_MODE::VIDEO;
        cap->open(0);
        ui->advanceFrameSpinBox->setMaximum(static_cast<int>(1000000000));
    } else if (pu::endsWith(source, ".mp4")) {
        LNPRINTLN("Input specified as video file '" << source << "'");
        input_mode = INPUT_MODE::VIDEO;
        cap->open(source);
    } else {
        LNPRINTLN("Input specified as image sequence located in '" << source << "'");
        input_mode = INPUT_MODE::IMG_SEQUENCE;
        imgSequence = pu::getImageFiles(source);

        ui->advanceFrameSpinBox->setMaximum(static_cast<int>(imgSequence->size() - 1));

        // Record the path in order to read the contained images, later
        path = source;
    }

    // check if we succeeded
    if (input_mode == INPUT_MODE::VIDEO && !cap->isOpened()) {
        std::cerr << "[!] ERROR: Unable to open camera" << std::endl;
        exit(-1);
    }

    if (input_mode == INPUT_MODE::IMG_SEQUENCE && imgSequence->size() < 1) {
        std::cerr << "[!] ERROR: image list empty" << std::endl;
        exit(-1);
    }
}

void MainWindow::grabNewFrame()
{
    switch (input_mode) {
    case INPUT_MODE::VIDEO:
        cap->read(img);

        // increment the spinbox if we are streaming
        if (qtimer->isActive()) {
            img_i++;
            ui->advanceFrameSpinBox->setValue(static_cast<int>(img_i));
        }

        // dont allow the user to "decrement"
        ui->advanceFrameSpinBox->setMinimum(static_cast<int>(img_i));
        break;

    case INPUT_MODE::IMG_SEQUENCE:
        if (img_i < imgSequence->size()) {
            img = cv::imread(path + "/" + imgSequence->at(img_i));

            // increment the spinbox if we are streaming
            if (qtimer->isActive()) {
                img_i++;
                ui->advanceFrameSpinBox->setValue(static_cast<int>(img_i));
            }
        } else {
            LNPRINTLN("You have reached the end of the given sequence.");
            endStreaming();
        }
        break;

    default:
        // this case will probably never actually occur
        std::cerr << "[!] ERROR: Unknown Input Type" << std::endl;
        exit(-1);
    }

    // check if we succeeded
    if (img.empty()) {
        std::cerr << "[!] ERROR: blank frame grabbed" << std::endl;
        LNPRINTLN("If connecting from a video camera, please try again.");
        exit(-1);
    }

    // Resize the image.
    cv::resize(img, img, cv::Size(400, 400), 0, 0, cv::INTER_LINEAR);
}

void MainWindow::displayNewFrame(int i)
{
    img_i = static_cast<unsigned int>(i);
    displayNewFrame();
}

void MainWindow::displayNewFrame()
{
    grabNewFrame();
    displayCurrentFrame();
}

void MainWindow::displayCurrentFrame()
{
    // Process the image according to current settings
    processor->setImage(img);
    cv::Mat processedImg =
    processor->getImage(ui->stageSlider->value(), preVars, threshVars, morphVars, ccaVars, featureVars);

    cv::cvtColor(processedImg, processedImg, processor->recommendedConversion);

    // construct QImage from mat
    QImage qimg(static_cast<uchar *>(processedImg.data), processedImg.cols, processedImg.rows,
                static_cast<int>(processedImg.step), QImage::Format_RGB888);

    // Set the picture in the video area to the current frame
    ui->videoArea->setPixmap(QPixmap::fromImage(qimg));

    denouement();
}

void MainWindow::denouement()
{
    std::vector<double> featVector = processor->getFeatureVectors(ui->stageSlider->value(), currentRegion);

    // Updates the features on the UI with the most recently calculated values
    updateFeatures(featVector);

    // Returns the object name currently being viewed.
    if (ui->stageSlider->value() > 4) {
        std::string objectName = processor->classify(featVector, classifierVars);
        if (!ui->trainingCheckBox->isChecked()) {
            ui->objNameText->setText(QString::fromStdString(objectName));
        }
    }
}

void MainWindow::updateFeatures(std::vector<double> featureVector)
{
    if (featureVector.size() == 0) {
        return;
    }

    // update the UI with the appropriate information
    ui->relSizeText->setText(QString::number(featureVector[0]));
    ui->mysteryText->setText(QString::number(featureVector[1]));
    ui->hu1Text->setText(QString::number(featureVector[2]));
    ui->hu2Text->setText(QString::number(featureVector[3]));
    ui->hu3Text->setText(QString::number(featureVector[4]));
    ui->hu4Text->setText(QString::number(featureVector[5]));
}

void MainWindow::beginStreaming()
{
    qtimer->start(1000 / (timerPeriod == -1 ? 24 : timerPeriod));

    /* make sure to disable manual frame incrementation, since we increment the frame counter
     * which will send out a valueChanged signal */
    disconnect(ui->advanceFrameSpinBox, SIGNAL(valueChanged(int)), this, SLOT(displayNewFrame(int)));
}

void MainWindow::endStreaming()
{
    if (qtimer->isActive()) {
        qtimer->stop();
    }

    // make sure to re-enable manual frame incrementation
    connect(ui->advanceFrameSpinBox, SIGNAL(valueChanged(int)), this, SLOT(displayNewFrame(int)));
}

void MainWindow::setTimerPeriod(int value)
{
    timerPeriod = value;
    qtimer->setInterval(1000 / timerPeriod);
}

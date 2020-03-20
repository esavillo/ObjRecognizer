/**
 * Evan Savillo
 * Spring 2019
 * Structs to pass parameters around
 */

#ifndef PROCESSINGVARS_H
#define PROCESSINGVARS_H

struct preVars
{
    bool median;
    bool bilateral;
};

struct threshVars
{
    double rthresh;
    double gthresh;
    double bthresh;
    bool autothresh;
};

struct morphVars
{
    bool rect_cross;
    unsigned int kernelSize;
    bool open_close;
    unsigned int moons;
};

struct ccaVars
{
    unsigned int numgroups;
};

struct featureVars
{
    bool centroid;
    bool boundbox;
    bool caxis;
};

struct classifierVars
{
    bool eucl;
    double min_acceptable;
    bool knn;
    unsigned int knn_k;
};


#endif // PROCESSINGVARS_H

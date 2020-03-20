/**
 * Evan Savillo
 * Spring 2019
 * "Database" Manager Class used to read or write date from saved .csv files from previous sessions
 */

#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <map>
#include <iostream>
#include <fstream>

#include "putility.h"

class DBManager
{
private:
    std::multimap<std::string, std::vector<double>> *db; // the loaded-from-a-csv database for classifying
    std::string dbfilename; // filename of <database_filename>.txt

public:
    DBManager();
    ~DBManager();

    void readCSV(const std::string &filename);
    void writeCSV(); // writes to the stored dbfilename
    void writeCSV(const std::string &outputName);
    std::multimap<std::string, std::vector<double>> *getDB();
};

#endif // DBMANAGER_H

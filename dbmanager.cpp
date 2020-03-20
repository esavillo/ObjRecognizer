#include "dbmanager.h"


DBManager::DBManager()
{
    db = new std::multimap<std::string, std::vector<double>>();
}

DBManager::~DBManager()
{
    delete db;
}

void DBManager::readCSV(const std::string &filename)
{
    dbfilename = filename;
    db->clear();

    std::ifstream file(filename);

    if (file.is_open()) {
        std::vector<std::string> line;
        std::string entry;
        char c;

        while (file.get(c)) {
            if (c == ',') {
                line.push_back(entry);
                entry.clear();
            } else if (c == '\n' && entry.size() > 0) {
                line.push_back(entry);
                entry.clear();

                db->insert(std::pair<std::string, std::vector<double>>(
                line[0], {std::stod(line[1]), std::stod(line[2]), std::stod(line[3]),
                          std::stod(line[4]), std::stod(line[5]), std::stod(line[6])}));

                line.clear();
            } else {
                entry += c;
            }
        }
    } else {
        std::cerr << "[!?] WARNING: could not open existing database file!" << std::endl;
    }

    file.close();
}

void DBManager::writeCSV()
{
    writeCSV(dbfilename);
}

void DBManager::writeCSV(const std::string &outputName)
{
    std::ofstream file(outputName);

    if (file.is_open()) {
        for (const auto &object : *db) {
            file << object.first;

            for (const auto &feature : object.second) {
                file << "," << feature;
            }

            file << std::endl;
        }
    } else {
        std::cerr << "[!] ERROR: could not write to file!" << std::endl;
    }

    file.close();
}

std::multimap<std::string, std::vector<double>> *DBManager::getDB()
{
    return db;
}

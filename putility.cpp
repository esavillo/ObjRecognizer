#include "putility.h"

#define DEBUG
#ifdef DEBUG
#define D if (true)
#else
#define D if (false)
#endif

namespace putility
{
std::vector<std::string> *getImageFiles(const std::string &dirname)
{
    DIR *dirp;
    struct dirent *dp;
    std::vector<std::string> *contents = new std::vector<std::string>();

    D PRINTLN("Attempting to access directory '" << dirname << "'");

    // open the directory
    dirp = opendir(dirname.c_str());
    if (dirp == nullptr) {
        std::cerr << "Cannot open " << dirname << std::endl;
        exit(-1);
    }

    // loop over the contents of the directory, looking for images
    while ((dp = readdir(dirp)) != nullptr) {
        std::string filetype = (dp->d_type == 4) ? "directory" : "file";

        if (strstr(dp->d_name, ".jpg") || strstr(dp->d_name, ".png") ||
            strstr(dp->d_name, ".ppm") || strstr(dp->d_name, ".tif")) {

            D PRINTLN("found image file: " << dp->d_name);
            contents->emplace_back(dp->d_name);
        } else {
            D PRINTLN("unrecognized " << filetype << ": " << dp->d_name);
        }
    }

    // close the directory
    closedir(dirp);

    D LNPRINTLN("Finished reading " << dirname);
    D PRINTLN("");

    return contents;
}

bool endsWith(const std::string &str, const std::string &ext)
{
    const char *c;
    bool endsWith = (c = strstr(str.c_str(), ext.c_str())) && *c == str.at(str.length() - ext.length());

    return endsWith;
}


} // namespace putility

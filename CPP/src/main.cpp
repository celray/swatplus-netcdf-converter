#include "Converter.h"
#include "Utils.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

// Simple argument parser helper
char* getCmdOption(char ** begin, char ** end, const std::string & option)
{
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option)
{
    return std::find(begin, end, option) != end;
}

void printUsage() {
    std::cout << "Usage: swat_nc_converter --region <RegionName> --txtInOutDir <Path> --convertedDir <Path> [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --climateResolution <float>  Resolution in degrees (default: 0.25)" << std::endl;
    std::cout << "  --shapePath <Path>           Path to shapefile" << std::endl;
    std::cout << "  --stopDate <YYYY-MM-DD>      Stop date (default: 2500-12-31)" << std::endl;
    std::cout << "  --help, -h                   Show this help message" << std::endl;
}

int main(int argc, char * argv[]) {
    if (cmdOptionExists(argv, argv + argc, "-h") || cmdOptionExists(argv, argv + argc, "--help")) {
        printUsage();
        return 0;
    }

    // Validate arguments
    std::vector<std::string> validArgs = {
        "--region", "--txtInOutDir", "--convertedDir", 
        "--climateResolution", "--shapePath", "--stopDate"
    };

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg[0] == '-') {
            bool found = false;
            for (const auto& valid : validArgs) {
                if (arg == valid) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                std::cerr << "Error: Unknown argument '" << arg << "'" << std::endl;
                printUsage();
                return 1;
            }
            // Skip value
            if (i + 1 < argc && argv[i+1][0] != '-') {
                i++;
            }
        }
    }

    char* regionOpt = getCmdOption(argv, argv + argc, "--region");
    char* txtInOutOpt = getCmdOption(argv, argv + argc, "--txtInOutDir");
    char* convertedOpt = getCmdOption(argv, argv + argc, "--convertedDir");
    char* resOpt = getCmdOption(argv, argv + argc, "--climateResolution");
    char* shapeOpt = getCmdOption(argv, argv + argc, "--shapePath");
    char* dateOpt = getCmdOption(argv, argv + argc, "--stopDate");

    if (!regionOpt || !txtInOutOpt || !convertedOpt) {
        std::cerr << "Error: Missing required arguments." << std::endl;
        printUsage();
        return 1;
    }

    std::string region = regionOpt;
    std::string txtInOutDir = txtInOutOpt;
    std::string convertedDir = convertedOpt;
    double resolution = resOpt ? std::stod(resOpt) : 0.25;
    std::string shapePath = shapeOpt ? shapeOpt : "";
    std::string stopDate = dateOpt ? dateOpt : "2500-12-31";

    if (!fs::exists(txtInOutDir)) {
        std::cerr << "Error: Input directory '" << txtInOutDir << "' does not exist." << std::endl;
        return 1;
    }

    if (!shapePath.empty() && !fs::exists(shapePath)) {
        std::cerr << "Error: Shapefile '" << shapePath << "' does not exist." << std::endl;
        return 1;
    }

    std::cout << "Starting SWAT+ NetCDF Converter (C++ Prototype)" << std::endl;
    std::cout << "Region: " << region << std::endl;
    std::cout << "Input: " << txtInOutDir << std::endl;
    std::cout << "Output: " << convertedDir << std::endl;

    // 1. Prepare Directories and Copy Files (Logic from convertSWATWeather)
    if (Utils::createDirectory(convertedDir)) {
        std::cout << "Created output directory." << std::endl;
    }

    bool fileCioExists = fs::exists(txtInOutDir + "/file.cio");

    if (fileCioExists) {
        // Copy essential files
        std::vector<std::string> files = Utils::listFiles(txtInOutDir);
        
        // Bad extensions to skip
        std::vector<std::string> badExtensions = {".cli", ".tmp", ".wnd", ".slr", ".hmd", ".pcp", ".tem"};
        
        for (const auto& file : files) {
            std::string filename = file.substr(file.find_last_of("/\\") + 1);
            
            // Filter logic: skip .txt
            if (filename.length() >= 4 && filename.substr(filename.length() - 4) == ".txt") continue;
            
            // Skip weather-sta.cli
            if (filename == "weather-sta.cli") continue;

            // Skip bad extensions
            bool isBad = false;
            for (const auto& ext : badExtensions) {
                if (filename.length() >= ext.length() && 
                    filename.substr(filename.length() - ext.length()) == ext) {
                    isBad = true;
                    break;
                }
            }
            if (isBad) continue;

            Utils::copyFile(file, convertedDir + "/" + filename);
        }
        
        // Explicitly copy weather-wgn.cli if it exists
        std::string wgnFile = txtInOutDir + "/weather-wgn.cli";
        if (fs::exists(wgnFile)) {
             Utils::copyFile(wgnFile, convertedDir + "/weather-wgn.cli");
        }
        
        // Update file.cio
        Utils::updateFileCIO(txtInOutDir, convertedDir, region);
    } else {
        std::cout << "file.cio not found. Skipping file copy and update." << std::endl;
    }

    // 2. Run Conversion (Logic from swatPlusNetCDFConverter)
    Converter converter(region, txtInOutDir, convertedDir);
    converter.run(resolution, shapePath, stopDate);

    return 0;
}

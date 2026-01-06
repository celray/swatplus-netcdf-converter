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
    std::cout << "Usage: swat_nc_converter -r <RegionName> -i <InputPath> -o <OutputPath> [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -r,   --region <name>            Region name (required)" << std::endl;
    std::cout << "  -i,   --inputPath <path>         Input TxtInOut directory (required)" << std::endl;
    std::cout << "  -o,   --outputPath <path>        Output converted directory (required)" << std::endl;
    std::cout << "  -res, --climateResolution <float> Resolution in degrees (default: 0.25)" << std::endl;
    std::cout << "  -b,   --shapePath <path>         Path to shapefile" << std::endl;
    std::cout << "  -s,   --stopDate <YYYY-MM-DD>    Stop date (default: 2500-12-31)" << std::endl;
    std::cout << "  -h,   --help                     Show this help message" << std::endl;
}

int main(int argc, char * argv[]) {
    if (cmdOptionExists(argv, argv + argc, "-h") || cmdOptionExists(argv, argv + argc, "--help")) {
        printUsage();
        return 0;
    }

    // Validate arguments
    std::vector<std::string> validArgs = {
        "-r", "--region", "-i", "--inputPath", "-o", "--outputPath", 
        "-res", "--climateResolution", "-b", "--shapePath", "-s", "--stopDate",
        "-h", "--help"
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

    // Helper lambda to get option with short/long forms
    auto getOption = [&](const std::string& shortOpt, const std::string& longOpt) -> char* {
        char* result = getCmdOption(argv, argv + argc, shortOpt);
        if (!result) result = getCmdOption(argv, argv + argc, longOpt);
        return result;
    };

    char* regionOpt = getOption("-r", "--region");
    char* inputPathOpt = getOption("-i", "--inputPath");
    char* outputPathOpt = getOption("-o", "--outputPath");
    char* resOpt = getOption("-res", "--climateResolution");
    char* shapeOpt = getOption("-b", "--shapePath");
    char* dateOpt = getOption("-s", "--stopDate");

    if (!regionOpt || !inputPathOpt || !outputPathOpt) {
        std::cerr << "Error: Missing required arguments." << std::endl;
        printUsage();
        return 1;
    }

    std::string region = regionOpt;
    std::string inputPath = inputPathOpt;
    std::string outputPath = outputPathOpt;
    double resolution = resOpt ? std::stod(resOpt) : 0.25;
    std::string shapePath = shapeOpt ? shapeOpt : "";
    std::string stopDate = dateOpt ? dateOpt : "2500-12-31";

    if (!fs::exists(inputPath)) {
        std::cerr << "Error: Input directory '" << inputPath << "' does not exist." << std::endl;
        return 1;
    }

    if (!shapePath.empty() && !fs::exists(shapePath)) {
        std::cerr << "Error: Shapefile '" << shapePath << "' does not exist." << std::endl;
        return 1;
    }

    std::cout << "Starting SWAT+ NetCDF Converter (C++ Prototype)" << std::endl;
    std::cout << "Region: " << region << std::endl;
    std::cout << "Input: " << inputPath << std::endl;
    std::cout << "Output: " << outputPath << std::endl;

    // 1. Prepare Directories and Copy Files (Logic from convertSWATWeather)
    if (Utils::createDirectory(outputPath)) {
        std::cout << "Created output directory." << std::endl;
    }

    bool fileCioExists = fs::exists(inputPath + "/file.cio");

    if (fileCioExists) {
        // Copy essential files
        std::vector<std::string> files = Utils::listFiles(inputPath);
        
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

            Utils::copyFile(file, outputPath + "/" + filename);
        }
        
        // Explicitly copy weather-wgn.cli if it exists
        std::string wgnFile = inputPath + "/weather-wgn.cli";
        if (fs::exists(wgnFile)) {
             Utils::copyFile(wgnFile, outputPath + "/weather-wgn.cli");
        }
        
        // Update file.cio
        Utils::updateFileCIO(inputPath, outputPath, region);
    } else {
        std::cout << "file.cio not found. Skipping file copy and update." << std::endl;
    }

    // 2. Run Conversion (Logic from swatPlusNetCDFConverter)
    Converter converter(region, inputPath, outputPath);
    converter.run(resolution, shapePath, stopDate);

    return 0;
}

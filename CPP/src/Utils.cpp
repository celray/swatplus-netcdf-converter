#include "Utils.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>

namespace fs = std::filesystem;

namespace Utils {

    bool copyFile(const std::string& src, const std::string& dst) {
        try {
            fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
            return true;
        } catch (fs::filesystem_error& e) {
            std::cerr << "Copy failed: " << e.what() << std::endl;
            return false;
        }
    }

    bool createDirectory(const std::string& path) {
        try {
            return fs::create_directories(path);
        } catch (fs::filesystem_error& e) {
            std::cerr << "Create directory failed: " << e.what() << std::endl;
            return false;
        }
    }

    bool deleteDirectory(const std::string& path) {
        try {
            return fs::remove_all(path) > 0;
        } catch (fs::filesystem_error& e) {
            std::cerr << "Delete directory failed: " << e.what() << std::endl;
            return false;
        }
    }

    std::vector<std::string> listFiles(const std::string& path) {
        std::vector<std::string> files;
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().string());
            }
        }
        return files;
    }

    std::string readFile(const std::string& path) {
        std::ifstream t(path);
        std::stringstream buffer;
        buffer << t.rdbuf();
        return buffer.str();
    }

    bool writeFile(const std::string& path, const std::string& content) {
        std::ofstream out(path);
        out << content;
        return out.good();
    }

    void updateFileCIO(const std::string& txtInOutDir, const std::string& convertedDir, const std::string& regionName) {
        std::string content = readFile(txtInOutDir + "/file.cio");
        std::stringstream ss(content);
        std::string line;
        std::stringstream output;
        
        output << "file.cio: written by swat-netcdf converter (C++)\n";

        // Skip first line of original if needed, or just process line by line
        // This is a simplified version of the Python logic
        bool first = true;
        while (std::getline(ss, line)) {
            if (first) { first = false; continue; } // Skip header if assumed

            // Replace logic
            // Check if line starts with specific keywords (ignoring leading whitespace)
            std::string trimmed = line;
            trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
            
            if (trimmed.find("pcp_path") == 0) {
                 output << "pcp_path          " << regionName << ".nc4   \n";
            } else if (trimmed.find("tmp_path") == 0) {
                 output << "tmp_path          " << regionName << ".nc4   \n";
            } else if (trimmed.find("slr_path") == 0) {
                 output << "slr_path          " << regionName << ".nc4   \n";
            } else if (trimmed.find("hmd_path") == 0) {
                 output << "hmd_path          " << regionName << ".nc4   \n";
            } else if (trimmed.find("wnd_path") == 0) {
                 output << "wnd_path          " << regionName << ".nc4   \n";
            } else if (trimmed.find("pet_path") == 0) {
                 output << "pet_path          " << regionName << ".nc4   \n";
            } else if (trimmed.find("climate") == 0) {
                 // Python: climate           netcdf.ncw        weather-wgn.cli   null              null              null              null              null              null              null
                 output << "climate           netcdf.ncw        weather-wgn.cli   null              null              null              null              null              null              null\n";
            }
            else {
                output << line << "\n";
            }
        }
        
        writeFile(convertedDir + "/file.cio", output.str());
    }

    void dualProgress(int primaryCount, int primaryEnd, int secondaryCount, int secondaryEnd, int barLength, const std::string& message) {
        std::string darkBlock   = "█";
        std::string denseBlock  = "▒";
        std::string lightBlock  = "░";
        std::string emptyBlock  = "-";

        double primaryPercent = (primaryEnd > 0) ? (double)primaryCount / primaryEnd * 100.0 : 100.0;
        double secondaryPercent = (secondaryEnd > 0) ? (double)secondaryCount / secondaryEnd * 100.0 : 100.0;

        int filledPrimary = (primaryEnd > 0) ? (int)(barLength * primaryCount / primaryEnd) : barLength;
        int filledShadow  = (secondaryEnd > 0) ? (int)(barLength * secondaryCount / secondaryEnd) : barLength;

        int filledSecondary = 0;

        if (filledShadow < filledPrimary) {
            filledPrimary = filledPrimary - filledShadow;
            filledSecondary = 0;
        } else {
            filledShadow = 0;
            darkBlock = denseBlock;
            
            filledSecondary = (secondaryEnd > 0) ? (int)(barLength * secondaryCount / secondaryEnd) : barLength;
            filledSecondary = filledSecondary - filledPrimary;
        }

        int shadowSection = filledShadow;
        int startSection  = filledPrimary;
        int middleSection = filledSecondary;
        int endSection    = barLength - startSection - middleSection - shadowSection;
        
        if (endSection < 0) endSection = 0;

        std::cout << "\r";
        for(int i=0; i<shadowSection; ++i) std::cout << denseBlock;
        for(int i=0; i<startSection; ++i) std::cout << darkBlock;
        for(int i=0; i<middleSection; ++i) std::cout << lightBlock;
        for(int i=0; i<endSection; ++i) std::cout << emptyBlock;

        std::cout << " " << std::fixed << std::setprecision(1) << std::setw(5) << primaryPercent << "% | "
                  << std::setw(5) << secondaryPercent << "% | " << message << "       " << std::flush;
                  
        if (primaryCount == primaryEnd && secondaryCount == secondaryEnd) {
             std::cout << std::endl;
        }
    }
}

#pragma once

#include <string>
#include <vector>

namespace Utils {
    bool copyFile(const std::string& src, const std::string& dst);
    bool createDirectory(const std::string& path);
    bool deleteDirectory(const std::string& path);
    std::vector<std::string> listFiles(const std::string& path);
    std::string readFile(const std::string& path);
    bool writeFile(const std::string& path, const std::string& content);
    void updateFileCIO(const std::string& txtInOutDir, const std::string& convertedDir, const std::string& regionName);
    void dualProgress(int primaryCount, int primaryEnd, int secondaryCount, int secondaryEnd, int barLength = 40, const std::string& message = "");
}

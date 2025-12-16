#pragma once

#include <string>
#include <vector>
#include <map>
#include <netcdf>

struct Station {
    int id;
    std::string name;
    double lat;
    double lon;
    double elev;
    int startYear = -1;
    int startDay = -1;
    std::vector<double> data; // Time series data
};

struct VariableData {
    std::string name; // e.g., "pcp", "tmp_max", "tmp_min"
    std::string unit;
    std::vector<Station> stations;
};

class Converter {
public:
    Converter(const std::string& region, const std::string& txtInOutDir, const std::string& convertedDir);
    
    void run(double climateResolution, const std::string& shapePath, const std::string& stopDate);

private:
    std::string m_region;
    std::string m_txtInOutDir;
    std::string m_convertedDir;
    double m_resolution;
    
    // Bounding box
    double m_minLat, m_maxLat, m_minLon, m_maxLon;

    // Time reference
    int m_startYear = -1;
    int m_startDay = -1;

    std::vector<VariableData> m_weatherData;

    void processWeatherFiles();
    void readStationFile(const std::string& filepath, const std::string& varName, const std::string& unit, int valueColumnIndex = 0);
    void createNetCDF(const std::string& filename);
    void readShapefile(const std::string& shapePath);
    void createStationListFile();
};

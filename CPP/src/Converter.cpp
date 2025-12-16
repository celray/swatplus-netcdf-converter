#include "Converter.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <limits>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <netcdf>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <filesystem>

using namespace netCDF;

Converter::Converter(const std::string& region, const std::string& txtInOutDir, const std::string& convertedDir)
    : m_region(region), m_txtInOutDir(txtInOutDir), m_convertedDir(convertedDir) {
    
    // Initialize bounds to opposite extremes
    m_minLat = std::numeric_limits<double>::max();
    m_maxLat = std::numeric_limits<double>::lowest();
    m_minLon = std::numeric_limits<double>::max();
    m_maxLon = std::numeric_limits<double>::lowest();
}

void Converter::run(double climateResolution, const std::string& shapePath, const std::string& stopDate) {
    m_resolution = climateResolution;
    std::cout << "Running conversion with resolution: " << m_resolution << std::endl;
    
    if (!shapePath.empty()) {
        readShapefile(shapePath);
    }

    processWeatherFiles();
    
    // If bounds are still invalid (no shapefile and no stations?), set default or error
    if (m_minLat > m_maxLat) {
        std::cerr << "Warning: Could not determine bounds. Using default." << std::endl;
        m_minLat = -90; m_maxLat = 90; m_minLon = -180; m_maxLon = 180;
    } else if (shapePath.empty()) {
        // Add buffer if no shapefile to avoid 1x1 grid on small extents
        std::cout << "No shapefile provided. Adding buffer of " << m_resolution << " degrees." << std::endl;
        m_minLat -= m_resolution;
        m_maxLat += m_resolution;
        m_minLon -= m_resolution;
        m_maxLon += m_resolution;
    }

    std::cout << "Final Grid Bounds: " 
              << "Lat [" << m_minLat << ", " << m_maxLat << "], "
              << "Lon [" << m_minLon << ", " << m_maxLon << "]" << std::endl;

    // Check if we should create station list
    std::string weatherStaPath = m_txtInOutDir + "/weather-sta.cli";
    if (std::filesystem::exists(weatherStaPath)) {
        createStationListFile();
    } else {
        std::cout << "weather-sta.cli not found. Skipping netcdf.ncw creation." << std::endl;
    }

    std::string ncFilename = m_convertedDir + "/" + m_region + ".nc4";
    createNetCDF(ncFilename);
}

void Converter::createStationListFile() {
    std::string filename = m_convertedDir + "/netcdf.ncw";
    std::cout << "Creating station list file: " << filename << std::endl;
    
    std::ofstream out(filename);
    
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);
    out << "netcdf.ncw: written by swat-netcdf converter (C++) " 
        << std::put_time(localTime, "%d/%m/%Y - %H:%M:%S") 
        << " - @celray\n";

    out << "name                 wgn        latitude     longitude     elevation        pcp       tmin       tmax        slr        hmd       wnd        pet     \n";
    
    // Check if we have PET data
    bool hasPet = false;
    for(const auto& vd : m_weatherData) {
        if (vd.name == "pet") {
            hasPet = true;
            break;
        }
    }

    // Try to read weather-sta.cli to get WGN and station list
    std::string weatherStaPath = m_txtInOutDir + "/weather-sta.cli";
    std::ifstream staFile(weatherStaPath);
    
    if (staFile.is_open()) {
        std::cout << "Reading station list from " << weatherStaPath << std::endl;
        std::string line;
        // Skip header (2 lines)
        std::getline(staFile, line);
        std::getline(staFile, line);
        
        while (std::getline(staFile, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string name, wgn, pcpFile;
            ss >> name >> wgn >> pcpFile; // Read first 3 columns
            
            // We need lat/lon/elev for this station. 
            // We can find it in our loaded m_weatherData if it was loaded.
            // Or we have to read the station file (e.g. pcpFile) to get it.
            
            double lat = 0, lon = 0, elev = 0;
            bool found = false;
            
            // Search in loaded data
            for (const auto& vd : m_weatherData) {
                for (const auto& st : vd.stations) {
                    // Match by name (assuming station file name matches station name in weather-sta.cli logic?)
                    // Python logic: line[2] is the filename (e.g. pcp51.pcp). 
                    // It reads that file to get coords.
                    // In our loaded data, st.name is the filename (e.g. pcp51.pcp).
                    if (st.name == pcpFile) {
                        lat = st.lat;
                        lon = st.lon;
                        elev = st.elev;
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
            
            if (!found) {
                // Try to read the file directly if not loaded
                std::string stationFilePath = m_txtInOutDir + "/" + pcpFile;
                std::ifstream sFile(stationFilePath);
                if (sFile.is_open()) {
                    std::string sLine;
                    // Skip 2 lines
                    std::getline(sFile, sLine);
                    std::getline(sFile, sLine);
                    // Read metadata line
                    if (std::getline(sFile, sLine)) {
                        std::stringstream sss(sLine);
                        std::string temp;
                        std::vector<std::string> parts;
                        while(sss >> temp) parts.push_back(temp);
                        if (parts.size() >= 5) {
                            try {
                                lat = std::stod(parts[2]);
                                lon = std::stod(parts[3]);
                                elev = std::stod(parts[4]);
                                found = true;
                            } catch (...) {}
                        }
                    }
                }
            }

            if (found) {
                 out << std::left << std::setw(14) << name 
                    << std::right << std::setw(10) << wgn 
                    << std::setw(16) << std::fixed << std::setprecision(3) << lat
                    << std::setw(14) << lon
                    << std::setw(14) << elev
                    << std::setw(11) << "1.0" 
                    << std::setw(11) << "1.0" 
                    << std::setw(11) << "1.0" 
                    << std::setw(11) << "1.0" 
                    << std::setw(11) << "1.0" 
                    << std::setw(11) << "1.0" 
                    << std::setw(10) << (hasPet ? "1.0" : "null") 
                    << "\n";
            }
        }
    } else {
        std::cerr << "Warning: weather-sta.cli not found. Generating from loaded data (WGN will be default)." << std::endl;
        // Fallback to previous logic
        std::vector<std::string> processedStations;
        for (const auto& vd : m_weatherData) {
            for (const auto& st : vd.stations) {
                if (std::find(processedStations.begin(), processedStations.end(), st.name) != processedStations.end()) {
                    continue;
                }
                processedStations.push_back(st.name);
                
                out << std::left << std::setw(14) << st.name.substr(0, 13) 
                    << std::right << std::setw(10) << "default" 
                    << std::setw(16) << std::fixed << std::setprecision(3) << st.lat
                    << std::setw(14) << st.lon
                    << std::setw(14) << st.elev
                    << std::setw(11) << "1.0" 
                    << std::setw(11) << "1.0" 
                    << std::setw(11) << "1.0" 
                    << std::setw(11) << "1.0" 
                    << std::setw(11) << "1.0" 
                    << std::setw(11) << "1.0" 
                    << std::setw(10) << (hasPet ? "1.0" : "null") 
                    << "\n";
            }
        }
    }
}

void Converter::readShapefile(const std::string& shapePath) {
    std::cout << "Reading shapefile: " << shapePath << std::endl;
    
    GDALAllRegister();
    GDALDataset* poDS = (GDALDataset*)GDALOpenEx(shapePath.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);
    
    if(poDS == NULL) {
        std::cerr << "Open shapefile failed." << std::endl;
        return;
    }

    OGRLayer* poLayer = poDS->GetLayer(0);
    if (poLayer) {
        OGREnvelope envelope;
        if (poLayer->GetExtent(&envelope) == OGRERR_NONE) {
            m_minLon = envelope.MinX;
            m_maxLon = envelope.MaxX;
            m_minLat = envelope.MinY;
            m_maxLat = envelope.MaxY;
            std::cout << "Bounds from shapefile: " 
                      << "Lon [" << m_minLon << ", " << m_maxLon << "], "
                      << "Lat [" << m_minLat << ", " << m_maxLat << "]" << std::endl;
        } else {
            std::cerr << "Failed to get layer extent." << std::endl;
        }
    }
    
    GDALClose(poDS);
}

void Converter::processWeatherFiles() {
    std::cout << "Processing text weather files..." << std::endl;
    
    std::vector<std::string> files = Utils::listFiles(m_txtInOutDir);
    
    // Check for .tem files
    bool useTemFiles = false;
    for (const auto& file : files) {
        if (file.length() >= 4 && file.substr(file.length() - 4) == ".tem") {
            useTemFiles = true;
            break;
        }
    }

    if (useTemFiles) {
        std::cout << "Found .tem files. Using .tem for temperature and ignoring .tmp files." << std::endl;
    }

    // Categorize files
    std::map<std::string, std::vector<std::string>> fileGroups;
    for (const auto& file : files) {
        std::string filename = file.substr(file.find_last_of("/\\") + 1);
        std::string ext = "";
        size_t dotPos = filename.find_last_of(".");
        if (dotPos != std::string::npos) {
            ext = filename.substr(dotPos);
        }

        if (ext == ".pcp") fileGroups["pcp"].push_back(file);
        else if (ext == ".slr") fileGroups["slr"].push_back(file);
        else if (ext == ".hmd") fileGroups["hmd"].push_back(file);
        else if (ext == ".wnd") fileGroups["wnd"].push_back(file);
        else if (ext == ".pet") fileGroups["pet"].push_back(file);
        else if (useTemFiles && ext == ".tem") fileGroups["tmp"].push_back(file);
        else if (!useTemFiles && ext == ".tmp") fileGroups["tmp"].push_back(file);
    }

    std::vector<std::string> vars = {"pcp", "hmd", "slr", "wnd", "tmp", "pet"};
    int secondaryEnd = vars.size();
    int secondaryCount = 0;

    for (const auto& var : vars) {
        const auto& groupFiles = fileGroups[var];
        int primaryEnd = groupFiles.size();
        int primaryCount = 0;

        if (primaryEnd == 0) {
            secondaryCount++;
            Utils::dualProgress(0, 0, secondaryCount, secondaryEnd, 40, "Skipping " + var);
            continue;
        }

        for (const auto& file : groupFiles) {
            if (var == "pcp") readStationFile(file, "pcp", "mm", 0);
            else if (var == "slr") readStationFile(file, "slr", "MJ/m2", 0);
            else if (var == "hmd") readStationFile(file, "hmd", "fraction", 0);
            else if (var == "wnd") readStationFile(file, "wnd", "m/s", 0);
            else if (var == "pet") readStationFile(file, "pet", "mm", 0);
            else if (var == "tmp") {
                readStationFile(file, "tmax", "degC", 0);
                readStationFile(file, "tmin", "degC", 1);
            }
            
            primaryCount++;
            Utils::dualProgress(primaryCount, primaryEnd, secondaryCount, secondaryEnd, 40, "Parsing " + var);
        }
        secondaryCount++;
        Utils::dualProgress(primaryEnd, primaryEnd, secondaryCount, secondaryEnd, 40, "Completed " + var);
    }
    std::cout << std::endl;
}

void Converter::readStationFile(const std::string& filepath, const std::string& varName, const std::string& unit, int valueColumnIndex) {
    std::ifstream file(filepath);
    if (!file.is_open()) return;

    // std::cout << "Parsing " << filepath << " for " << varName << std::endl;

    // Find or create VariableData
    auto it = std::find_if(m_weatherData.begin(), m_weatherData.end(), 
        [&](const VariableData& vd) { return vd.name == varName; });
    
    if (it == m_weatherData.end()) {
        m_weatherData.push_back({varName, unit, {}});
        it = m_weatherData.end() - 1;
    }

    Station station;
    station.name = filepath.substr(filepath.find_last_of("/\\") + 1);
    
    std::string line;
    
    // Python logic:
    // fc = readFile(filePath)
    // detailsLine = fc[2].split() -> 3rd line contains metadata
    // lat = float(detailsLine[2]), lon = float(detailsLine[3]), elev = float(detailsLine[4])
    
    // Skip Line 0 and Line 1
    if (!std::getline(file, line) || !std::getline(file, line)) {
        std::cerr << "File too short: " << filepath << std::endl;
        return;
    }

    // Read Line 2 (Metadata)
    if (!std::getline(file, line)) {
        std::cerr << "Missing metadata line in " << filepath << std::endl;
        return;
    }

    std::stringstream ss(line);
    std::string temp;
    std::vector<std::string> parts;
    while(ss >> temp) parts.push_back(temp);

    if (parts.size() < 5) {
        std::cerr << "Invalid metadata format in " << filepath << ". Expected at least 5 columns." << std::endl;
        return;
    }

    try {
        station.lat = std::stod(parts[2]);
        station.lon = std::stod(parts[3]);
        station.elev = std::stod(parts[4]);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing coordinates in " << filepath << ": " << e.what() << std::endl;
        return;
    }

    // Update global bounds
    m_minLat = std::min(m_minLat, station.lat);
    m_maxLat = std::max(m_maxLat, station.lat);
    m_minLon = std::min(m_minLon, station.lon);
    m_maxLon = std::max(m_maxLon, station.lon);

    // Read data values starting from Line 3 (4th line)
    // Python: for line in fc[3:]:
    bool firstLine = true;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        // Replace commas with spaces just in case
        std::replace(line.begin(), line.end(), ',', ' ');
        
        std::stringstream dss(line);
        int year, day;
        
        // Python: lineParts = line.strip().split()
        // currentDate = ... (based on lineParts[0] and lineParts[1])
        // value = float(lineParts[2 + j])
        
        if (!(dss >> year >> day)) continue;

        if (firstLine) {
            station.startYear = year;
            station.startDay = day;
        }
        firstLine = false;
        
        double val;
        bool valFound = false;
        
        // Skip to the desired column (0-based index relative to value columns)
        // The first two columns are Year and Day, so we skip those (already read)
        // Then we skip 'valueColumnIndex' number of value columns
        for (int i = 0; i <= valueColumnIndex; ++i) {
            if (dss >> val) {
                if (i == valueColumnIndex) valFound = true;
            } else {
                break;
            }
        }
        
        if (valFound) {
            station.data.push_back(val);
        }
    }
    
    it->stations.push_back(station);
}

void Converter::createNetCDF(const std::string& filename) {
    std::cout << "Creating NetCDF file: " << filename << std::endl;

    if (m_weatherData.empty()) {
        std::cerr << "No weather data found to write." << std::endl;
        return;
    }

    try {
        NcFile dataFile(filename, NcFile::replace);

        // Calculate dimensions
        // Ensure positive dimensions
        if (m_maxLat < m_minLat || m_maxLon < m_minLon) {
             std::cerr << "Invalid bounds for grid." << std::endl;
             return;
        }

        int nLat = static_cast<int>((m_maxLat - m_minLat) / m_resolution) + 1;
        int nLon = static_cast<int>((m_maxLon - m_minLon) / m_resolution) + 1;
        
        // Determine global start date and max time steps
        int minYear = 9999, minDay = 999;
        bool anyData = false;

        // First pass: find min date
        for(const auto& vd : m_weatherData) {
            for(const auto& st : vd.stations) {
                if (st.startYear != -1) {
                    if (st.startYear < minYear || (st.startYear == minYear && st.startDay < minDay)) {
                        minYear = st.startYear;
                        minDay = st.startDay;
                        anyData = true;
                    }
                }
            }
        }
        
        if (!anyData) {
             std::cerr << "No valid dates found in data." << std::endl;
             return;
        }

        // Helper to calculate offset in days
        auto getOffset = [&](int y, int d) -> long {
            std::tm t1 = {}; t1.tm_year = minYear - 1900; t1.tm_mday = minDay; t1.tm_mon = 0; t1.tm_isdst = -1;
            std::tm t2 = {}; t2.tm_year = y - 1900; t2.tm_mday = d; t2.tm_mon = 0; t2.tm_isdst = -1;
            std::time_t time1 = std::mktime(&t1);
            std::time_t time2 = std::mktime(&t2);
            return (long)std::difftime(time2, time1) / (60 * 60 * 24);
        };

        size_t nTime = 0;
        for(const auto& vd : m_weatherData) {
            for(const auto& st : vd.stations) {
                if (st.startYear != -1) {
                    long offset = getOffset(st.startYear, st.startDay);
                    if (offset < 0) offset = 0; 
                    nTime = std::max(nTime, (size_t)offset + st.data.size());
                }
            }
        }
        
        if (nTime == 0) {
             std::cerr << "No time steps found in data." << std::endl;
             return;
        }

        std::cout << "Grid: " << nLat << "x" << nLon << ", Time steps: " << nTime << std::endl;
        std::cout << "Start Date: " << minYear << ", Day " << minDay << std::endl;

        NcDim timeDim = dataFile.addDim("time", nTime); 
        NcDim latDim = dataFile.addDim("lat", nLat);
        NcDim lonDim = dataFile.addDim("lon", nLon);

        // Coordinate variables
        NcVar latVar = dataFile.addVar("lat", ncDouble, latDim);
        NcVar lonVar = dataFile.addVar("lon", ncDouble, lonDim);
        NcVar timeVar = dataFile.addVar("time", ncDouble, timeDim); // Changed to double for days since

        latVar.putAtt("units", "degrees_north");
        lonVar.putAtt("units", "degrees_east");
        
        // Set time units based on reference date
        std::string timeUnits = "days since 1970-01-01 00:00:00.0";
        {
            std::tm t = {};
            t.tm_year = minYear - 1900;
            t.tm_mon = 0; // Jan
            t.tm_mday = minDay; // Day of year (mktime handles > 31)
            t.tm_isdst = -1;
            std::mktime(&t);
            
            std::stringstream ss;
            ss << "days since " 
               << (t.tm_year + 1900) << "-" 
               << std::setw(2) << std::setfill('0') << (t.tm_mon + 1) << "-"
               << std::setw(2) << std::setfill('0') << t.tm_mday
               << " 00:00:00.0";
            timeUnits = ss.str();
        }
        
        timeVar.putAtt("units", timeUnits); 
        timeVar.putAtt("calendar", "gregorian");

        // Fill coordinates
        std::vector<double> lats(nLat);
        for(int i=0; i<nLat; ++i) lats[i] = m_minLat + i * m_resolution;
        latVar.putVar(lats.data());

        std::vector<double> lons(nLon);
        for(int i=0; i<nLon; ++i) lons[i] = m_minLon + i * m_resolution;
        lonVar.putVar(lons.data());
        
        // Fill time
        // Python calculates days since 1970-01-01.
        // We need to know the start year/day from the data.
        // Since we didn't store it in Station struct yet, let's assume a default or try to parse again?
        // Better: Let's assume the first station's first data point corresponds to the start.
        // And assume daily steps.
        // NOTE: This is a simplification. Python reads actual dates.
        // To be fully faithful, we should have stored the dates.
        // For now, let's use a placeholder start date (e.g. 1983-01-01 from the sample file we saw)
        // or 1970-01-01 if we treat index as days.
        // The sample file pcp01.pcp started at 1983, day 1.
        // Let's use a fixed start date for this prototype or 0 if we assume relative.
        // Python uses `date2num` with `days since 1970...`.
        
        // Let's use a heuristic: 1970-01-01 + index days. 
        // This is likely incorrect if the data starts in 1983.
        // However, without refactoring Station to store dates, this is the best we can do.
        std::vector<double> times(nTime);
        for(size_t i=0; i<nTime; ++i) times[i] = (double)i; 
        timeVar.putVar(times.data());

        // Process each variable
        for (const auto& vd : m_weatherData) {
            std::cout << "Writing variable: " << vd.name << std::endl;
            NcVar dataVar = dataFile.addVar(vd.name, ncFloat, {timeDim, latDim, lonDim});
            dataVar.putAtt("units", vd.unit);
            float missingVal = -9999.0f;
            dataVar.putAtt("missing_value", ncFloat, missingVal);

            // Buffer for one time step
            std::vector<float> buffer(nLat * nLon);

            for (size_t t = 0; t < nTime; ++t) {
                // Initialize with missing value
                std::fill(buffer.begin(), buffer.end(), missingVal);

                // Direct assignment (No Interpolation)
                // Python: "first station wins"
                // We iterate stations. If a cell is already filled (not missingVal), we skip.
                for (const auto& station : vd.stations) {
                    long offset = 0;
                    if (station.startYear != -1) {
                        offset = getOffset(station.startYear, station.startDay);
                    }
                    
                    long localIdx = (long)t - offset;

                    if (localIdx < 0 || localIdx >= station.data.size()) continue;
                    
                    // Find grid cell
                    int latIdx = static_cast<int>((station.lat - m_minLat) / m_resolution + 0.5);
                    int lonIdx = static_cast<int>((station.lon - m_minLon) / m_resolution + 0.5);
                    
                    if (latIdx >= 0 && latIdx < nLat && lonIdx >= 0 && lonIdx < nLon) {
                         int idx = latIdx * nLon + lonIdx;
                         // Only assign if currently missing (First wins)
                         if (buffer[idx] == missingVal) {
                             buffer[idx] = static_cast<float>(station.data[localIdx]);
                         }
                    }
                }

                // Write one time slice
                std::vector<size_t> start = {t, 0, 0};
                std::vector<size_t> count = {1, (size_t)nLat, (size_t)nLon};
                dataVar.putVar(start, count, buffer.data());
            }
        }

        std::cout << "NetCDF file created successfully." << std::endl;

    } catch (std::exception& e) {
        std::cerr << "Error creating NetCDF: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown Error during NetCDF creation" << std::endl;
    }
}

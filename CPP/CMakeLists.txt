cmake_minimum_required(VERSION 3.15)
project(SwatNetCDFConverter VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force static runtime (MT) for MSVC to create a standalone executable
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Dependencies
# Prefer CMake config packages (works best with vcpkg). Only fall back to
# pkg-config on platforms where that's the typical distribution mechanism.
find_package(netCDFCxx CONFIG QUIET)

if(NOT TARGET netCDF::netcdf-cxx4)
    set(_netcdfcxx_config "")
    set(_vcpkg_triplet "")
    set(_vcpkg_installed_dir "")

    if(DEFINED VCPKG_TARGET_TRIPLET)
        set(_vcpkg_triplet "${VCPKG_TARGET_TRIPLET}")
    elseif(DEFINED CACHE{VCPKG_TARGET_TRIPLET})
        set(_vcpkg_triplet "$CACHE{VCPKG_TARGET_TRIPLET}")
    endif()

    if(DEFINED VCPKG_INSTALLED_DIR)
        set(_vcpkg_installed_dir "${VCPKG_INSTALLED_DIR}")
    elseif(DEFINED CACHE{VCPKG_INSTALLED_DIR})
        set(_vcpkg_installed_dir "$CACHE{VCPKG_INSTALLED_DIR}")
    endif()

    if(_vcpkg_installed_dir AND _vcpkg_triplet)
        set(_netcdfcxx_config "${_vcpkg_installed_dir}/${_vcpkg_triplet}/share/netCDFCxx/netCDFCxxConfig.cmake")
    elseif(_vcpkg_triplet)
        set(_netcdfcxx_config "${CMAKE_CURRENT_LIST_DIR}/vcpkg/installed/${_vcpkg_triplet}/share/netCDFCxx/netCDFCxxConfig.cmake")
    endif()

    if(_netcdfcxx_config AND EXISTS "${_netcdfcxx_config}")
        include("${_netcdfcxx_config}")
    endif()
endif()

if(NOT TARGET netCDF::netcdf-cxx4)
    if(WIN32)
        message(FATAL_ERROR "Could not locate netCDF C++ (netcdf-cxx4). On Windows this project expects vcpkg to provide the CMake config package netCDFCxx. Attempted netCDFCxx config: '${_netcdfcxx_config}' (triplet='${_vcpkg_triplet}', installed='${_vcpkg_installed_dir}').")
    endif()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(NETCDF_CXX REQUIRED netcdf-cxx4)
endif()
find_package(GDAL CONFIG REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/Converter.cpp
    src/Utils.cpp
)

# Executable
add_executable(swat2netcdf ${SOURCES})

# Include directories
target_include_directories(swat2netcdf PRIVATE include)
if(NOT netCDFCxx_FOUND)
    target_include_directories(swat2netcdf PRIVATE ${NETCDF_CXX_INCLUDE_DIRS})
    target_link_directories(swat2netcdf PRIVATE ${NETCDF_CXX_LIBRARY_DIRS})
    target_compile_options(swat2netcdf PRIVATE ${NETCDF_CXX_CFLAGS_OTHER})
endif()

# Link libraries
if(TARGET netCDF::netcdf-cxx4)
    target_link_libraries(swat2netcdf PRIVATE netCDF::netcdf-cxx4 netCDF::netcdf GDAL::GDAL)
else()
    target_link_libraries(swat2netcdf PRIVATE ${NETCDF_CXX_LIBRARIES} GDAL::GDAL)
endif()

# Installation
install(TARGETS swat2netcdf DESTINATION bin)

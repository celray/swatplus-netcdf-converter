# SWAT+ NetCDF Converter Tools

This repository contains Python tools for converting SWAT+ weather data to NetCDF format, designed to work with the NetCDF-supported version of SWAT+.

## Tools

### `convertSWATWeather`
This script creates the `weather-sta.cli` equivalent for NetCDF and prepares the NetCDF data package for `swatNetCDF`.

### `swatPlusNetCDFConverter`
A simple SWAT weather to NetCDF converter. It handles raw data only without interpolation.

## SWAT+ NetCDF Support

These tools are intended to be used with the SWAT+ version that supports NetCDF. The source code for SWAT+ with NetCDF support can be found in the `netCDF-Support` branch of the official SWAT+ repository:

[https://github.com/swat-model/swatplus/tree/netCDF-Support](https://github.com/swat-model/swatplus/tree/netCDF-Support)

## Requirements

- Python 3
- [`ccfx`](https://pypi.org/project/ccfx/)

**Note for Windows Users:**
`ccfx` requires GDAL, which can be tricky to install on Windows. If you encounter issues, it is recommended to use [`gdal-installer`](https://pypi.org/project/gdal-installer/) to set up GDAL first.

You can install the standard dependencies using:

```bash
pip install -r requirements.txt
```

## Usage

The main entry point is the `convertSWATWeather` script. This script parses the arguments and internally calls `swatPlusNetCDFConverter` to perform the actual conversion of SWAT+ weather data to NetCDF format.

Run the script from the command line as follows:

```bash
python3 convertSWATWeather --region <RegionName> --txtInOutDir <PathToInput> --convertedDir <PathToOutput> [options]
```

### Arguments

| Argument | Type | Required | Description |
|----------|------|----------|-------------|
| `--region` | String | Yes | Region name (e.g., `01010010`). |
| `--txtInOutDir` | String | Yes | Directory containing the SWAT text weather files (e.g., `TxtInOut`). |
| `--convertedDir` | String | Yes | Directory where the converted NetCDF files will be saved. |
| `--climateResolution` | Float | No | Climate resolution in meters. Default is `0.25`. |
| `--shapePath` | String | No | Optional path to a shapefile for the region. |
| `--stopDate` | String | No | Optional stop date for the data in `YYYY-MM-DD` format. Default is `1985-12-31`. |

### Example

```bash
python3 convertSWATWeather --region 'mississippi' --txtInOutDir ./TxtInOut --convertedDir ./NetCDF_Output --climateResolution 0.5
```

## License

See the [LICENSE](LICENSE) file for details.

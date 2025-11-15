# ArkGIS

A desktop GIS application developed based on Qt and QGIS, providing comprehensive geographic information data processing, analysis, and visualization capabilities.

## Project Introduction

ArkGIS is a feature-rich desktop Geographic Information System (GIS) application developed using the Qt framework and QGIS library. The application provides core GIS functionalities including map display, layer management, data editing, and spatial analysis, suitable for viewing, editing, and analyzing geographic data.

## Main Features

### Map Display and Interaction
- Map canvas display and interaction
- Map zooming, panning, and rotation
- Coordinate display and scale control
- Multiple coordinate system support (CRS)

### Layer Management
- **Vector Layers**: Support for Shapefile, GeoJSON, and other formats
- **Raster Layers**: Support for various raster data formats
- **Delimited Text Layers**: Support for CSV and other text format data
- Layer tree management (show/hide, order adjustment)
- Layer attribute table viewing
- Layer symbolization and style management

### Data Editing
- **Point Feature Editing**: Create, delete, and move point features
- **Line Feature Editing**:
  - Create and edit line features
  - Reverse line operation
  - Intersecting line cutting
  - Line feature simplification
  - Insert nodes
- **Polygon Feature Editing**:
  - Create and edit polygon features
  - Polygon merging
  - Polygon rotation
- **Editing Tools**:
  - Undo/Redo operations
  - Feature copying
  - Feature moving
  - Feature deletion
  - Attribute editing

### Spatial Analysis
- **Vector Analysis**:
  - K-means clustering analysis
  - Spatial join (join attributes by location)
  - Vector clipping (rectangle, circle, polygon)
  - Buffer analysis
  - Table to vector (Excel to Shapefile)
- **Raster Analysis**:
  - Raster data processing and analysis
  - Feature to raster (vector to raster conversion)

### Projection and Coordinate Systems
- Projection transformation
- Data frame coordinate system settings
- Multiple coordinate system support

### Symbols and Styles
- Symbol library management
- Custom symbol creation
- Categorized symbolization
- SLD style file support

### Project Management
- Open/save QGIS project files (.qgs)
- Project file management

## Technology Stack

- **Development Language**: C++
- **GUI Framework**: Qt 5.15.2
- **GIS Library**: QGIS LTR Development
- **Development Tools**: Visual Studio 2019/2022
- **Build System**: MSBuild
- **Third-party Libraries**:
  - GDAL (Geospatial Data Abstraction Library)
  - SQLite (Database)

## System Requirements

### Development Environment
- Windows 10/11 (x64)
- Visual Studio 2019 or later
- Qt 5.15.2 (msvc2019_64)
- QGIS LTR Development
- GDAL Development Libraries

### Runtime Environment
- Windows 10/11 (x64)
- Qt 5.15.2 runtime libraries
- QGIS related dependency libraries
- GDAL runtime libraries

## Project Structure

```
ArkGIS/
├── ArkGIS/                    # Main project directory
│   ├── main.cpp              # Program entry point
│   ├── mainwindow.h/cpp      # Main window class
│   ├── LayerManager.h/cpp    # Layer management class
│   ├── VectorAnalysis.h/cpp  # Vector analysis class
│   ├── RasterAnalysis.h/cpp  # Raster analysis class
│   ├── SymbolManager.h/cpp   # Symbol management class
│   ├── EditView.h/cpp        # Edit view class
│   ├── EditCommandManager.h/cpp  # Edit command management class
│   ├── VectorToRasterConverter.h/cpp  # Vector to raster converter
│   ├── ProjectionTransformDialog.h/cpp  # Projection transform dialog
│   ├── bufferdialog.h/cpp    # Buffer analysis dialog
│   ├── icons/                # Icon resources
│   ├── symbols/              # Symbol library files
│   ├── sld_data/             # SLD style files
│   ├── testdata/             # Test data
│   │   ├── shp/              # Shapefile test data
│   │   ├── geojson/          # GeoJSON test data
│   │   ├── project/          # Project files
│   │   └── wkt/              # WKT format data
│   └── thirdlib/             # Third-party libraries
│       ├── gdal-dev/         # GDAL development libraries
│       ├── qgis-ltr-dev/     # QGIS development libraries
│       ├── Qt5/              # Qt libraries
│       └── SQLite-3.46.0/    # SQLite library
├── QGISdev.sln               # Visual Studio solution file
└── README.md                 # Project documentation
```

## Compilation Instructions

### Prerequisites
1. Install Visual Studio 2019 or later
2. Install Qt 5.15.2 (msvc2019_64)
3. Configure QGIS LTR Development environment
4. Configure GDAL development libraries

### Compilation Steps
1. Open the `QGISdev.sln` solution file
2. Configure Qt and QGIS paths in project properties
3. Select build configuration (Debug or Release)
4. Build Solution

### Configuration Notes
In `main.cpp`, configure the QGIS path:
```cpp
QgsApplication::setPrefixPath("D:/OSGeo4W/apps/qgis-ltr-dev", true);
```
Modify according to the actual installation path.

## Usage Instructions

### Launching the Application
1. Run the compiled executable file
2. The application will display a random splash screen on startup
3. Begin using the application after entering the main interface

### Basic Operations
1. **Add Layers**: Add vector/raster layers through the file tree or menu
2. **Map Operations**: Use toolbar for zooming, panning, rotation, etc.
3. **Edit Data**: Click the edit button to enter edit mode, select layers for editing
4. **Spatial Analysis**: Select corresponding analysis functions through the menu bar
5. **Save Project**: Save the current project through the file menu

## Test Data

The project includes a test data directory `testdata/`, containing:
- Vector data in Shapefile format (Beijing, Wuhan, CUG, etc.)
- GeoJSON format data
- QGIS project file examples
- WKT format data

## License

This project is a course design project, for learning and research purposes only.

## Notes

1. Ensure QGIS and GDAL environments are properly configured before first use
2. Some features require specific data format support
3. Recommended to compile in Release mode for better performance
4. Save the project before editing operations to avoid data loss
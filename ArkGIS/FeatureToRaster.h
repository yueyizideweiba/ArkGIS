#ifndef FEATURETORASTER_H
#define FEATURETORASTER_H

#include <QObject>
#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QgsVectorLayer.h>
#include <QgsRasterLayer.h>
#include <QgsProject.h>
#include <QFileDialog>
#include <QMessageBox>
#include <QgsPolygon.h>
#include <QgsRasterFileWriter.h>
#include <QgsRasterDataProvider.h>
#include <QgsRasterPipe.h>
#include <QgsCoordinateReferenceSystem.h>
#include <QgsCoordinateTransformContext.h>
#include <QImage>
#include <QColor>
#include <cmath>
#include <gdal.h>
#include <gdal_alg.h>

class MainWindow;

class FeatureToRaster : public QDialog
{
	Q_OBJECT

public:
	explicit FeatureToRaster(QWidget* parent = nullptr);

	bool execute();

private slots:
	void updateFields();
	void selectOutputFile();
	void convertToRaster();

private:

	QComboBox* mLayerComboBox;
	QComboBox* mFieldComboBox;
	QLineEdit* mOutputFileLineEdit;
	QDoubleSpinBox* mCellSizeSpinBox;
	QPushButton* mBrowseButton;
	QPushButton* mOkButton;
	QPushButton* mCancelButton;

	bool convertFeatureToRaster(QgsVectorLayer* vectorLayer, const QString& fieldName, const QString& outputFilePath, double resolution);
	bool validateLayer(QgsVectorLayer* layer);

	QColor getColorForValue(const QVariant& value);
	void drawLineOnRaster(QImage& raster, const QgsPointXY& start, const QgsPointXY& end, double xmin, double ymax, double resolution, const QColor& color);
	void drawPolygonOnRaster(QImage& raster, const QgsPolylineXY& ring, double xmin, double ymax, double resolution, const QColor& color);

	MainWindow* mMainWindow;
};

#endif 

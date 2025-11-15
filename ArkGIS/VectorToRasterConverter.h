// vectorToRasterConverter.h
#ifndef VECTORTORASTERCONVERTER_H
#define VECTORTORASTERCONVERTER_H
// 前向声明 MainWindow
class MainWindow;

#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QgsVectorLayer.h>
#include <QgsRasterLayer.h>
#include <QgsProject.h>
#include <QgsMapCanvas.h>
#include <QgsCoordinateTransform.h>
#include <QgsRectangle.h>
#include <QgsField.h>
#include <QgsFields.h>
#include <QMainWindow>
#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include "mainwindow.h"

class VectorToRasterConverter {
public:
	VectorToRasterConverter(QgsMapCanvas* mapCanvas, MainWindow* mainWindow);

	void startConversion();

private:
	void selectVectorLayer();
	void selectField(QgsVectorLayer* vectorLayer);
	void convertToRaster(QgsVectorLayer* vectorLayer, int fieldIndex, const QString& outputPath);
	void addRasterLayer(const QString& rasterPath);

	QgsMapCanvas* mpMapCanvas;
	MainWindow* mMainWindow;

	class FieldSelectionDialog : public QDialog {
	public:
		FieldSelectionDialog(QgsVectorLayer* vectorLayer, QWidget* parent = nullptr);
		QString getOutputPath() const;
		int getFieldIndex() const;
		double getCellSizeX() const;
		double getCellSizeY() const;

	private:

		QListWidget* mFieldListWidget;
		QLineEdit* minputPathEdit;
		QLineEdit* mOutputPathEdit;
		QPushButton* mBrowseButton;
		QPushButton* mConvertButton;
		QPushButton* mCancelButton;
		int mFieldIndex;
		QString mOutputPath;
	};
};

#endif // VECTORTORASTERCONVERTER_H
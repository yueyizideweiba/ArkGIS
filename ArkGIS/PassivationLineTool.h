#ifndef PASSIVATIONLINETOOL_H
#define PASSIVATIONLINETOOL_H

#include <QgsMapTool.h>
#include <QgsMapCanvas.h>
#include <QgsVectorLayer.h>
#include <QgsFeature.h>
#include <QgsGeometry.h>
#include <QgsRubberBand.h>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QgsVectorFileWriter.h>
#include <QgsMapMouseEvent.h>
#include <QCoreApplication>

class PassivationLineTool : public QgsMapTool
{
	Q_OBJECT

public:
	PassivationLineTool(QgsMapCanvas* canvas);
	~PassivationLineTool();

private slots:
	void selectVectorFile();
	void selectOutputFile();
	void onFileSelected();
	void processPassivationLine();

signals:
	void finished();

private:
	QgsMapCanvas* mCanvas;
	QgsVectorLayer* mLayer;
	QgsRubberBand* mRubberBand;
	bool mIsPassivating;
	QDialog* mDialog;

	QLineEdit* lineEditVectorFile;
	QLineEdit* lineEditOutputFile;
	QPushButton* pushButtonSelectVectorFile;
	QPushButton* pushButtonSelectOutputFile;
};

#endif 
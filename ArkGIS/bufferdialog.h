#ifndef BUFFERDIALOG_H
#define BUFFERDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QgsVectorLayer.h>
#include <qgsmapcanvas.h>
#include <QCheckBox>

class BufferDialog : public QDialog
{
	Q_OBJECT

public:
	explicit BufferDialog(QgsMapCanvas* mapCanvas, QWidget* parent = nullptr);
	~BufferDialog();

private slots:
	void on_pushButtonSelectVectorFile_clicked();
	void on_pushButtonSelectOutputFile_clicked();
	void on_pushButtonGenerateBuffer_clicked();

private:
	QLineEdit* lineEditVectorFile;
	QLineEdit* lineEditOutputFile;
	QDoubleSpinBox* doubleSpinBoxBufferSize;
	QPushButton* pushButtonSelectVectorFile;
	QPushButton* pushButtonSelectOutputFile;
	QPushButton* pushButtonGenerateBuffer;
	QCheckBox* checkBoxMergeBuffers;
	QgsMapCanvas* mpMapCanvas; // 绘图画布
};

#endif // BUFFERDIALOG_H
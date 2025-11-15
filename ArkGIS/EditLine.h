#ifndef EDITLINE_H
#define EDITLINE_H

#include <QObject>
#include <QDialog>
#include <QString>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <vector>
#include <cmath>

// QGIS 相关类
#include <qgspoint.h>             // 完整点类型
#include <qgspointxy.h>           // 轻量级点类型
#include <qgsgeometry.h>          // 几何类
#include <qgsvectorlayer.h>       // 矢量图层
#include <qgsproject.h>           // QGIS 项目管理类
#include <qgsfeature.h>           // 特征操作
#include <qgsmaplayer.h>          // 地图图层类

// 前向声明 QGIS 类
class QgsMapLayer;

class EditLine : public QObject
{
	Q_OBJECT

public:
	explicit EditLine(QObject* parent = nullptr);

	// 执行线图层抽稀操作
	void executeSimplification(QWidget* parent = nullptr);

private:
	// 不同的抽稀算法
	void douglasPeucker(const QString& layerName);
	void topologySimplify(const QString& layerName);

	// 内嵌对话框类
	class EditLineDialog : public QDialog
	{
	public:
		explicit EditLineDialog(QWidget* parent = nullptr);

		QString getSelectedLayer() const; // 获取用户选择的图层
		QString getSelectedMethod() const; // 获取用户选择的抽稀方法

	private:
		void setupUI(); // 设置 UI 界面

		// 成员变量
		QComboBox* mLayerComboBox = nullptr;  // 图层选择下拉框
		QComboBox* mMethodComboBox = nullptr; // 抽稀方法选择下拉框
		QPushButton* mOkButton = nullptr;     // 确定按钮
		QPushButton* mCancelButton = nullptr; // 取消按钮
	};

public:
	// 道格拉斯普克算法函数
	double perpendicularDistance(const QgsPoint& point, const QgsPoint& lineStart, const QgsPoint& lineEnd);
	void douglasPeuckerRecursion(const std::vector<QgsPoint>& points, double tolerance, std::vector<QgsPoint>& outPoints);

	std::vector<QgsPoint> someMemberVariable; // 示例成员变量

	// 拓扑抽稀递归函数
	void topologicalDouglasPeuckerRecursion(const std::vector<QgsPoint>& points,
		double tolerance,
		std::vector<QgsPoint>& simplifiedPoints,
		QgsVectorLayer* layer);

private:
};

#endif // EDITLINE_H

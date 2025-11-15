/************************************************************
FileName: editview.h
Author: XYH
Version : 1.0
Date: 2024-10-24
Description: 编辑视图类，继承自QgsMapTool，用于实现地图编辑工具
Function List:
1. setLayer - 设置当前编辑图层和编辑模式
2. reset - 重置编辑状态
3. canvasReleaseEvent - 捕获鼠标释放事件，用于记录点坐标
4. canvasMoveEvent - 捕获鼠标移动事件，用于更新橡皮筋位置
5. canvasDoubleClickEvent - 捕获鼠标双击事件，用于结束线段绘制
6. createPoint - 创建点要素
7. addPointToLine - 添加点到线段列表
8. updateRubberBand - 更新橡皮筋的动态效果
9. finishLine - 完成线要素的创建
10. addPointToPolygon - 添加点到面顶点列表
11. finishPolygon - 完成面要素的创建
12. initializeAttributes - 初始化要素的属性字段
*************************************************************/

#ifndef EDITVIEW_H
#define EDITVIEW_H

#include <QgsMapTool.h>
#include <QgsVectorLayer.h>
#include <QgsFeature.h>
#include <QgsGeometry.h>
#include <QgsPointXY.h>
#include <QgsMapCanvas.h>
#include <QMouseEvent>
#include <QList>
#include <QgsMapMouseEvent.h>
#include <QMessageBox>
#include <QgsRubberBand.h>
#include <QgsFillSymbol.h>

#include <QMouseEvent> 
#include <QDialogButtonBox>
#include <QTextEdit>
#include <QDialog>
#include <QFormLayout> 
#include <QLineEdit> 
#include <QCheckBox>
#include <QPushButton> 
#include <QMessageBox>
#include <QMap>
#include <QVariant> 
#include <QString>
#include <QList>
#include <QInputDialog>
#include <QSpinBox>

class EditView : public QgsMapTool {
	Q_OBJECT

public:
	enum EditMode {
		None,
		PointMode,
		LineMode,
		PolygonMode,
		SelectAndAttriMode,
		MoveMode,
		CopyMode,
		InsertPointMode,

	};

	EditView(QgsMapCanvas* canvas, QObject* parent = nullptr);
	EditMode mEditMode;

	// 设置当前编辑图层
	void setLayer(QgsVectorLayer* layer, EditMode mode);

	// 重置编辑状态
	void reset();

protected:
	void canvasPressEvent(QgsMapMouseEvent* event)override;

	// 捕获鼠标点击事件，用于记录点坐标
	void canvasReleaseEvent(QgsMapMouseEvent* event) override;

	// 捕获鼠标移动事件，用于更新橡皮筋位置
	void canvasMoveEvent(QgsMapMouseEvent* event) override;

	// 捕获鼠标双击事件，用于结束线段绘制
	void canvasDoubleClickEvent(QgsMapMouseEvent* event) override;

private:
	QgsMapCanvas* mpCanvas;
	QgsVectorLayer* mCurrentLayer;
	QList<QgsPointXY> points;  // 用于存储线段的点列表
	QgsRubberBand* mpRubberBand;

	bool mSelectionActive = false;
	QPoint mInitDragPos;
	std::unique_ptr<QgsRubberBand> mSelectionRubberBand;
	QColor mFillColor = QColor(0, 255, 0, 50);   // 填充颜色（可自定义）
	QColor mStrokeColor = Qt::red;

	QgsFeature mMovingFeature;  // 当前正在移动的要素
	QgsPointXY mMoveStartPoint; // 移动开始时的坐标
	bool mIsMoving = false;     // 是否正在移动

	// 创建点要素
	void createPoint(const QgsPointXY& point);

	// 添加点到线段列表
	void addPointToLine(const QgsPointXY& point);

	// 在线要素或面要素中插入点
	void insertPointInLine(QgsGeometry& geom, const QgsPointXY& point);

	// 更新橡皮筋的动态效果
	void updateRubberBand(const QgsPointXY& tempPoint);

	// 完成线要素的创建
	void finishLine();

	// 添加点到面顶点列表
	void addPointToPolygon(const QgsPointXY& point);

	// 完成面要素的创建
	void finishPolygon();

	// 初始化要素的属性字段
	void initializeAttributes(QgsFeature& feature);

	void identifyFromGeometry(const QgsGeometry& geometry);
	void showFeatureAttributes(QgsFeature& feature);

	void editFeatureAttributes(QgsFeature& feature);

	void copyFeaturesArray(int rows, int cols, double rowSpacing, double colSpacing);
};

class GeometryRelation {
public:
	static bool pointIntersects(const QgsPointXY& point, const QgsGeometry& geometry) {
		QgsGeometry pointGeometry = QgsGeometry::fromPointXY(point);

		if (geometry.isNull()) {
			return false;
		}

		// 判断几何类型并处理相交逻辑
		if (geometry.type() == Qgis::GeometryType::Point) {
			// 点与点的相交
			return geometry.distance(pointGeometry) < 1.0;
		}
		else if (geometry.type() == Qgis::GeometryType::Line) {
			// 点与线的相交（允许一定的容差）
			return geometry.distance(pointGeometry) < 1.0; // 距离阈值可调整
		}
		else if (geometry.type() == Qgis::GeometryType::Polygon) {
			// 点与面的相交
			return geometry.intersects(pointGeometry);
		}

		return false;
	}

	static double distanceToGeometry(const QgsPointXY& point, const QgsGeometry& geometry) {
		QgsGeometry pointGeometry = QgsGeometry::fromPointXY(point);

		if (geometry.isNull()) {
			return std::numeric_limits<double>::max();
		}

		// 计算点到几何体的距离
		return geometry.distance(pointGeometry);
	}
};


#endif
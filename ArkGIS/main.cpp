#include "mainwindow.h"
#include "SymbolLibraryManager.h"
#include <QtWidgets/QApplication>
#include <QSplashScreen>
#include <QPixmap>
#include <QTimer>
#include <QRandomGenerator>
#include <qgsapplication.h>


int main(int argc, char* argv[])
{
	QgsApplication app(argc, argv, false);

	// 初始化符号库管理器
	SymbolLibraryManager::instance().initialize();

	QgsApplication::setPrefixPath("D:/OSGeo4W/apps/qgis-ltr-dev", true);
	QgsApplication::init();
	QgsApplication::initQgis();
	app.initQgis();  // 初始化QGIS
	QgsApplication::setThemeName("Blend of Gray");  // 加载默认主题

	// 设置应用程序图标
	app.setWindowIcon(QIcon("./icons/1.ico"));

	// 所有启动页图像文件
	QStringList splashImages = {
		"./icons/openpage/1.png",
		"./icons/openpage/2.png",
		"./icons/openpage/3.png",
		"./icons/openpage/4.png",
		"./icons/openpage/5.png",
		"./icons/openpage/6.png",
		"./icons/openpage/7.png",
		"./icons/openpage/8.png"
	};

	// 随机选择一张启动页图像
	int randomIndex = QRandomGenerator::global()->bounded(splashImages.size());
	QPixmap pixmap(splashImages[randomIndex]);

	// 调整启动页图像大小
	QSize targetSize(1500, 750);
	pixmap = pixmap.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

	// 创建启动页
	QSplashScreen splash(pixmap);
	splash.show();

	QTimer::singleShot(3000, &splash, &QSplashScreen::close); // 3秒后关闭启动页

	MainWindow mainWindow;
	QTimer::singleShot(1000, &mainWindow, &QMainWindow::show); // 1秒后显示主窗口

	return app.exec();
}

#ifndef EDITCOMMANDMANAGER_H
#define EDITCOMMANDMANAGER_H

#include <QObject>
#include <QString>
#include <QgsVectorLayer.h>

class EditCommandManager : public QObject
{
	Q_OBJECT

public:
	explicit EditCommandManager(QgsVectorLayer* layer, QObject* parent = nullptr);

	void beginCommand(const QString& description);
	void endCommand();
	void undo();
	void redo();

signals:
	void stateChanged(bool canUndo, bool canRedo);

private:
	QgsVectorLayer* mLayer;
	QString mCurrentCommand;
};

#endif 

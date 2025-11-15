#include "EditCommandManager.h"

EditCommandManager::EditCommandManager(QgsVectorLayer* layer, QObject* parent)
	: QObject(parent), mLayer(layer)
{
}

void EditCommandManager::beginCommand(const QString& description)
{
	if (mLayer && mLayer->isEditable()) {
		mCurrentCommand = description;
		mLayer->beginEditCommand(mCurrentCommand);
	}
}

void EditCommandManager::endCommand()
{
	if (mLayer && mLayer->isEditable()) {
		mLayer->endEditCommand();
		emit stateChanged(mLayer->undoStack()->canUndo(), mLayer->undoStack()->canRedo());
	}
}

void EditCommandManager::undo()
{
	if (mLayer && mLayer->undoStack()->canUndo()) {
		mLayer->undoStack()->undo();
		emit stateChanged(mLayer->undoStack()->canUndo(), mLayer->undoStack()->canRedo());
	}
}

void EditCommandManager::redo()
{
	if (mLayer && mLayer->undoStack()->canRedo()) {
		mLayer->undoStack()->redo();
		emit stateChanged(mLayer->undoStack()->canUndo(), mLayer->undoStack()->canRedo());
	}
}

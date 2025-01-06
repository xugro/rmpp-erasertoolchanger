#pragma once

#include <QPointer>
#include "eraserhandler.h"


void setHandlerPointer(QPointer<EraserHandler> &other);

extern "C" {

	int connectionStart(const char *filePath, const char *mode, int flags, int modeI);

	void startEventPipe();

}

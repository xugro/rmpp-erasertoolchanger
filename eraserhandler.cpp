#include "eraserhandler.h"
#include "eventpipe.h"
#include <atomic>
#include "debug.h"

extern volatile std::atomic_bool disabled;


void EraserHandler::cancelOverride(){
	disabled = true;
}


void EraserHandler::overrideEraser(){
	disabled = false;
}

void EraserHandler::completed(){
	LOGF("\033[0;33mEraserHandler:\033[0m handler loaded\n");
	QPointer<EraserHandler> p = this;
	setHandlerPointer(p);
}

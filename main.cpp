#include <QQmlEngine>
#include "eventpipe.h"
#include "eraserhandler.h"


extern const struct XoViEnvironment {
    char *(*getExtensionDirectory)(const char *family);
    void (*requireExtension)(const char *name, unsigned char major, unsigned char minor, unsigned char patch);
} *Environment;

extern "C"{
	void _xovi_construct(){
		Environment->requireExtension("qt-resource-rebuilder", 0, 2, 0);
		Environment->requireExtension("fileman", 0, 1, 0);

		qmlRegisterType<EraserHandler>("net.xugro.EraserHandler", 1, 0, "EraserHandler");
		qt_resource_rebuilder$qmldiff_add_external_diff(r$eraserDiff, "eraser tool changer");

		startEventPipe();
	}
}

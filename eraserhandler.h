#pragma once

#include <QObject>

class EraserHandler : public QObject {

	Q_OBJECT

	public:

	signals:

		void eraserDown();
		void eraserUp();

	public slots:

		void overrideEraser();
		void cancelOverride();
		void completed();
};

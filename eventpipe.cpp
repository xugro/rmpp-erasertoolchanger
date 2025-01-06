#include <unistd.h>
#include <fcntl.h>

#include <cstdlib>
#include <cstdio>
#include <thread>
#include <vector>
#include <iostream>
#include <string>
#include <atomic>

#include <QPointer>
#include "eraserhandler.h"
#include <QObject>

#include "file.h"
#include "debug.h"

#include "eventpipe.h"

extern "C"{ //put these in extern C to use C style structs
#include <linux/input.h> // input_event type definition
#include <linux/input-event-codes.h>

#include <string.h>

#include "fileman.h" //TODO: make this file a submodule if you put this on git
}
/* these are #defined in input-event-codes.h
** Event types:
	EV_SYN = 0
	EV_KEY = 1
	EV_ABS = 3

** Event codes:
	BTN_TOOL_PEN = 320
	BTN_TOOL_RUBBER = 321
	BTN_TOUCH = 330
	BTN_STYLUS = 331
	BTN_STYLUS2 = 332

	ABS_X = 0
	ABS_Y = 1
	ABS_PRESSURE = 24	//NOTE: the pen has pressure sensitivity but the eraser does not
	ABS_DISTANCE = 25
	ABS_TILT_X   = 26
	ABS_TILT_Y   = 27
*/

volatile std::atomic_bool disabled = 1;
QPointer<EraserHandler> handlerPointer;

int real_BTN_TOOL_PEN    = 0;
int real_BTN_TOOL_RUBBER = 0;
int fake_BTN_TOOL_PEN    = 0;
int fake_BTN_TOOL_RUBBER = 0;

void log_values(){
	// LOGF("\033[0;31mpen_eraser\033[0m: ");// TODO: make a log function that prints this line and can be disabled with defines
	LOGS << "real_pen: " << real_BTN_TOOL_PEN << " real_eraser: " << real_BTN_TOOL_RUBBER << " fake_pen: " << fake_BTN_TOOL_PEN << " fake_eraser: " << fake_BTN_TOOL_RUBBER << std::endl;
}

std::string key_code(int code){
	switch(code){
		case BTN_TOOL_PEN:
			return "BTN_TOOL_PEN   ";
		case BTN_TOOL_RUBBER:
			return "BTN_TOOL_RUBBER";
		case BTN_TOUCH:
			return "BTN_TOUCH      ";
		case BTN_STYLUS:
			return "BTN_STYLUS     ";
		case BTN_STYLUS2:
			return "BTN_STYLUS2    ";
	}

	return (std::string) "unknown_code: " + std::to_string(code);
}

void print_event(input_event &ev){
	LOGF("\033[0;31mpen_eraser\033[0m: ");
	//LOGF("type: %hu, code: %hu, value: %d\n", ev.type, ev.code, ev.value);
	
	switch(ev.type){
		case EV_SYN:
			LOGF("sync event (end of packet)\n");
			break;
		case EV_KEY:
			LOGS << "key: " << key_code(ev.code) << "\tvalue: " << ev.value << std::endl;
			break;
		case EV_ABS:
			LOGF("code: %hu\tvalue: %d\n", ev.code, ev.value);
			break;
	}

}

void processEvents(std::vector<input_event> &packet, int &pipewriteFD /*for writing skipping some events (see NOTE below in pipeHandler)*/){

	int len = packet.size();

	bool update_PEN = 0;
	bool update_RUBBER = 0;
	int new_value_PEN = 0;
	int new_value_RUBBER = 0;

	int ind_penupdate = -1;
	int ind_eraserupdate = -1;

	for(int i=0; i < len; i++){
		if( packet[i].type == EV_KEY && packet[i].code == BTN_TOOL_PEN ){
			update_PEN = true;
			new_value_PEN = packet[i].value;
			ind_penupdate = i;

			LOGF("updating pen (event %d): %d\n", i, packet[i].value);
		}
		if( packet[i].type == EV_KEY && packet[i].code == BTN_TOOL_RUBBER ){
			update_RUBBER = true;
			new_value_RUBBER = packet[i].value;
			ind_eraserupdate = i;
			LOGF("updating rubber (event %d): %d\n", i, packet[i].value);
		}
	}

	if(!update_RUBBER && !update_PEN) return;

	//LOGS << "update_PEN: " << update_PEN << " new_value_PEN: " << new_value_PEN << " update_RUBBER: " << update_RUBBER << " new_value_RUBBER: " << new_value_RUBBER << std::endl;

	if(!update_RUBBER){
		if(update_PEN){
			real_BTN_TOOL_PEN = new_value_PEN;
			fake_BTN_TOOL_PEN = new_value_PEN;
			LOGF("updated pen only\n");
		}
		return; //TODO: make this also call a signal to make sure the pen was restored
	}

	/*	there are 3 stages:			BTN_TOOL_PEN	BTN_TOOL_RUBBER
	 *	stage 0(before any eraser action): 	1		0		(this is just a regular pen event)
	 *	stage 1(eraser is detected):		0*		1*	<- sets both values
	 *	stage 2(eraser is lifted):		0		0*	<- sets onlt rubber
	 *
	 *
	 */

	if(!update_PEN){ //stage 2

		if(real_BTN_TOOL_RUBBER == 1 && real_BTN_TOOL_PEN == 0 &&  new_value_RUBBER == 0 && fake_BTN_TOOL_PEN == 1 && fake_BTN_TOOL_RUBBER == 0){
			packet[ind_eraserupdate].code = BTN_TOOL_PEN;
			fake_BTN_TOOL_PEN = 0;
			real_BTN_TOOL_RUBBER = 0;
			LOGF("lifted eraser\n");
			handlerPointer -> eraserUp();
			return;
		}

		LOGF("pen not updated but rubber updated\n");

		
	}

	if(real_BTN_TOOL_RUBBER == 0 && real_BTN_TOOL_PEN == 1 && new_value_RUBBER == 1 && new_value_PEN == 0){ // stage 1

		real_BTN_TOOL_RUBBER = 1;
		real_BTN_TOOL_PEN = 0;
		fake_BTN_TOOL_RUBBER = 0;
		fake_BTN_TOOL_PEN = 1;

		int packet_size = packet.size();
		for(int i=0; i < packet_size; i++){
			if(i == ind_penupdate || i == ind_eraserupdate) continue;
			print_event(packet[i]);
			void *buf;
			buf = &(packet[i]);
			write(pipewriteFD, buf, sizeof(packet[i]));
		}
		packet.clear();

		LOGF("\033[0;32mpen_eraser\033[0m: ");
		LOGF("switched from pen to rubber\npenindex: %d, rubberindex: %d\n", ind_penupdate, ind_eraserupdate);

		handlerPointer -> eraserDown();
		return;
	}

	//should not get here
	std::cerr << "\033[0;31mpen_eraser\033[0m: invalid state in pen_eraser\n";


	return;
}

void pipehandler(int pipereadFD, int pipewriteFD, int realFD){

	auto pid = std::this_thread::get_id();
	int raw_inputFD = $open(DIGITIZER_PATH, O_RDONLY);


	LOGF("\033[0;31mpen_eraser\033[0m: ");
	LOGS << "[" << pid << "]";
	LOGF("handler started, pipereadFD: %d, pipewriteFD: %d, realFD: %d, raw_inputFD: %d\n", pipereadFD, pipewriteFD, realFD, raw_inputFD);

	LOGF("\033[0;31mpen_eraser\033[0m: ");

	if(raw_inputFD == -1){
		std::cerr << "could not open input FD" << std::endl;
		return;
	}

	LOGS << "[" << pid << "]";
	LOGF("opened digitizer, sleeping\n");

	sleep(10); //idk why but asivery waited in his palm-rejection plugin and nothing bad happens for waiting

	LOGF("\033[0;31mpen_eraser\033[0m: ");
	LOGS << "[" << pid << "]";
	LOGF("woke up\n");
	int e;
	e = dup2(pipereadFD, realFD); //realFD is no longer reading from the file but from our pipe
	if(e == -1){
		std::cerr << "ERROR: could not replace realFD\n";
		return;	
			//TODO: check docs for behaviour if realFD still works after returning
	}

	$close(pipereadFD);

	LOGF("\033[0;31mpen_eraser\033[0m: ");
	LOGS << "[" << pid << "]";
	LOGF("replaced digitizer file descriptor with own pipe's output\n");

	input_event input;

	std::vector<input_event> eventPacket;
	eventPacket.reserve(8); //the most amount of changes in one packet I saw was 7 (including sync) but using vector here just to be sure 
				//reserved so it shouldn't need re-allocation mid-event
				//
				//when the eraser is close to the screen, the digitizer assumes the pen tip is hovering up to a point
				//then, it detects the eraser and sends BTN_PEN=0, BTN_RUBBER=1 signals in the same packet,
				//sending two BTN_PEN signals might confuse the software so we need to look at the whole packet
				//
				//the protocol specifies a change can be sent when the value isn't changed, so sending a BTN_RUBBER=0 signal when it already is zero should not cause a problem
				//NOTE: This was a wrong assumption, xochitl just toggles the state regardless.

	int read_cnt = 0;
	LOGF("\033[0;31mpen_eraser\033[0m: ");
	LOGS << "[" << pid << "]";
	LOGF("about to enter loop, pipewriteFD: %d, realFD: %d, rawFD: %d\n", pipewriteFD, realFD, raw_inputFD);
	while(true){
		read_cnt = read(raw_inputFD, &input, sizeof(input) );
		if( read_cnt < sizeof(input) ){
			LOGS << "ERROR: could not read event\n";
			return; //TODO: skip event maybe?
		}

		eventPacket.push_back(input);

		if(input.type != EV_SYN){ // EV_SYN denotes the end of a packet
			continue;
		}

		if(!disabled || real_BTN_TOOL_PEN || real_BTN_TOOL_RUBBER)// disable after pen is lifted to keep the values correct TODO: handle being enabled while pen is close to screen
			processEvents(eventPacket, pipewriteFD); //everything other than this line in the function just reads events and writes them to the program without altering
		LOGS << "[" << pid << "]";
		log_values();

		LOGF("fd: %d event_count: %ld\n", pipewriteFD, eventPacket.size());

		int packet_size = eventPacket.size();
		for(int i=0; i < packet_size; i++){
			print_event(eventPacket[i]);
			void *buf;
			buf = &(eventPacket[i]);
			write(pipewriteFD, buf, sizeof(eventPacket[i]));
		}
		eventPacket.clear(); //this leaves the capacity unchanged so hopefully no unnecessary re-allocations
	}

	std::cerr << "ERROR: exited loop, xochitl will hang or crash\n";
	return;
}

void setHandlerPointer(QPointer<EraserHandler> &other){
	disabled = 1;
	handlerPointer = QPointer(other);
	disabled = 0;
}


extern "C" {


int connectionStart(const char *filePath, const char *mode, int flags, int modeI){
	LOGF("\033[0;31mpen_eraser\033[0m: ");
	LOGF("open digitizer was called\nfilePath = %s\nmode = %s\nflags = %d\nmodeI = %d\n", filePath, mode, flags, modeI);

	if(flags & O_CLOEXEC){
		LOGF("\033[0;31mpen_eraser\033[0m: O_CLOEXEC was set, no sense in replacing this FD\n");

		return $open(filePath, flags);

	} // */


	int pipes[2];
	pipe(pipes);
	// 0 = read/output end , 1 = write/input end
	
	int realFile = $open(DIGITIZER_PATH, O_RDONLY);

	LOGF("\033[0;31mpen_eraser\033[0m: ");
	LOGF("realfile descriptor: %d, pipe write: %d\n", realFile, pipes[1]);

	std::thread handlerThread(pipehandler, pipes[0], pipes[1], realFile);
	LOGF("\033[0;31mpen_eraser\033[0m: ");
	LOGF("started handlder thread pipewrite: %d\n", pipes[1]);
	handlerThread.detach();
	LOGF("\033[0;31mpen_eraser\033[0m: ");
	LOGF("detached handler thread pipewrite: %d\n", pipes[1]);

	return realFile;
}

void startEventPipe(){

	LOGF("\033[0;31mpen_eraser\033[0m: ");
	LOGF("loading pen_eraser\n");
	FilemanOverride *fo = (FilemanOverride *) malloc( sizeof(FilemanOverride));

	fo -> name =		strdup(DIGITIZER_PATH);//to explicity make this a regular C string
	fo -> nameMatchType =	FILEMAN_MATCH_WHOLE;
	fo -> handlerType =	FILEMAN_HANDLE_FD_MAP;
	fo -> handler.fdRemap = connectionStart;

	LOGF("\033[0;31mpen_eraser\033[0m: registering override\n");
	fileman$registerOverride(fo);
	LOGF("\033[0;32mpen_eraser\033[0m: registered override\n");
	return;
}

}

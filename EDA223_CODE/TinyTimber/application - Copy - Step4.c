#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"

#include <stdio.h>
#include <stdlib.h>

/************************************************
 ***** VOLUME GENERATOR
 *************************************************/

char* const DAC_SOUND_REG = (char *) 0x4000741C;

int bench = 0;

typedef struct {
	Object super;
	int deadline;
	int volume;
	int pulse;
	int freq;
} ToneGen;

typedef struct {
    Object super;
    int isToneGenRunning;
	int isBenchRunning;
	int isBackgrLoadRunning;
} App;

typedef struct {
	Object super;
	int deadline;
	int period; 
	int background_loop_range;
} BackgrLoad;

void reader(App*, int);
void receiver(App*, int);

App app = { initObject(), 0, 0, 0};
ToneGen tonegen = {initObject(), 100, 5, 1, 500 };
BackgrLoad backgrl = {initObject(), 1300, 1300, 1000};

Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);

void backgr_load(BackgrLoad * self, int unused){
	if (!bench) SEND(USEC(self->period), USEC(self->deadline),self,  backgr_load, NULL);
	for (int i=0; i<self->background_loop_range; i++);
	
}

//Function generating tone
void tone_generator(ToneGen *self, int unused){
	/* To produce audible output from 1kHz wave we need to:
	 * - have wave split on two half-periods of 500Hz
	 * - at next periods volume is multiplied by pulse
	 * - pulse switch between 1 and 0
	 * - so we have high and low volume respectively
	 * - that gives audible output
	 */	
	 
	 //after 500[us] run tone_generator again with 1/0 pulse
	if( !bench) SEND(USEC(self->freq), USEC(self->deadline),self,  tone_generator, NULL);
	
	//send signal to DAC with high/low volume
	*DAC_SOUND_REG = self->volume && self->pulse;
	//switch pulse to 1/0
	self->pulse = !self->pulse;
	
}

//Function for muting volume
void sound_mutter(ToneGen *self, int arg){
	self->volume = 0;
	
	char buf[100];
	sprintf(buf,"volume = %d\n",self->volume);
	SCI_WRITE(&sci0, buf);
}

//Function for unmute volume
void sound_unmutter(ToneGen *self, int arg){
	self->volume = 5;
		
	char buf[100];
	sprintf(buf,"volume = %d\n",self->volume);
	SCI_WRITE(&sci0, buf);
}

//Function for increasing volume
void volume_up(ToneGen *self, int arg){
	if (self->volume >= 20) {
		self->volume = 20;
	} else {
		self->volume += 1;
	}
		
	char buf[100];
	sprintf(buf,"volume = %d\n",self->volume);
	SCI_WRITE(&sci0, buf);
}

//Function for decreasing volume
void volume_down(ToneGen *self, int arg){
	if (self->volume <= 0) {
		self->volume = 0;
	} else {
		self->volume -= 1;
	}
	
	char buf[100];
	sprintf(buf,"volume = %d\n",self->volume);
	SCI_WRITE(&sci0, buf);
}


//Function for decreasing volume
void change_period(ToneGen *self, int arg){
	
	if (self->freq == 500) self->freq = 650;
	else if (self->freq == 650) self->freq = 931;
	else if (self->freq == 931) self->freq = 500;

	char buf[100];
	sprintf(buf,"freq = %d\n",self->freq);
	SCI_WRITE(&sci0, buf);
}

void load_add(BackgrLoad * self, int arg){
		self->background_loop_range += 500;
	
	
	char buf[100];
	sprintf(buf,"load = %d\n",self->background_loop_range);
	SCI_WRITE(&sci0, buf);
}

void load_remove(BackgrLoad * self, int arg){
	
	if (self->background_loop_range <= 0){
		self->background_loop_range = 0;
	} else {
		self->background_loop_range -= 500;
	}
	
	char buf[100];
	sprintf(buf,"load = %d\n",self->background_loop_range);
	SCI_WRITE(&sci0, buf);
}


void change_deadline(ToneGen * self, int arg){
	
	if (self->deadline){
		self->deadline = 0;
	} else {
		self->deadline = arg;
	}
	
	char buf[100];
	sprintf(buf,"deadline = %d\n",self->deadline);
	SCI_WRITE(&sci0, buf);
}



void receiver(App *self, int unused) {
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
    SCI_WRITE(&sci0, "Can msg received: ");
    SCI_WRITE(&sci0, msg.buff);
}

void run_tone_generator(App *self, int c){
	if (self->isToneGenRunning) {
		SCI_WRITE(&sci0, "Already is running \n");
	} else {
		SCI_WRITE(&sci0, "Starting tone generator \n");
		ASYNC(&tonegen, tone_generator, NULL);
		self->isToneGenRunning = 1;
	}
}

void run_backgr_load(App *self, int c){
	if (self->isBackgrLoadRunning) {
		SCI_WRITE(&sci0, "Already is running \n");
	} else {
		SCI_WRITE(&sci0, "Starting dummy load \n");
		ASYNC(&backgrl, backgr_load, NULL);
		self->isBackgrLoadRunning = 1;
	}
}

void run (App* self, int arg) {
	SCI_WRITE (&sci0, "\nRun\n");

	//initialize tone generator in parallel, sync will start nothing
	ASYNC(&app, run_tone_generator, NULL);
	ASYNC(&app, run_backgr_load, NULL);


}
#define BENCH_TIME 500

#define BENCHMARK (obj, task, arg){          \
	float d, avg, max = 0;\
	Time t1, t2, t3, t4; \
	for (int i = 0; i < BENCH_TIME; i++) { \
		t1 = CURRENT_OFFSET();\
		task (obj, arg);  \
		t2 = CURRENT_OFFSET();\
		float d = (t2 - t1) * 10.0;\
		if (d > max) max = d; \
	}\
	t3 = CURRENT_OFFSET(); \
	for (int i= 0; i< BENCH_TIME; i++) { \
		task( obj, avg); \
	}\
	t4 = CURRENT_OFFSET(); \
	avg = (t4 - t3) /50.0; \
	char buf [40]; \
	sprintf(buf, "max = %d.%04d, avg = %d.%04d\n", \
	(int)max, ((int) (max* 100.0)) %100, \
	(int) avg,	((int) (avg *100.0)) % 100); \
	SCI_WRITE(&sci0, buf); \	
}
	

void benchmark (App* self, int arg) {
	if (self->isBenchRunning) {
		SCI_WRITE(&sci0, "Error: Benchmark already running\n");
	} else {
		self->isBenchRunning = 1;
		
		SCI_WRITE(&sci0, "Starting benchmark\n");
		bench = 1;
		
		SCI_WRITE(&sci0, "Benchmark tone generator: ");
		BENCHMARK(&tonegen, tone_generator, 0);
		
		SCI_WRITE(&sci0, "Benchmark background load = ");
		char buf [20];
		sprintf(buf, "%d\n", backgrl.load);
		SCI_WRITE(%sci0, buf);
		
		BENCHMARK (&backgrl, backgr_load, 0);
		
		backgrl.load = 13500;
		
		SCI_WRITE(&sci0, "Benchmark background load = ");
		sprintf(buf, "%d\n", backgrl.load);
		SCI_WRITE(%sci0, buf);
		
		BENCHMARK (&backgrl, backgr_load, 0);

		SCI_WRITE (&sci0, "Benchmark complete\n");
		
		bench = 0;
		
}

void reader(App *self, int c) {
	
	SCI_WRITE(&sci0, "Rcv: \'"); // echo the keyboard input
    SCI_WRITECHAR(&sci0, c);
    SCI_WRITE(&sci0, "\'\n");
		
	if (c == 'm') { // do SYNC to run after current task
		SYNC(&tonegen, sound_mutter, 0);
	} else if (c == 'j') {
		SYNC(&tonegen, volume_up, 0);
	} else if(c == 'k') {
		SYNC(&tonegen, volume_down, 0);
	} else if (c == 'u') {
		SYNC(&tonegen, sound_unmutter, 0);
	} else if (c == 'l') {
		SYNC(&backgrl, load_add, 0);
	} else if (c == 'r') {
		SYNC(&backgrl, load_remove,0 );
	} else if (c == 'n') {
		SYNC(&tonegen, change_period,0);
	} else if (c == 'd'){
		ASYNC(&tonegen, change_deadline,100);
		ASYNC(&backgrl, change_deadline,1300);
	} else if (c == 'a') {
		run(&app, 0);
	} else if (c == 'b') 
		benchmark(&app, 0);
	} else {
		SCI_WRITE(&sci0, "Ignoring key input \n");
	}
	
}

void begin(App *self, int unused){
	CAN_INIT(&can0);
	SCI_INIT(&sci0);
	SCI_WRITE(&sci0, "Initializing tone generator \n");
	
}


int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, begin, 0);
    return 0;
}
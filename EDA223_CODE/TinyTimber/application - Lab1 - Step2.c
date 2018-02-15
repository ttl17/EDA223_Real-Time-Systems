#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"

#include <stdio.h>
#include <stdlib.h>

/************************************************
 ***** VOLUME GENERATOR
 *************************************************/

char* const DAC_SOUND_REG = (char *) 0x4000741C;

typedef struct {
	Object super;
	int volume;
	int pulse;
	int freq;
	int freqNo;
} ToneGen;

typedef struct {
    Object super;
    int isToneGenRunning;
	int isDummyLoadRunning;
} App;

typedef struct {
	Object super;
	int period; 
	int background_loop_range;
} DummyLoad;

void reader(App*, int);
void receiver(App*, int);

App app = { initObject(), 0 };
ToneGen tonegen = {initObject(), 5, 1, 500, 1};
DummyLoad dumload = {initObject(), 1300, 1000};

Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);

void dummy_load(DummyLoad * self, int unused){
	AFTER(USEC(self->period), self, dummy_load, NULL);
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
	AFTER(USEC(self->freq), self, tone_generator, NULL);
	//send signal to DAC with high/low volume
	*DAC_SOUND_REG = self->volume * self->pulse;
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
	
	if (self->freq == 500) self->freq = 769;
	else if (self->freq == 769) self->freq = 537;
	else if (self->freq == 537) self->freq = 500;

	char buf[100];
	sprintf(buf,"freq = %d\n",self->freq);
	SCI_WRITE(&sci0, buf);
}

void load_increase(DummyLoad * self, int arg){
	if (self->background_loop_range >= 8000){
		self->background_loop_range = 8000;
	} else {
		self->background_loop_range += 500;
	}
	
	char buf[100];
	sprintf(buf,"load = %d\n",self->background_loop_range);
	SCI_WRITE(&sci0, buf);
}

void load_decrease(DummyLoad * self, int arg){
	
	if (self->background_loop_range <= 1000){
		self->background_loop_range = 1000;
	} else {
		self->background_loop_range -= 500;
	}
	
	char buf[100];
	sprintf(buf,"load = %d\n",self->background_loop_range);
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

void run_dummy_load(App *self, int c){
	if (self->isDummyLoadRunning) {
		SCI_WRITE(&sci0, "Already is running \n");
	} else {
		SCI_WRITE(&sci0, "Starting dummy load \n");
		ASYNC(&dumload, dummy_load, NULL);
		self->isDummyLoadRunning = 1;
	}
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
		SYNC(&dumload, load_increase, 0);
	} else if (c == 'r') {
		SYNC(&dumload, load_decrease,0 );
	} else if (c == 'n') {
		SYNC(&tonegen, change_period,0);
	} else {
		SCI_WRITE(&sci0, "Ignoring key input \n");
	}
	
}

void begin(App *self, int unused){
	CAN_INIT(&can0);
	SCI_INIT(&sci0);
	SCI_WRITE(&sci0, "Initializing tone generator \n");
	//initialize tone generator in parallel, sync will start nothing
	ASYNC(&app, run_tone_generator, NULL);
	ASYNC(&app, run_dummy_load, NULL);
}


int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, begin, 0);
    return 0;
}
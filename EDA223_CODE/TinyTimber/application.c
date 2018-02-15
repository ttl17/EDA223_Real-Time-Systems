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
	int deadline;
	int volume;
	int pulse;
	int freq;
} ToneGen;

typedef struct {
    Object super;
    int isToneGenRunning;
	int isBackgrLoadRunning;
} App;

typedef struct {
	Object super;
	int deadline;
	int period; 
	int tune;
} MelodyPlayer;

void reader(App*, int);
void receiver(App*, int);

App app = { initObject(), 0 };
ToneGen tonegen = {initObject(), 100, 5, 1, 500 };
BackgrLoad backgrl = {initObject(), 1300, 1300, 1000};

Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);

void backgr_load(BackgrLoad * self, int unused){
	SEND(USEC(self->period), USEC(self->deadline),self,  backgr_load, NULL);
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
	SEND(USEC(self->freq), USEC(self->deadline),self,  tone_generator, NULL);
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
	
	if (self->freq == 500) self->freq = 650;
	else if (self->freq == 650) self->freq = 931;
	else if (self->freq == 931) self->freq = 500;

	char buf[100];
	sprintf(buf,"freq = %d\n",self->freq);
	SCI_WRITE(&sci0, buf);
}

void load_add(BackgrLoad * self, int arg){
	if (self->background_loop_range >= 8000){
		self->background_loop_range = 8000;
	} else {
		self->background_loop_range += 500;
	}
	
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
	ASYNC(&app, run_backgr_load, NULL);
}


int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, begin, 0);
    return 0;
}
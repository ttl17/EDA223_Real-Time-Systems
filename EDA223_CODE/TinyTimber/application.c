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
	int period;
} ToneGen;

typedef struct {
    Object super;
    int isToneGenRunning;
	int isMelodyPlayerRunning;
} App;

typedef struct {
	Object super;
	int deadline; //D = 100ms, may be equal or smaller to period
	int period; //T = beat*tempo
	int tempo; //factor a,b,c
	int beat; //default a = 500ms
	int idx; //select tempo from arr
	int key; //change tune
} MelodyPlayer;

void reader(App*, int);
void receiver(App*, int);

App app = { initObject(), 0 };
ToneGen tonegen = {initObject(), 100, 5, 1, 500 };
MelodyPlayer melplay = {initObject(),50, 500, 2, 250, 0, 0};

Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);

#define IDX2FREQ(k) 440*pow(2,k/12.)
#define FREQ2PER(f) 1/(2*f)
#define BPM2MS(b) 60000/b

#define MIN_INDEX -10
#define MAX_INDEX 14
int tune[32] = {0, 2, 4, 0, 0, 2, 4, 0, 4, 5, 7, 4, 5, 7, 7, 9, 7, 5, 4, 0, 7, 9, 7, 5, 4, 0, 0, -5, 0, 0, -5, 0};
int tempo[32] = {2, 2, 2, 2, 
				2, 2, 2, 2, 
				2, 2, 4, 2, 
				2, 4, 1, 1, 
				1, 1, 2, 2, 
				1, 1, 1, 1, 
				2, 2, 2, 2,
				4, 2, 2, 4};
				
int period[25] = {2024, 1911, 1803, 1702, 1607, 1516, 1431, 1351, 1275, 1203, 1136,1072, 1012,  955,  901,
  851,  803,  758,  715,  675,  637,  601,  568,  536,  506};//14-(-10)+1

void get_brother_john_tune(ToneGen *tonegen, MelodyPlayer * melplay) {
	
	int k = melplay->key - MIN_INDEX;
	
	melplay->tempo = tempo[melplay->idx];
	
	//&tonegen->period=;
	melplay->period=melplay->beat*melplay->tempo;
	tonegen->period=period[tune[melplay->idx] + k];
	
	
	
	
}



//Function generating tone
void tone_generator(ToneGen *self, int doWork){
	/* To produce audible output from 1kHz wave we need to:
	 * - have wave split on two half-periods of 500Hz
	 * - at next periods volume is multiplied by pulse
	 * - pulse switch between 1 and 0
	 * - so we have high and low volume respectively
	 * - that gives audible output
	 */
	 
	//if (!doWork){
	//	ABORT();
	//} else {
		//after 500[us] run tone_generator again with 1/0 pulse
		//SEND(USEC(self->period), USEC(self->deadline),self,  tone_generator, doWork);
		SEND(USEC(self->period), USEC(self->period),self,  tone_generator, doWork);
		
		//send signal to DAC with high/low volume
		*DAC_SOUND_REG = self->volume * self->pulse;
		//switch pulse to 1/0
		
		self->pulse = !self->pulse;
		
	//}
	
	
}



//Function for muting volumej
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
	
	if (self->period == 500) self->period = 650;
	else if (self->period == 650) self->period = 931;
	else if (self->period == 931) self->period = 500;

	char buf[100];
	sprintf(buf,"period = %d\n",self->period);
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

void change_key(MelodyPlayer *self, int key) {
	static int i, key;
	static char buf[1000];
	
	SCI_WRITE(&sci0, "Rcv: \'");
    SCI_WRITECHAR(&sci0, c);
	SCI_WRITE(&sci0, "\'\n");
	
		
		i = 0;
		if (key > -6 && key < 6)
			self->key = key;
		else if (key < -5 ) {
			self->key = -5;
		} else if (key > 5) {
			self->key = 5;
		} else {
			SCI_WRITE(&sci0, "wrong key ");
			sprintf(buf,"%d",key);
			SCI_WRITE(&sci0, buf);
			SCI_WRITE(&sci0, ", expected value -6 < key < 6\n");
		}


}

void change_tempo(MelodyPlayer *self, int c) {
	static int i, key;
	static char buf[1000];
	
	SCI_WRITE(&sci0, "Rcv: \'");
    SCI_WRITECHAR(&sci0, c);
	SCI_WRITE(&sci0, "\'\n");
	
	if (c == 'e') {
		buf[i++] = '\0';
		key = atoi(buf);
		i = 0;
		if (key > 59 && key < 241)
			self->tempo = BPM2MS(key);
		else if (key < 60) {
			self->tempo = BPM2MS(60);
		} else if (key > 240) {
			self->tempo = BPM2MS(240);
		} else {
			SCI_WRITE(&sci0, "wrong key ");
			sprintf(buf,"%d",key);
			SCI_WRITE(&sci0, buf);
			SCI_WRITE(&sci0, ", expected value -6 < key < 6\n");
		}
	
	} else if (c != 'e') { 
		buf[i++] = c;
	} 

}

void melplay_load(MelodyPlayer * self, int unused){
	ASYNC(&tonegen, sound_unmutter, 0);
	//set new tone	
	get_brother_john_tune(&tonegen, self);

	
	if(self->idx < 31) self->idx++; else self->idx=0;
	char buf[100];
	sprintf(buf,"%d, %d, %d\n",self->idx, self->period, tonegen.period);
	SCI_WRITE(&sci0, buf);
	
	SEND(MSEC(self->period - 50), MSEC(self->period-50), &tonegen, sound_mutter, 1);
	SEND(MSEC(self->period),  	  MSEC(self->period), self,  	  melplay_load, NULL);
	
	
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

void run_melplay_load(App *self, int c){
	if (self->isMelodyPlayerRunning) {
		SCI_WRITE(&sci0, "Already is running \n");
	} else {
		SCI_WRITE(&sci0, "Starting dummy load \n");
		
		self->isMelodyPlayerRunning = 1;
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
		SYNC(&t)
	} else if (c == 'r') {
		buf[i++] = c;
		buf[i++] = '\0';
		key = atoi(buf);
	} else if (c == 'n') {
		SYNC(&tonegen, change_period,0);
	} else if (c == 'd'){
		ASYNC(&tonegen, change_deadline,100);
		ASYNC(&melplay, change_deadline,1300);
	} else {
		SCI_WRITE(&sci0, "Ignoring key input \n");
	}
	
}

void begin(App *self, int unused){
	CAN_INIT(&can0);
	SCI_INIT(&sci0);
	SCI_WRITE(&sci0, "Initializing tone generator \n");
	//initialize tone generator in parallel, sync will start nothing
	ASYNC(&melplay, melplay_load, NULL);
	ASYNC(&tonegen, tone_generator, NULL);
}


int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, begin, 0);
    return 0;
}

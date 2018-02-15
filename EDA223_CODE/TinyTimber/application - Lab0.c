#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    Object super;
    int count;
    char c;
} App;

App app = { initObject(), 0, 'X' };

void reader(App*, int);
void reader_2a(App*, int);
void reader_2b(App*, int);
void reader_3l(App*, int);
void receiver(App*, int);

//Serial sci0 = initSerial(SCI_PORT0, &app, &reader);
//Serial sci0 = initSerial(SCI_PORT0, &app, &reader_2a);
//Serial sci0 = initSerial(SCI_PORT0, &app, &reader_2b);
Serial sci0 = initSerial(SCI_PORT0, &app, &reader_3l);

Can can0 = initCan(CAN_PORT0, &app, receiver);

void receiver(App *self, int unused) {
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
    SCI_WRITE(&sci0, "Can msg received: ");
    SCI_WRITE(&sci0, msg.buff);
}

void reader(App *self, int c) {
    SCI_WRITE(&sci0, "Rcv: \'");
    SCI_WRITECHAR(&sci0, c);
    SCI_WRITE(&sci0, "\'\n");
}

void reader_2a(App *self, int c) {
	static int i, myNum;
	static char buf[1000];
	
	SCI_WRITE(&sci0, "Rcv: \'");
    SCI_WRITECHAR(&sci0, c);
	SCI_WRITE(&sci0, "\'\n");

	if (c == 'e') {
		buf[i++] = '\0';
		myNum = atoi(buf);
		if (myNum == -106) myNum+=13;
		sprintf(buf,"myNum=%d\n",myNum);
		SCI_WRITE(&sci0, buf);
		i = 0;
		
	} else if (c != 'e') { 
		buf[i++] = c;
	}

}

void reader_2b(App *self, int c) {
	static int i, myNum, mySum;
	static char buf[1000];
	
	SCI_WRITE(&sci0, "Rcv: \'");
    SCI_WRITECHAR(&sci0, c);
	SCI_WRITE(&sci0, "\'\n");

	if (mySum == NULL) mySum = 0;
	
	if (c == 'e') {
		buf[i++] = '\0';
		myNum = atoi(buf);
		mySum += myNum;
		sprintf(buf,"The entered number is (%d)\nThe running sum is (%d)\n",myNum,mySum);
		SCI_WRITE(&sci0, buf);
		i = 0;
	} else if (c == 'F'){
		mySum = 0;
		sprintf(buf,"Sum reseted. The running sum is (%d)\n",mySum);
		SCI_WRITE(&sci0, buf);
	} else if (c != 'e' && c != 'F') { 
		buf[i++] = c;
	} 

}

void reader_3l(App *self, int c) {
	static int i, key;
	static char buf[1000];
	
	SCI_WRITE(&sci0, "Rcv: \'");
    SCI_WRITECHAR(&sci0, c);
	SCI_WRITE(&sci0, "\'\n");
	
	if (c == 'e') {
		buf[i++] = '\0';
		key = atoi(buf);
		i = 0;
		if (key > -6 && key < 6)
			print_brother_john_3l(key);
		else {
			SCI_WRITE(&sci0, "wrong key ");
			sprintf(buf,"%d",key);
			SCI_WRITE(&sci0, buf);
			SCI_WRITE(&sci0, ", expected value -6 < key < 6\n");
		}
	
	} else if (c != 'e') { 
		buf[i++] = c;
	} 

}
/* Task 3:
 * Task 3.a :
 Consider that alternating ’1’s and ’0’s are written to the DAC
so that a tone of frequency 200 Hz is produced. The period of the tone-
generating task is equal to one of the following. Tick the correct one. 
[Hint: 1 sec = 10^3 ms =10^6μs]: (a) 5000 μs (b) 2500 μs (c) 2000 μs
 anwserw: f = 200Hz -> T = 1/2/f = 1/2/200 = 0.0025[s] = 2500[us]
  * Task 3.b : 
Consider that the DAC is written with alternating ’1’s and
’0’s so that a tone of frequency 1 kHz is produced. Find the period of the
tone-generating task.
 f = 1kHz = 1000Hz -> T = 1/2/1000 = 0.0005[s] = 
  * */

#define MIN_INDEX -10
#define MAX_INDEX 14
int tune[32]= {0, 2, 4, 0, 0, 2, 4, 0, 4, 5, 7, 4, 5, 7, 7, 9, 7, 5, 4, 0, 7, 9, 7, 5, 4, 0, 0, -5, 0, 0, -5, 0}; 
int period[25] = {2024, 1911, 1803, 1702, 1607, 1516, 1431, 1351, 1275, 1203, 1136,1072, 1012,  955,  901,
  851,  803,  758,  715,  675,  637,  601,  568,  536,  506};//14-(-10)+1

void print_brother_john_3k() {
	int key = 0;
	for (int i=0; i<32; i++) {
		char buf[5];
		sprintf(buf, "%d ",period[tune[i] + key - MIN_INDEX]);
		SCI_WRITE(&sci0, buf);
	}
	SCI_WRITECHAR(&sci0, '\n');
}

void print_brother_john_3l(int key) {
	for (int i=0; i<32; i++) {
		char buf[5];
		sprintf(buf, "%d ",period[tune[i] + key - MIN_INDEX]);
		SCI_WRITE(&sci0, buf);
	}
	SCI_WRITECHAR(&sci0, '\n');
}


void startApp(App *self, int arg) {
    CANMsg msg;

    CAN_INIT(&can0);
    SCI_INIT(&sci0);
    SCI_WRITE(&sci0, "Hello, hello...\n");

    msg.msgId = 1;
    msg.nodeId = 1;
    msg.length = 6;
    msg.buff[0] = 'H';
    msg.buff[1] = 'e';
    msg.buff[2] = 'l';
    msg.buff[3] = 'l';
    msg.buff[4] = 'o';
    msg.buff[5] = 0;
    CAN_SEND(&can0, &msg);
}

int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}

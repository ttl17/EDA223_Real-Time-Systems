/*
	startup.c
	startup for applications using MD407 monitor
	2016-01-25 JJ First version
	2016-02-xx JJ Added DAC and USART init code
	2016-03-08 JJ Added CAN init code
	2017-02-21 JJ Added support for DIV_0_TRP and UNALIGN_TRP exceptions
*/
  
/* **********************************************************************
	STANDARD STARTUP ROUTINE
*********************************************************************** */

#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_dac.h"
#include "stm32f4xx_can.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_rcc.h"

void startup(void) __attribute__((naked)) __attribute__((section (".start_section")) );

static void Init(void) __attribute__((used));
static void Exit(void) __attribute__((used));

/* Built in DBG-routines, absolute addresses */
typedef void (* function1 )(void);  

#define hwInit  ((function1)    (0x08000201))   /* do hardware initialisation */
#define exitDBG ((function1)    (0x08000205))   /* reentry in debugger */

//#define GDB_DEBUG

#define __USE_UNALIGN_TRP       // Note: Usage Fault exception enabled for unaligned 16-bit/32-bit data transfers

#define __USE_dbgARM_MONITOR    // Note: Let the dbgARM monitor do the USART initialization

#ifndef __USE_dbgARM_MONITOR
static void __usart_init() {
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_ClockInitTypeDef USART_ClockInitStructure;
	USART_InitTypeDef USART_InitStructure;

	GPIO_StructInit( &GPIO_InitStructure );

	GPIO_DeInit( GPIOA );
	USART_DeInit( USART1 );
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_USART1, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE);

	/* Connect USART pins to AF */
	/* PA9 - USART1 TX  */
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);  
	/* PA10 - USART1 RX */
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);  
	/* PA11 - USART CTS  not used*/
	/* PA12 - USART RTS not used */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);	

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

   /* USARTx configured as follow:
         - BaudRate = 115200 baud  
         - Word Length = 8 Bits
         - One Stop Bit
         - No parity
         - Hardware flow control disabled (RTS and CTS signals)
         - Receive and transmit enabled
   */
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_ClockStructInit(&USART_ClockInitStructure);	
	USART_ClockInit(USART1, &USART_ClockInitStructure);

	USART_Init( USART1, &USART_InitStructure);
	USART_Cmd( USART1, ENABLE);	
}
#else
static void __usart_init() {
}
#endif

void DUMP(char *s);

static void __can_init() {
	GPIO_InitTypeDef GPIO_InitStructure;
	CAN_InitTypeDef CAN_InitStructure;
	CAN_FilterInitTypeDef  CAN_FilterInitStructure;

	GPIO_StructInit( &GPIO_InitStructure );

	GPIO_DeInit( GPIOB );
	CAN_DeInit( CAN1 );
	CAN_DeInit( CAN2 );
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_CAN1, ENABLE );
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_CAN2, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB, ENABLE);

	/* Connect CAN1 pins to AF */
	/* PB9 - CAN1 TX */
	/* PB8 - CAN1 RX  */
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_CAN1);  	
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_CAN1);  

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);	

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	/* Connect CAN2 pins to AF */
	/* PB6 - CAN1 TX */
	/* PB5 - CAN1 RX  */
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_CAN2);  	
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_CAN2);  

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);	

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	// code below should be moved to canTinyTimber.c
	CAN_InitStructure.CAN_TTCM = DISABLE; // time-triggered communication mode = DISABLED
	CAN_InitStructure.CAN_ABOM = DISABLE; // automatic bus-off management mode = DISABLED
	CAN_InitStructure.CAN_AWUM = DISABLE; // automatic wake-up mode = DISABLED
	CAN_InitStructure.CAN_NART = DISABLE; // non-automatic retransmission mode = DISABLED
	CAN_InitStructure.CAN_RFLM = DISABLE; // receive FIFO locked mode = DISABLED
	CAN_InitStructure.CAN_TXFP = DISABLE; // transmit FIFO priority = DISABLED
	CAN_InitStructure.CAN_Mode = CAN_Mode_Normal; // normal CAN mode
	//
	// 42 MHz clock on APB1
	// Prescaler = 7 => time quanta tq = 1/6 us
	// Bit time = tq*(SJW + BS1 + BS2)
	// See figure 346 in F407 - Reference Manual.pdf
	// 
	CAN_InitStructure.CAN_SJW = CAN_SJW_1tq; // synchronization jump width = 1
	CAN_InitStructure.CAN_BS1 = CAN_BS1_3tq; 
	CAN_InitStructure.CAN_BS2 = CAN_BS2_4tq; 
	CAN_InitStructure.CAN_Prescaler = 7; // baudrate 750kbps

	if (CAN_Init(CAN1, &CAN_InitStructure) == CAN_InitStatus_Failed)
		DUMP("\n\rCAN #1 Init failed!\n\r");
//	else
//		DUMP("\n\rCAN #1 successful!\n\r");

	if (CAN_Init(CAN2, &CAN_InitStructure) == CAN_InitStatus_Failed)
		DUMP("CAN #2 Init failed!\n\r");
//	else
//		DUMP("CAN #2 successful!\n\r");
	
    /* CAN filter init */
    CAN_FilterInitStructure.CAN_FilterNumber = 0;
    CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
    CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
    CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;
    CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000; // 0 in a position means ignore that bit
    CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;
    CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
    CAN_FilterInit(&CAN_FilterInitStructure);
}

static void __dac_init() {
	GPIO_InitTypeDef GPIO_InitStructure;
	DAC_InitTypeDef  DAC_InitStructure;

//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

	/* DAC channel 2 (DAC_OUT2 = PA.5) configuration */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	DAC_DeInit();

	DAC_StructInit( &DAC_InitStructure );

	DAC_InitStructure.DAC_Trigger = DAC_Trigger_None;
	DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
	DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;

	DAC_Init(DAC_Channel_2, &DAC_InitStructure);
	
	DAC_Cmd(DAC_Channel_2, ENABLE);
	
	DAC_SetChannel2Data(DAC_Align_8b_R, 0);
}

static void __sys_init() {
	__set_CONTROL(__get_CONTROL() & ~0x03); // Use MSP and privileged Thread mode

    SCB->CCR = 0x210;		// STKALIGN + DIV_0_TRP 
#ifdef __USE_UNALIGN_TRP
    SCB->CCR |= 0x008;      // + UNALIGN_TRP
#else
	DUMP("\n\rNOTE: Usage Fault exception disabled for unaligned 16-bit/32-bit data transfers!\n\r");
#endif

    //
    // FPU enabled by dbgARM monitor, in function SystemInit() called by ResetHandler 
	//	SCB->CPACR = 0xF00000; // Enable FPU
	FPU->FPCCR = 0x80000000; // No lazy stacking
}

static void __timer_init() {
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM5, ENABLE);
	RCC_APB1PeriphClockLPModeCmd( RCC_APB1Periph_TIM5, ENABLE);
}

static void __bss_init() {
asm volatile(
	" ldr  r2, =_sbss\n"  
	" b LoopFillZerobss\n"
/* Zero fill the bss segment. */  
"FillZerobss:\n"
	" movs  r3, #0\n"
	" str  r3, [r2], #4\n"
    
"LoopFillZerobss:\n"
	" ldr  r3, = _ebss\n"
	" cmp  r2, r3\n"
	" bcc  FillZerobss\n"
	) ;
}

static void Init( void )
{
#ifdef GDB_DEBUG
	hwInit();
#endif

	__bss_init();

	__usart_init();
	__can_init();
	__dac_init();
	__sys_init();
	__timer_init();
}

static void Exit( void )
{
#ifdef GDB_DEBUG
	exitDBG();
#endif   
}

void startup( void )
{
asm volatile(
	" NOP\n"					/* executed by first 'trace'-command */
	" LDR SP,=0x2001C000\n"		/* set stack */

	" BL Init\n"				/* do hardware init */
	" BL main\n"				/* call main */
	" B  Exit\n"				/* exit to debugger */
	".L1: B .L1\n"				/* never return */
	) ;
}



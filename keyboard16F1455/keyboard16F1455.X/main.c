/*
 * File:   Main.c
 * Author: Szymon Roslowski
 *
 * Created on 13 October 2014, 17:41
 *
 * Adapted by Klaus Fensterseifer - PY2KLA - 2025
 *
 */


//#include <htc.h>   // Obsoleto. Substituir por #include <xc.h> com o PIC16F1455 selecionado corretamente no projeto.
#include <xc.h>
#include "Usb.h"

// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection Bits (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = OFF       // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = OFF      // MCLR Pin Function Select (MCLR/VPP pin function is Digital Input)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = OFF       // Internal/External Switchover Mode (Internal/External Switchover Mode is enabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config CPUDIV = NOCLKDIV // CPU System Clock Selection Bit (CPU system clock divided by 6)
#pragma config USBLSCLK = 48MHz // USB Low SPeed Clock Selection bit (System clock expects 48 MHz, FS/LS USB CLKENs divide-by is set to 8.)
#pragma config PLLMULT = 3x     // PLL Multipler Selection Bit (3x Output Frequency Selected)
#pragma config PLLEN = ENABLED  // PLL Enable Bit (3x or 4x PLL Enabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LPBOR = OFF      // Low-Power Brown Out Reset (Low-Power BOR is disabled)
#pragma config LVP = OFF         // Low-Voltage Programming Enable (Low-voltage programming enabled)

// Local Defines
//#define Button              RC2
//#define Led                 RC3
#define Button1     PORTAbits.RA4
#define Button2     PORTAbits.RA5
#define Led1        LATCbits.LATC2
#define Led2        LATCbits.LATC3


#define PORT_IN   1
#define PORT_OUT  0

#define AB      11
#define CTRL    22

//#define KEYS    AB
#define KEYS    CTRL


#if KEYS == AB
//#define KeyToPressA          0x04 // Letter A - Lookup Hid Keyboard Scan Codes For further Information
//#define KeyToPressB          0x05 // Letter B - Lookup Hid Keyboard Scan Codes For further Information
#define KeyToPressTraco      0x2D // Letter . - Lookup Hid Keyboard Scan Codes For further Information
#define KeyToPressPonto      0x37 // Letter - - Lookup Hid Keyboard Scan Codes For further Information
#endif

/*
Modifiers
Bit 0: Left CTRL (0x01)
Bit 1: Left Shift (0x02)
Bit 2: Left Alt (0x04)
Bit 3: Left GUI (Windows/Command) (0x08)
Bit 4: Right CTRL (0x10)
Bit 5: Right Shift (0x20)
Bit 6: Right Alt (0x40)
Bit 7: Right GUI (0x80)
*/
#if KEYS == CTRL
#define ModifierToPressCtrlLeft     0x01 //just CTRL left
#define KeyToPressCtrlLeft          0x00 // no letter
#define ModifierToPressCtrlRight    0x10 //just CTRL right
#define KeyToPressCtrlRight         0x00 // no letter
#endif

// Local Variables
BYTE Button1Status;  // This is to hold last status of the button so that we only report if it changes
BYTE Button2Status;  // This is to hold last status of the button so that we only report if it changes



// Interrupt
//void interrupt ISRCode(void)
void __interrupt() ISRCode(void)
{
    if (UsbInterrupt) ProcessUSBTransactions();
}

static void InitializeSystem(void)
{
    ANSELA =        0x00;
    ANSELC =        0x00;
    OSCTUNE =       0x00;
    OSCCON =        0xFC;           // 16MHz HFINTOSC with 3x PLL enabled (48MHz operation)
    ACTCON =        0x90;           // Enable active clock tuning with USB
    OPTION_REG =    0xC3;           // Set prescaler to 256

    //TRISC =         0b00000100;     // Set RC3 as output except RC2 for Button
    //LATC =          0b00000000;     // Clear Port C Latches;
    
    TRISAbits.TRISA5 = PORT_IN;  //RA5 entrada
    WPUAbits.WPUA5 = 1;   // Habilita weak pull-up em RA5
    TRISAbits.TRISA4 = PORT_IN;  //RA4 entrada
    WPUAbits.WPUA4 = 1;   // Habilita weak pull-up em RA4
    OPTION_REGbits.nWPUEN = 0;  // Habilita pull-ups individuais configurados 


    TRISCbits.TRISC3 = PORT_OUT; //RC3 saida
    Led1 = 1; //led desligado
    TRISCbits.TRISC2 = PORT_OUT; //RC4 saida
    Led2 = 1; //led desligado

    Button1Status = 0;
    Button2Status = 0;
}

static void EnableInterrupts(void)
{
    UIE = 0x4B;             // Transaction complete, Strat Of Frame, Error, Reset
    INTCONbits.PEIE = 1;    // Peripheral Interrupt enable
    INTCONbits.GIE = 1;     // Global Interrupt Enable
    PIE2bits.USBIE = 1;     // Enable Usb Global Interrupt
}

//Modifier Keys (First Byte in Keyboard Message) - Not used anywere, just plaed here for referene
#define KEY_L_CTRL			0x01
#define KEY_L_SHIFT			0x02
#define KEY_L_ALT			0x04
#define KEY_L_WIN			0x08
#define KEY_R_CTRL			0x10
#define KEY_R_SHIFT			0x20
#define KEY_R_ALT			0x40
#define KEY_R_WIN			0x80

void PrepareTxBuffer(void)
{
    BYTE i;

    // Fill The Buffer with 0
    for(i = 0 ; i < HidReportByteCount; i++)
    {
        HIDTxBuffer[i] = 0x00;
    }    
    
#if KEYS == AB
    i = 2;
    if((Button1 != Button1Status ) && (Button1 == 0))
    {
        HIDTxBuffer[i] = KeyToPressTraco;    // First of possible 6 simultaneous key pressed
        if(i<8) i++;  //in case another key is pressed, put it on next position (should be max 6 positions))
    }
    if((Button2 != Button2Status ) && (Button2 == 0))
    {
        HIDTxBuffer[i] = KeyToPressPonto;    // uses the second position to avoid rearange the buffer in case of 2 key preeesd
        //if(i<8) i++;  //in case another key is pressed, put it on next position (should be max 6 positions))
    }
#endif

#if KEYS == CTRL
    //An iambic keyer = thumb for dits/dots, index finger for dahs/dashes) 
    if((Button1 != Button1Status ) && (Button1 == 0))
    {
        HIDTxBuffer[0] |= ModifierToPressCtrlRight;       // Dahs   set the bit on modifiers
    }
    if((Button2 != Button2Status ) && (Button2 == 0))
    {
        HIDTxBuffer[0] |= ModifierToPressCtrlLeft;        // Dits   set the bit on modifiers
    }
#endif


}

void ProcessIncommingData(void)
{
    // Windows Will send only a single Byte
    // with statuses of leds
    // first bit for num lock, second for caps etc..
    //Led = (HIDRxBuffer[0] & 0x01);
}

static void CheckUsb(void)
{
    if(IsUsbDataAvaialble(HidInterfaceNumber) > 0 )
    {
        ProcessIncommingData();
        ReArmInterface(HidInterfaceNumber);
    }
}

void ProcessIO(void)
{
    // Check USB for incomming Commands
    if (IsUsbReady) CheckUsb();

    // Check Status Of the Button
    if ((Button1 != Button1Status ) ||
        (Button2 != Button2Status ))
    {
        // If Button Status Chenged - Report
        PrepareTxBuffer();
        HIDSend(HidInterfaceNumber);

        // Save New Button Status
        Button1Status = Button1;
        Button2Status = Button2;
        Led1 = Button1;
        Led2 = Button2;
    }
}

void main(void)
{
    InitializeSystem();
    InitializeUSB();
    EnableUSBModule();
    EnableInterrupts();

    while(1) { ProcessIO(); }
}





#if 0

/*
 * File:   keyboard_main.c
 * Author: elikla
 *
 * Created on 27 de Junho de 2025, 07:46
 * 
 * 
 * PIC16F1455
 * Le 2 entradas para dih e dah do paddle CW e 
 * envia via USB as teclas CTRL_rigth e CTRL_left
 * como se fosse um teclado de PC
 * 
 * HFINTOSC = 16MHz
 * 
 */


// PIC16F1455 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection Bits (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = OFF        // Watchdog Timer Enable (WDT enabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = ON       // MCLR Pin Function Select (MCLR/VPP pin function is MCLR)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = ON        // Internal/External Switchover Mode (Internal/External Switchover Mode is enabled)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config CPUDIV = NOCLKDIV// CPU System Clock Selection Bit (NO CPU system divide)
#pragma config USBLSCLK = 48MHz // USB Low Speed Clock Selection bit (System clock expects 48 MHz, FS/LS USB CLKENs divide-by is set to 8.)
#pragma config PLLMULT = 3x     // PLL Multiplier Selection Bit (3x Output Frequency Selected)
#pragma config PLLEN = ENABLED  // PLL Enable Bit (3x or 4x PLL Disabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LPBOR = OFF      // Low-Power Brown Out Reset (Low-Power BOR is disabled)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>


#define _XTAL_FREQ   16000000UL   //HFINTOSC = 16MHz  used on delay functions





#define RA4_IN     PORTAbits.RA4
#define RA5_IN     PORTAbits.RA5

//#define RC2_IN     PORTCbits.RC2
//#define RC3_IN     PORTCbits.RC3
#define RC2_OUT    LATCbits.LATC2
#define RC3_OUT    LATCbits.LATC3


#define PORT_IN   1
#define PORT_OUT  0

void main(void) 
{
    //SCS<1:0>=1x select internal INTOSC clock divider (already done by FOSC<2:0> = 100)
    //OSCCON  IRCF<3:0>=1111 = 16Mhz  (default=0111=500kHz))
    OSCCONbits.IRCF = 0b1111; // Seleciona 16 MHz
    OSCCONbits.SCS  = 0b10;   // Usa o INTOSC como clock primário (SCS = 10)
    
    //TRISA |= (1<<5);  //RA5 entrada
    TRISAbits.TRISA5 = PORT_IN;
    //TRISA |= (1<<4);  //RA4 entrada
    TRISAbits.TRISA4 = PORT_IN;
    
    //TRISC &= ~(1<<3); //RC3 saida
    TRISCbits.TRISC3 = PORT_OUT;
    LATC &= ~(1<<3); //RC3 desligado
    //TRISC &= ~(1<<2); //RC4 saida
    TRISCbits.TRISC2 = PORT_OUT;
    LATC &= ~(1<<2); //RC4 desligado

    
#if 0    
    //LATC |= (1<<2); //RC2 ligado
    RC2_OUT = 1; //RC2 ligado
    __delay_ms(1000);
    //LATC &= ~(1<<2); //RC2 desligado
    RC2_OUT = 0; //RC2 desligado

    //LATC |= (1<<3); //RC3 ligado
    RC3_OUT = 1; //RC3 ligado
#endif
    
    while(1)
    {
            //LATC = (PORTC ^ (1<<3)); //RC3 toggle
            //RC3_OUT = ~RC3_IN;  //RC3 toggle
            //RC3_OUT = (RC3_OUT == 0 ? 1 : 0);  //RC3 toggle
            RC3_OUT ^= 1;  //RC3 toggle
            //LATCbits.LATC3 = 1;
            __delay_ms(500);
            //LATCbits.LATC3 = 0;
            
            //LATC = (PORTC ^ (1<<2)); //RC2 toggle
            //RC2_OUT = ~RC2_IN;  //RC2 toggle
            RC2_OUT ^= 1;  //RC2 toggle
            //LATCbits.LATC2 = 1;
            __delay_ms(500);
            //LATCbits.LATC2 = 0;
    }
    
}


#endif

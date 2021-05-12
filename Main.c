#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <math.h>
#include <usart.h>
#include <pic18f4321.h>

#include "LCD_Screen.h"
#include "State_Machine.h"

#pragma config OSC = INTIO67
#pragma config WDT = OFF
#pragma config LVP = OFF
#pragma config BOREN = OFF

#define Data_Out LATA0              /* assign Port pin for data*/
#define Data_In PORTAbits.RA0       /* read data from Port pin*/
#define Data_Dir TRISAbits.RA0      /* Port direction */
#define _XTAL_FREQ 8000000          /* define _XTAL_FREQ for using internal delay */


#define PIR_DIR TRISAbits.TRISA1         // Sets the I/O direction for the PIR sensor
#define PIR_OUTPUT PORTAbits.RA1         // Reads the output voltage of the PIR at pin RA1 
#define BUZZ_DIR TRISCbits.TRISC3

void DHT11_Start();
void DHT11_CheckResponse();
char DHT11_ReadData();
void Init_ADC(void);
unsigned int Get_Full_ADC(void);
void Select_ADC_Channel(char);
void putch (char c)
{
    while (!TRMT);
    TXREG = c;
}
void init_UART()
{
    OpenUSART (USART_TX_INT_OFF & USART_RX_INT_OFF &
    USART_ASYNCH_MODE & USART_EIGHT_BIT & USART_CONT_RX &
    USART_BRGH_HIGH, 25);
    OSCCON = 0x60;
}

void Activate_Buzzer()                                                          //Function to Activate Buzzer
{
        PR2 = 0b11111001 ; 
    T2CON = 0b00000101 ; 
    CCPR2L = 0b01001010 ; 
    CCP2CON = 0b00111100 ; 
    
}

void Deactivate_Buzzer()                                                        //Function to Deactivate Buzzer
{
    CCP2CON = 0x0;
    PORTCbits.RC3 = 0;
}


void Select_ADC_Channel(char channel)      //selects channel via input shifting 4 bits
{
    ADCON0=channel *4 +1;

}

void Init_ADC(void)
{
ADCON1=0x19;                                 // set pins 1 & 2 as analog signal, VDD-VSS as ref voltage
ADCON2=0xA9;                                 // Set the bit conversion time (TAD) and acquisition time
}

unsigned int Get_Full_ADC(void)
{
 int result;
 ADCON0bits.GO=1;                            // Start Conversion
 while(ADCON0bits.DONE==1);                  // Wait for conversion to be completed (DONE=0)
 result = (ADRESH * 0x100) + ADRESL;         // Combine result of upper byte and lower byte into
 return result;                              // return the most significant 8- bits of the result.
} 
void main() 
{
    
    init_UART();
    Init_ADC();                       // initialize the A2D converter
    TRISA =0xDF;                       // bit5 is output, rest is input
    OSCCON = 0x72;
    ADCON1 = 0x0F;                  
   
    char RH_Decimal,RH_Integral,T_Decimal,T_Integral;
    char Checksum;
    char value[10];    

    LCD_Init();         /* initialize LCD16x2 */

    PIR_DIR = 1;                                                                // Set pin RA2 to recieve input
    BUZZ_DIR = 0;                                                               // Set pin RB3 to be output pin
    
    while(1)
    {
       
    DHT11_Start();                  /* send start pulse to DHT11 module */
    DHT11_CheckResponse();          /* wait for response from DHT11 module */
    
    /* read 40-bit data from DHT11 module */
    RH_Integral = DHT11_ReadData(); /* read Relative Humidity's integral value */
    RH_Decimal = DHT11_ReadData();  /* read Relative Humidity's decimal value */
    T_Integral = DHT11_ReadData();  /* read Temperature's integral value */
    T_Decimal = DHT11_ReadData();   /* read Relative Temperature's decimal value */
    Checksum = DHT11_ReadData();    /* read 8-bit checksum value */
    
   
    Select_ADC_Channel (4);
    unsigned int nstep = Get_Full_ADC();
    float float_nstep = nstep;
    int temp_temp = T_Integral;
    while(PIR_OUTPUT || float_nstep > 0 || temp_temp > 21 || temp_temp == 0)                                                         // While reading output voltage of the PIR sensor
    {   
        
        if(PIR_OUTPUT == 1)                                                   // If output voltage is high
        {             
            LCD_String_xy(1,1,"Motion Detected");
            BUZZ_DIR = 1;                           //  Activate_Buzzer()   ;

        }
        else if (float_nstep>0 )
        {
            LCD_String_xy(0,8,"PACKAGE"); 
            BUZZ_DIR = 1;                           //  Activate_Buzzer()   ;
        }
        else if (temp_temp > 21 | temp_temp == 0)
        {
            LCD_String_xy(0,8,"Outside Temp unsafe"); 
            BUZZ_DIR = 1;                           //  Activate_Buzzer()   ;
            
            sprintf(value,"%d",T_Integral);
            LCD_String_xy(0,0,value);
            sprintf(value,".%d",T_Decimal);
            LCD_String(value);
            LCD_Char(0xdf);
            LCD_Char('C');
            MSdelay(500);
        }
        LCD_Clear();
    }
    LCD_Clear();
    
//    while(PIR_OUTPUT)                                                         // While reading output voltage of the PIR sensor
//    {          
//        if(PIR_OUTPUT == 1)                                                   // If output voltage is high
//        {             
//            LCD_String_xy(1,1,"Motion Detected");                    
//        }
//        LCD_Clear();
//    }
    BUZZ_DIR=0;
    PORTC=0xFF;
    PORTD=0xFF;
    PORTE=0xFF;
    }  
}   
   

char DHT11_ReadData()
{
  char i,data = 0;  
    for(i=0;i<8;i++)
    {
        while(!(Data_In & 1));      /* wait till 0 pulse, this is start of data pulse */
        __delay_us(30);         
        if(Data_In & 1)             /* check whether data is 1 or 0 */    
          data = ((data<<1) | 1); 
        else
          data = (data<<1);  
        while(Data_In & 1);
    }
  return data;
}

void DHT11_Start()
{    
    Data_Dir = 0;       /* set as output port */
    Data_Out = 0;       /* send low pulse of min. 18 ms width */
    __delay_ms(18);
    Data_Out = 1;       /* pull data bus high */
    __delay_us(20);
    Data_Dir = 1;       /* set as input port */    
//    LED = 14;
}

void DHT11_CheckResponse()
{
    while(Data_In & 1);     /* wait till bus is High */     
    while(!(Data_In & 1));  /* wait till bus is Low */
    while(Data_In & 1);     /* wait till bus is High */
}

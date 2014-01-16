/* Name: main.c
 * Author: Juan Carlos Ferrer
 * Copyright: 2012
 * License: BSD
 */

#include <string.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "uart.c" 

#define MASK 0x03 // 0x03 gives us the first 2 bits of the char
#define IN_BUFFER_SIZE 4 // 4 char command buffer  "<READER_ID>:<CMD>\n"
#define flip(x) (((x << 4) & 0xF0) | (x >> 4))
#define bit_at(x,i) !!((x) & (1 << (i)))

typedef struct {
    //Currently only support 4 readers
    //Max RFID value length = 35 bits = 5 bytes
    volatile unsigned char readers[4][5];
    volatile unsigned char readersBitCount[4]; //Count how many bits we've received so far for each reader
}CONTROLLER;
volatile CONTROLLER controller; //Global instance of our RFID controller

typedef struct{
    volatile char buffer[IN_BUFFER_SIZE];
    volatile unsigned char bytesReceived;
    volatile unsigned char cmdAvailable;
}UART0;
volatile UART0 uart0; //Global instance of our uart0 object

enum DOORS {
    //Mapps door names to ascii command values to open them
    DOOR0=0x30, DOOR1=0x31, DOOR2=0x32, DOOR3=0x33,
    //Maps door names to pin assignments.
    DOOR0_PIN=PC7, DOOR1_PIN=PC6, DOOR2_PIN=PC5, DOOR3_PIN=PC4,
};

enum CMD_SECTIONS{
    READERID=0, SEPARATOR, COMMAND,
};

enum COMMAND_FORMAT{
    //     "0"         "1"
    DENIED=0x30, ALLOW=0x31
};

//Handle receiving data on UART0
ISR(USART0_RX_vect){
    uart0.buffer[uart0.bytesReceived++] = UDR0; // Read the buffer
    if(uart0.bytesReceived == 4){
        //We've received a whole command
        uart0.bytesReceived = 0; //Reset
        uart0.cmdAvailable = 1; //Enable the flag, handle the command in main loop
    }

}

//Handle receiving RFID data on PORTK
ISR(PCINT2_vect){
    /* Weigand protocol
     * D1(1) D0(1) - 0x03 == No change
     * D1(0) D0(1) - 0x01 == logical 1
     * D1(1) D0(0) - 0x02 == logical 0
     */
    unsigned char data = PINK; //Read the state of the pins when entering ISR
    //Now figure out which ones changed, and update values if needed
    if(data == 0xFF) return; //Pins changed back to non transmitting state, return
    //If we're here, then we received data
    //Go by 2s, see if it's a 1 or 0
    for(unsigned char i=0; i<8; i+=2){
        unsigned char temp  = (data >> i) & MASK; //Pull out the 2 bits we want
        if(temp == 0x01){
            //Received a '1', set the corresponding bit to one
            if(controller.readersBitCount[i/2] < 8)
                controller.readers[i/2][0] |= (1 << controller.readersBitCount[i/2]);
            if(controller.readersBitCount[i/2] >= 8 && controller.readersBitCount[i/2] < 16)
                controller.readers[i/2][1] |= (1 << controller.readersBitCount[i/2] - 8);
            if(controller.readersBitCount[i/2] >=16 && controller.readersBitCount[i/2] < 24)
                controller.readers[i/2][2] |= (1 << controller.readersBitCount[i/2] - 16);
            if(controller.readersBitCount[i/2] >=24 && controller.readersBitCount[i/2] < 32)
                controller.readers[i/2][3] |= (1 << controller.readersBitCount[i/2] - 24);
            if(controller.readersBitCount[i/2] >=32 && controller.readersBitCount[i/2] < 40)
                controller.readers[i/2][4] |= (1 << controller.readersBitCount[i/2] - 32);
            controller.readersBitCount[i/2]++;
        }
        else if(temp == 0x02){
            //Received a '0', just increment the bit count
            controller.readersBitCount[i/2]++;
        }
    }
}

void init_portk(void){
    //PORTK arduino pins ANALOG8-ANALOG15, ALL inputs
    //These are the PCINT16-PCINT23 pins
    DDRK = 0x00; 
    PORTK = 0xFF; //Enable internal pullups

    PCICR |= (1 << PCIE2); // Turn on pin change interrupt PCIE2 for pins PCINT16-PCINT23
    PCMSK2 = 0xFF; //Now enable ALL PCINT16-PCINT23 pins to trigger PCIE2 interrupt
    
    sei(); //Enable global interrupt
}

void init_portc(void){
    //PORTC arduino pins DIGITAL30-DIGITAL37, ALL outputs
    //These pins "open" the doors
    DDRC = 0xFF;
    PORTC = 0x00; //All low for now

    //DIGITAL30 == READER1
    //DIGITAL31 == READER2
    //...see DOORS enum
}

int main(void)
{
    init_portk();
    init_portc();
    //Init UART0 and set stdout to it
    init_uart0();
    stdout = &uart0_out;
    printf("0:010101010110101001101000\n");

    //Clear out all the controller data
    memset(controller.readers, 0, sizeof(controller.readers));
    memset(controller.readersBitCount, 0, sizeof(controller.readersBitCount));

    for(;;){
        if(uart0.cmdAvailable){
            unsigned char door = uart0.buffer[READERID]; //Number of the RFID reader
            unsigned char cmd = uart0.buffer[COMMAND]; //Command: 0=DENIED 1=ALLOW, etc..
            uart0.cmdAvailable = 0; //Reset the flag
            //Do something with the command here
            if(cmd == ALLOW){
                //Set a pin high, open the door, whatevez
                //printf("Door %x YOU MAY PASS\n", door);
                switch(door){
                    case DOOR0:
                        PORTC |= (1 << DOOR0_PIN);
                        break;
                    case DOOR1:
                        PORTC |= (1 << DOOR1_PIN);
                        break;
                    case DOOR2:
                        PORTC |= (1 << DOOR2_PIN);
                        break;
                    case DOOR3:
                        PORTC |= (1 << DOOR3_PIN);
                        break;
                }
                //_delay_ms(2000); //Don't delay in the real world
                //PORTC = 0x00;
            }
            else if(cmd == DENIED){
                //Log and report to authorities
                //printf("Door %x DENIED\n", door);
                switch(door){
                    case DOOR0:
                        PORTC &= ~(1 << DOOR0_PIN);
                        break;
                    case DOOR1:
                        PORTC &= ~(1 << DOOR1_PIN);
                        break;
                    case DOOR2:
                        PORTC &= ~(1 << DOOR2_PIN);
                        break;
                    case DOOR3:
                        PORTC &= ~(1 << DOOR3_PIN);
                        break;
                }
            }
        }
        //Loop through all the readers
        for(char i=0; i<sizeof(controller.readers); i++){
            //If we've read the correct number of bits, print "<READER_ID>:<CARD_ID>"
            if(controller.readersBitCount[i] == 26 || controller.readersBitCount[i] == 35){
                printf("%d:", i);
                for(char j=0; j<sizeof(controller.readers[i]); j++){
                    for(char k=0; k<8; k++){
                        //Print the individual bits read as ascii '1' or '0'
                        printf("%d", bit_at(controller.readers[i][j], k));
                    }
                    //Get ready for the next card swipe
                }
                controller.readersBitCount[i] = 0x00;
                memset(controller.readers[i], 0, sizeof(controller.readers[i]));
            }
        }
    }
    return 0;   /* never reached */
}

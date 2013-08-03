#include <stdio.h>
#include <stdint.h>


typedef struct {
    //Currently only support 4 readers
    //Max RFID value length = 35 bits = 5 bytes
    uint8_t readers[4][5];
    uint8_t readersBitCount[4];
}CONTROLLER;
CONTROLLER controller; //Global instance of our RFID controller

int main(void){
    uint8_t temp = 0;
    uint8_t mask = 0x03;
    for(uint8_t data=0; data<0x2F; data++){
        for(uint8_t i=0; i<8; i+=2){
            temp  = (data >> i) & mask;
            //temp |= ((data)&(1 << i)) | ((data)&(1 << (i+1)));
            if(temp == 0x01){
                //Set bit to one
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
                printf("Port data: %x  Got 1, on reader %d, count:%d  stuff:%x %x %x %x %x\n", data,i/2, controller.readersBitCount[i/2], controller.readers[i/2][0], controller.readers[i/2][1], controller.readers[i/2][2], controller.readers[i/2][3], controller.readers[i/2][4]);
            }
            if(temp == 0x02){
                controller.readersBitCount[i/2]++;
                printf("Port data: %x  Got 0, on reader %d, count:%d stuff:%x %x %x %x %x\n", data,i/2, controller.readersBitCount[i/2], controller.readers[i/2][0], controller.readers[i/2][1], controller.readers[i/2][2], controller.readers[i/2][3], controller.readers[i/2][4]);
            }
        }
        printf("\n");
    }
}

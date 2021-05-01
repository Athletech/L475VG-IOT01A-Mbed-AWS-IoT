#include "mbed.h"
#include "mbed-trace/mbed_trace.h"
#include "mbedtls/debug.h"

#include "stm32l475e_iot01_tsensor.h"
#include "stm32l475e_iot01_hsensor.h"
#include "stm32l475e_iot01_psensor.h"
#include "stm32l475e_iot01_accelero.h"
#include "stm32l475e_iot01_gyro.h"
#include "stm32l475e_iot01_magneto.h"

#include "core/cloud.h"

#define AWS_IOT_UPDATE_FREQUENCY    1

DigitalOut led(LED1);

Ticker time_int;

RawSerial ser(D1, D0);

struct data_point {
    float gyro[3];
    int16_t acce[3];
    int16_t mag[3];
};

#define MAX_Q_SIZE  64

struct data_point *point_arr;

volatile int q_in = 0;
volatile int q_out = 0;

/*
    Blinks or not
    val         1 = blink, 0 = don't
*/
void blink(int val){
    if(val == -1){
        for(int x=0; x<4; x++){
            led = 1;
            wait(0.8);
            led = 0;
            wait(0.8);
        } 
    }
    
    if(val == 0){
        led = 0;
        wait(0.5); 
    }
    
    if(val == 1){
        led = 1;
        wait(0.5);      
    }
}

/* connect services */
int connect(){
    int result = 0;

    int cloud_status = cloud_connect();
    if(cloud_status == 1){
        result = 1;    
    }
    
    return result;
}

void collect_samples() {
    struct data_point *point = &point_arr[q_in];
    BSP_ACCELERO_AccGetXYZ(point->acce); 
    BSP_GYRO_GetXYZ(point->gyro);
    BSP_MAGNETO_GetXYZ(point->mag);
    
    q_in++;
    if (q_in == MAX_Q_SIZE) {
        q_in = 0;
    }
}

/* just the funky start */
int main(){
    mbed_trace_init();
    
    printf("IoT device starting\r\n");
    
    /* define sensor vars */
    int count = 0;
    
    int status = 0;
    struct data_point *point = NULL;
    
    /* initalise sensors */
    //BSP_TSENSOR_Init();
    //BSP_HSENSOR_Init();
    //BSP_PSENSOR_Init();
    BSP_ACCELERO_Init();
    BSP_GYRO_Init();
    BSP_MAGNETO_Init();
    
    ser.baud(9600);
    ser.format(8, SerialBase::None, 1);
    
    while(1) {
        printf("Sending\n");
        ser.puts("Hello from the other side\n");
        wait(2);
    }
    
    while(1) {
        ;
    }
    /* init cloud (connect wifi) */
    cloud_init();
    
    /* connect to network */
    int connected = connect();
    
    point_arr = (struct data_point *)malloc(MAX_Q_SIZE * sizeof(struct data_point));
    if (point_arr == NULL) {
        printf("Malloc failed\n");
    }
    
    memset(point_arr, 0, MAX_Q_SIZE * sizeof(struct data_point));
    
    time_int.attach(&collect_samples, 0.04);
    while(1) {
        if(connected == 1){
            /* all good here */
            //blink(1);
            
            /* get current sensor data */
            //temperature = BSP_TSENSOR_ReadTemp();
            //humidity = BSP_HSENSOR_ReadHumidity();
            //pressure = BSP_PSENSOR_ReadPressure();
            
            /* send message to cloud */
            
            //wait(0.02);
            while (q_in == q_out) {
                ;
            }
            
            point = &point_arr[q_out];
            count++;
            status = cloud_send(point->acce, point->gyro, point->mag, count);
            if (status < 0) {
                wait(1);
                printf("Inside reconnect \n");
                //HAL_NVIC_SystemReset();
                cloud_init();
                status = connect();
                if (status == 1) {
                    continue;
                }
            }
            q_out++;
            if (q_out == MAX_Q_SIZE) {
                q_out = 0;
            }
        }else{
            /* give it time */
            wait(2);
            
            /* network is off */    
            blink(-1);
        }
    }
}
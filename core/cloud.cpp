#define MQTTCLIENT_QOS1 0
#define MQTTCLIENT_QOS2 0

#include "mbed.h"
#include "ISM43362Interface.h"
#include "NTPClient.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTT_server_setting.h"
#include "MQTTClient.h"
#include "mbed-trace/mbed_trace.h"
#include "mbed_events.h"
#include "mbedtls/error.h"

/*
    Private classes for MQTT handling
*/

ISM43362Interface wifi(false);

MQTTNetwork* mqttNetwork = NULL;
MQTT::Client<MQTTNetwork, Countdown>* mqttClient = NULL;

#define BUFFER_SIZE 1024

int buf_size = BUFFER_SIZE;

char buf[BUFFER_SIZE] = {0};

static unsigned int mqtt_message_id = 0;

/*
    Sync clock with NTP server
*/
void cloud_sync_clock(NetworkInterface* network){
    NTPClient ntp(network);
    
    /* use PTB server in Braunschweig */
    ntp.set_server("ptbtime1.ptb.de", 123);
    
    time_t now = ntp.get_timestamp();
    set_time(now);    
}

/*
    initialise cloud enviroment
*/
void cloud_init(){
    /* connect to the wifi network */
    int w_ret = wifi.connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);
    if(w_ret == 0) {
        printf("WIFI is now connected\r\n");
    }else{
        printf("Failed to connect WIFI\r\n");
    }
    
    /* sync the clock */
    cloud_sync_clock(&wifi);    
}

/*
    Sends an MQTT message to the cloud
*/
int cloud_send(int16_t *acce, float *gyro, int16_t *mag, int count){
    MQTT::Message message;
    message.retained = false;
    message.dup = false;
    
    message.payload = (void*)buf;

    message.qos = MQTT::QOS0;
    message.id = mqtt_message_id++;
    int ret = snprintf(buf, buf_size, "{\"acc_x\":%d,\"acc_y\":%d,\"acc_z\":%d,\"gyr_x\":%.2f,\"gyr_y\":%.2f,\"gyr_z\":%.2f,\"mag_x\":%d,\"mag_y\":%d,\"mag_z\":%d,\"Count\": %d,\"DeviceID\": 19951234}",
                                      acce[0], acce[1], acce[2],
                                      gyro[0], gyro[1], gyro[2],
                                      mag[0], mag[1], mag[2],count);
    if(ret < 0) {
        printf("ERROR: snprintf() returns %d.", ret);
    }
    message.payloadlen = ret;
    
    int rc = mqttClient->publish("stm32/sensor", message);
    if(rc != MQTT::SUCCESS) {
        printf("ERROR: rc from MQTT publish is %d\r\n", rc);
        //wait(10);
        mqttNetwork->disconnect();
        delete mqttNetwork;
        return -1;
    }
    //printf("Message published: %s\r\n",buf);
    //delete[] buf; 
    return 0;
}

/*
    Connects to cloud using TLS socket
*/
int cloud_connect(){
    int result = 0;
      
    /* create mqtt network instance */
    printf("Initialising MQTT network...");
    mqttNetwork = new MQTTNetwork(&wifi);
    int rc = mqttNetwork->connect(MQTT_SERVER_HOST_NAME, MQTT_SERVER_PORT, SSL_CA_PEM,
                SSL_CLIENT_CERT_PEM, SSL_CLIENT_PRIVATE_KEY_PEM);
    if (rc != MQTT::SUCCESS){
        printf("MQTT network not successful\r\n");
        return 0;
    }      
    
    /* create mqtt data packet */
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = (char *)MQTT_CLIENT_ID;
    data.username.cstring = (char *)MQTT_USERNAME;
    data.password.cstring = (char *)MQTT_PASSWORD;

    /* execute client and send data */
    mqttClient = new MQTT::Client<MQTTNetwork, Countdown>(*mqttNetwork);
    rc = mqttClient->connect(data);
    if (rc != MQTT::SUCCESS) {
        printf("MQTT client failed\r\n");
        return 0;
    }else{
        printf("MQTT connect successful\r\n");
        result = 1;
    }
    
    return result;
}

/*
    Connects to cloud using TLS socket
*/
int cloud_reconnect(){
    int result = 0;
      
    /* create mqtt network instance */
    printf("Initialising MQTT network...");
    mqttNetwork = new MQTTNetwork(&wifi);
    int rc = mqttNetwork->connect(MQTT_SERVER_HOST_NAME, MQTT_SERVER_PORT, SSL_CA_PEM,
                SSL_CLIENT_CERT_PEM, SSL_CLIENT_PRIVATE_KEY_PEM);
    if (rc != MQTT::SUCCESS){
        printf("MQTT network not successful\r\n");
        return 0;
    }      
    
    /* create mqtt data packet */
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = (char *)MQTT_CLIENT_ID;
    data.username.cstring = (char *)MQTT_USERNAME;
    data.password.cstring = (char *)MQTT_PASSWORD;

    /* execute client and send data */
    mqttClient = new MQTT::Client<MQTTNetwork, Countdown>(*mqttNetwork);
    rc = mqttClient->connect(data);
    if (rc != MQTT::SUCCESS) {
        printf("MQTT client failed\r\n");
        return 0;
    }else{
        printf("MQTT connect successful\r\n");
        result = 1;
    }
    
    return result;
}
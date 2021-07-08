/*-
 * BSD 2-Clause License
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file   ESP32_scpi.ino
 * 
 *
 * @brief  SCPI parser test
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libscpi/inc/scpi.h"
#include "libscpi/inc/scpi-def.h"

#include <WiFi.h>
#include "Parser.h"

#define RXD2 16
#define TXD2 17

//Digital input pins
const int DIn[3] = {13, 12, 14};

//Digital output pins
const int DOut[3] = {5, 2, 4};

const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASS";

WiFiServer server; //Declare the server object
WiFiClient client;

HardwareSerial& serialSDS(Serial1);

char TXbuffer;  //[256];
int result;
scpi_t *context = &scpi_context;
bool dataReady = false;

#define SCPI_INPUT(cmd)    result = SCPI_Input(context, cmd, bfsize)

void SerialPrintf(const char *format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    client.print(buffer);
    Serial.println(buffer);
}

void setup() {
  // put your setup code here, to run once:

    for (int i = 0; i < 3; i++) {
      pinMode(DIn[i], INPUT);
    }
    for (int i = 0; i < 3; i++) {
      pinMode(DOut[i], OUTPUT);
    }
    serialSDS.begin(19200, SERIAL_7O1, RXD2, TXD2, false);
    Serial.begin(115200);
    
    while (!Serial);

    serialSDS.begin(19200, SERIAL_7O1, RXD2, TXD2, false);
    
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false); //Turn off wifi sleep in STA mode to improve response speed
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected");
    Serial.print("IP Address:");
    Serial.println(WiFi.localIP());

    server.begin(5025); //Server start listening port number 22333

    SCPI_Init(&scpi_context,
            scpi_commands,
            &scpi_interface,
            scpi_units_def,
            SCPI_IDN1, SCPI_IDN2, SCPI_IDN3, SCPI_IDN4,
            scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,
            scpi_error_queue_data, SCPI_ERROR_QUEUE_SIZE);
    Serial.flush();
    
}

void loop() {
    static char buf[64] = {0};
    static uint8_t pos = 0;
  // put your main code here, to run repeatedly:
    client = server.available(); //Try to create a customer object
    
    if (client) //If the current customer is available
    {
        Serial.println("[Client connected]");
        Serial.flush();
        String readBuff;
        while (client.connected()) //If the client is connected
        {
            if (client.available()) //If there is readable data
            {
                const char c = client.read(); //Read a byte
                SCPI_Input(context, &c, 1);
            }
            if (serialSDS.available()) {  
              char b = serialSDS.read();
              switch(b) {
                case 13: break;
                case 0x0A: 
                  buf[pos] = 0;
                  Parser::parse(buf);
                  digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));
                  pos = 0;
                  buf[pos] = 0;
                  break;
                default:
                  buf[pos++] = b;
                  break;
              }
            }
        }
        client.stop(); //End the current connection:
        Serial.println("[Client disconnected]");
    }
  
}

size_t SCPI_Write(scpi_t * context, const char * data, size_t len){
    (void) context;
    return client.print(data); //Serial.write((unsigned char*)data, len);
}

scpi_result_t SCPI_Flush(scpi_t * context) {
    (void) context;
    return SCPI_RES_OK;
}

int SCPI_Error(scpi_t * context, int_fast16_t err) {
    (void) context;

    SerialPrintf("**ERROR: %d, \"%s\"\r\n", (int16_t) err, SCPI_ErrorTranslate(err));
    return 0;
}

scpi_result_t SCPI_Control(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val) {
    (void) context;

    if (SCPI_CTRL_SRQ == ctrl) {
        SerialPrintf("**SRQ: 0x%X (%d)\r\n", val, val);
    } else {
        SerialPrintf("**CTRL %02x: 0x%X (%d)\r\n", ctrl, val, val);
    }
    return SCPI_RES_OK;
}

scpi_result_t SCPI_Reset(scpi_t * context) {
    (void) context;

    SerialPrintf("**Reset\r\n");
    return SCPI_RES_OK;
}

scpi_result_t SCPI_SystemCommTcpipControlQ(scpi_t * context) {
    (void) context;

    return SCPI_RES_ERR;
}

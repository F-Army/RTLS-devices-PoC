/* 
 * StandardRTLSAnchorMain_TWR.ino
 * 
 * This is an example master anchor in a RTLS using two way ranging ISO/IEC 24730-62_2013 messages
 */

#include <SPI.h>
#include <SerialESP8266wifi.h>
#include <SoftwareSerial.h>
#include <DW1000Ng.hpp>
#include <DW1000NgUtils.hpp>
#include <DW1000NgRanging.hpp>
#include <DW1000NgRTLS.hpp>
#include "config.h"

// esp module SoftwareSerial pins
#define RX_PIN 6
#define TX_PIN 5
// esp module CH_PD pin for hw reset
#define CHPD_PIN 4

// connection pins
const uint8_t PIN_RST = 9;
const uint8_t PIN_SS = SS; // spi select pin

byte target_eui[8];
byte tag_shortAddress[] = {0x05, 0x00};

uint16_t tagAddress = 5;

uint16_t this_anchor = 1;
uint16_t next_anchor = 2;

SoftwareSerial SwSerial(RX_PIN, TX_PIN);

SerialESP8266wifi wifi(SwSerial, SwSerial, CHPD_PIN);

device_configuration_t DEFAULT_CONFIG = {
    false,
    true,
    true,
    true,
    false,
    SFDMode::STANDARD_SFD,
    Channel::CHANNEL_5,
    DataRate::RATE_850KBPS,
    PulseFrequency::FREQ_16MHZ,
    PreambleLength::LEN_256,
    PreambleCode::CODE_3
};

frame_filtering_configuration_t ANCHOR_FRAME_FILTER_CONFIG = {
    false,
    false,
    true,
    false,
    false,
    false,
    false,
    true /* This allows blink frames */
};

void setup() {
    pinMode(4, OUTPUT);
    // DEBUG monitoring
    Serial.begin(115200);
    SwSerial.begin(115200);
    Serial.println();
    while(!SwSerial)
    ;
    Serial.println("Starting wifi...");
    wifi.setTransportToTCP();
    wifi.endSendWithNewline(false);
    
    boolean wifi_started = wifi.begin();
    if (wifi_started) {
      if(wifi.connectToAP(WIFI_SSID, WIFI_PASSWORD)) Serial.print("wifi OK");
    } else {
      Serial.println("Error during starting WIFI");
    }
    wifi.connectToServer("127.0.0.1", "3000");
    if(wifi.isConnectedToServer()) wifi.send(SERVER, "Anchor MAIN");
        
    Serial.println(F("### DW1000Ng-arduino-ranging-anchorMain ###"));
    // initialize the driver
    DW1000Ng::initializeNoInterrupt(PIN_SS, PIN_RST);
    
    Serial.println(F("DW1000Ng initialized ..."));
    // general configuration
    DW1000Ng::applyConfiguration(DEFAULT_CONFIG);
    DW1000Ng::enableFrameFiltering(ANCHOR_FRAME_FILTER_CONFIG);
    
    DW1000Ng::setEUI("AA:BB:CC:DD:EE:FF:00:01");

    DW1000Ng::setPreambleDetectionTimeout(64);
    DW1000Ng::setSfdDetectionTimeout(273);
    DW1000Ng::setReceiveFrameWaitTimeoutPeriod(5000);

    DW1000Ng::setNetworkId(RTLS_APP_ID);
    DW1000Ng::setDeviceAddress(this_anchor);
	
    DW1000Ng::setAntennaDelay(16436);
    
    Serial.println(F("Committed configuration ..."));
    // DEBUG chip info and registers pretty printed
    char msg[128];
    DW1000Ng::getPrintableDeviceIdentifier(msg);
    Serial.print("Device ID: "); Serial.println(msg);
    DW1000Ng::getPrintableExtendedUniqueIdentifier(msg);
    Serial.print("Unique ID: "); Serial.println(msg);
    DW1000Ng::getPrintableNetworkIdAndShortAddress(msg);
    Serial.print("Network ID & Device Address: "); Serial.println(msg);
    DW1000Ng::getPrintableDeviceMode(msg);
    Serial.print("Device mode: "); Serial.println(msg);    
}

void loop() {
    if(DW1000NgRTLS::receiveFrame()){
        size_t recv_len = DW1000Ng::getReceivedDataLength();
        byte recv_data[recv_len];
        DW1000Ng::getReceivedData(recv_data, recv_len);


        if(recv_data[0] == BLINK) {
            DW1000NgRTLS::transmitRangingInitiation(&recv_data[2], tag_shortAddress);
            DW1000NgRTLS::waitForTransmission();

            RangeAcceptResult result = DW1000NgRTLS::anchorRangeAccept(NextActivity::RANGING_CONFIRM, next_anchor);
            if(!result.success) return;

            // Call server ESP
            /*
            HTTPClient http;
            String request = "";
                request += "anchor=";
                request += this_anchor;
                request += "&tag=";
                request += tagAddress;
                request += "&range=";
                request += result.range;
            http.begin(SERVER_URL);
            http.addHeader("Content-Type", "application/x-www-form-urlencoded");
            int httpCode = http.POST(request);
            String payload = http.getString();
            Serial.print("Range sent: ");
            Serial.print(httpCode);
            Serial.print(" ");
            Serial.print(payload);
            Serial.println();
            http.end();
            */
        }
    }

    
}

/**
    - Connect Tempal Temperature Sensor over BLE.
    - Read battery voltage and temperature.
    - Send to STM32l072 via UART2.
 */

#include "BLEDevice.h"

#define sda_pin 23
#define sck_pin 22
#define cs_pin  21

#define grn1 32
#define red1 33
#define grn2 25
#define red2 26
#define grn3 27
#define red3 14
#define grn4 12
#define red4 13

static String tempalMacAddress = "64:69:4e:45:3e:66";
static BLEUUID serviceUUID("c050");
static BLEUUID    charUUID("c05a");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

static int cnt = 10;

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    uint16_t vBatt;
    uint16_t temperature;
    uint16_t dacValue = 0;
    float tempFloat;
  
    temperature = pData[2];
    temperature = (temperature<<8)+pData[3];
    
    vBatt = pData[0];
    vBatt = (vBatt<<8)+pData[1];
    Serial.println(vBatt);
    if(vBatt>4000){
      digitalWrite(grn1, LOW);
      digitalWrite(red1, HIGH);
      digitalWrite(grn2, LOW);
      digitalWrite(red2, HIGH);
      digitalWrite(grn3, LOW);
      digitalWrite(red3, HIGH);
      digitalWrite(grn4, LOW);
      digitalWrite(red4, HIGH);
    }
    else if(vBatt>3900){
      digitalWrite(grn1, HIGH);
      digitalWrite(red1, HIGH);
      digitalWrite(grn2, LOW);
      digitalWrite(red2, HIGH);
      digitalWrite(grn3, LOW);
      digitalWrite(red3, HIGH);
      digitalWrite(grn4, LOW);
      digitalWrite(red4, HIGH);
    }
    else if(vBatt>3800){
      digitalWrite(grn1, HIGH);
      digitalWrite(red1, HIGH);
      digitalWrite(grn2, HIGH);
      digitalWrite(red2, HIGH);
      digitalWrite(grn3, LOW);
      digitalWrite(red3, LOW);
      digitalWrite(grn4, LOW);
      digitalWrite(red4, LOW);
    }
    else{
      digitalWrite(grn1, HIGH);
      digitalWrite(red1, HIGH);
      digitalWrite(grn2, HIGH);
      digitalWrite(red2, HIGH);
      digitalWrite(grn3, HIGH);
      digitalWrite(red3, HIGH);
      digitalWrite(grn4, HIGH);
      digitalWrite(red4, LOW);
    }

    tempFloat = float(temperature)/100;

    if(1660<=temperature && temperature<=2069)
    {
      //Serial.println("16.6 - 20.6");
      dacValue = 46*tempFloat+29953.4;
      for(int i=0; i<cnt; i++){
        ext_dac(sda_pin, sck_pin, cs_pin, dacValue, 500);
      }
      Serial.println(dacValue);
    }
    if(2070<=temperature && temperature<=2479)
    {
      //Serial.println("20.7 - 24.7");
      dacValue = 45.25*tempFloat+29968.325;
      for(int i=0; i<cnt; i++){
        ext_dac(sda_pin, sck_pin, cs_pin, dacValue, 500);
      }
      Serial.println(dacValue);
    }
    if(2480<=temperature && temperature<=2889)
    {
      //Serial.println("24.8 - 28.8");
      dacValue = 42.25*tempFloat+30043.2;
      for(int i=0; i<cnt; i++){
        ext_dac(sda_pin, sck_pin, cs_pin, dacValue, 500);
      }
      Serial.println(dacValue);
    }
    if(2890<=temperature && temperature<=3299)
    {
      //Serial.println("28.9 - 32.9");
      dacValue = 39*tempFloat+30136.9;
      for(int i=0; i<cnt; i++){
        ext_dac(sda_pin, sck_pin, cs_pin, dacValue, 500);
      }
      Serial.println(dacValue);
    }
    if(3300<=temperature && temperature<=3669)
    {
      //Serial.println("33,0 - 36,6");
      dacValue = 36.38*tempFloat+30223.46;
      for(int i=0; i<cnt; i++){
        ext_dac(sda_pin, sck_pin, cs_pin, dacValue, 500);
      }
      Serial.println(dacValue);
    }
    if(3670<=temperature)
    {
      //Serial.println("t>36,7");
      dacValue = 33.63*tempFloat+30223.78;
      for(int i=0; i<cnt; i++){
      ext_dac(sda_pin, sck_pin, cs_pin, dacValue, 500);
      }
      Serial.println(dacValue);
    }

    //Serial.println(temperature);
    Serial.println(tempFloat);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }
  void onDisconnect(BLEClient* pclient){
    connected = false;
    Serial.println("onDisconnect");
    ext_dac(sda_pin, sck_pin, cs_pin, 0, 500);
    digitalWrite(grn1, HIGH);
    digitalWrite(red1, LOW);
    digitalWrite(grn2, HIGH);
    digitalWrite(red2, LOW);
    digitalWrite(grn3, HIGH);
    digitalWrite(red3, LOW);
    digitalWrite(grn4, HIGH);
    digitalWrite(red4, LOW);
    ESP.restart();
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    if(String(myDevice->getAddress().toString().c_str()) == tempalMacAddress)
    {
      BLEClient*  pClient  = BLEDevice::createClient();
      Serial.println(" - Created client");
  
      pClient->setClientCallbacks(new MyClientCallback());
      // Connect to the remove BLE Server.
      pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
      Serial.println(" - Connected to server");
  
      // Obtain a reference to the service we are after in the remote BLE server.
      BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
      if (pRemoteService == nullptr) {
        Serial.print("Failed to find our service UUID: ");
        Serial.println(serviceUUID.toString().c_str());
        pClient->disconnect();
        return false;
      }
      Serial.println(" - Found our service");
  
  
      // Obtain a reference to the characteristic in the service of the remote BLE server.
      pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
      if (pRemoteCharacteristic == nullptr) {
        Serial.print("Failed to find our characteristic UUID: ");
        Serial.println(charUUID.toString().c_str());
        pClient->disconnect();
        return false;
      }
      Serial.println(" - Found our characteristic");
  
      // Read the value of the characteristic.
      /*if(pRemoteCharacteristic->canRead()) {
        std::string value = pRemoteCharacteristic->readValue();
        Serial.print("The characteristic value was: ");
        Serial.println(value.c_str());
      }*/
  
      if(pRemoteCharacteristic->canNotify())
        pRemoteCharacteristic->registerForNotify(notifyCallback);
  
      connected = true;
      return true;
    }
    else{
      return false;
    }
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
    else{
      doConnect = true;
      doScan = true;
    }
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void ext_dac(uint8_t sdaPin, uint8_t sckPin, uint8_t csPin, uint16_t cmdData, int dacPeriod)
{
  digitalWrite(csPin, LOW);
  bool data_ser=0;
  for(int i=0; i<16; i++)
  {
    data_ser = (cmdData<<i)&(0x8000);
    digitalWrite(sdaPin, data_ser);     // MSB sda pinine yazıldı.
    delayMicroseconds(dacPeriod);       // period kadar beklendi. sck level 0
    digitalWrite(sckPin, HIGH);         // sck level 1
    delayMicroseconds(dacPeriod);       // period kadar beklendi.
    digitalWrite(sckPin, LOW);          // sck level 0
  }
  digitalWrite(sdaPin, LOW);
  digitalWrite(csPin, HIGH);
}


void setup() {
  Serial.begin(115200);
  pinMode(sda_pin, OUTPUT);
  pinMode(sck_pin, OUTPUT);
  pinMode(cs_pin, OUTPUT);

  pinMode(grn1, OUTPUT);
  pinMode(grn2, OUTPUT);
  pinMode(grn3, OUTPUT);
  pinMode(grn4, OUTPUT);
  pinMode(red1, OUTPUT);
  pinMode(red2, OUTPUT);
  pinMode(red3, OUTPUT);
  pinMode(red4, OUTPUT);

  digitalWrite(grn1, HIGH);
  digitalWrite(red1, LOW);
  digitalWrite(grn2, HIGH);
  digitalWrite(red2, LOW);
  digitalWrite(grn3, HIGH);
  digitalWrite(red3, LOW);
  digitalWrite(grn4, HIGH);
  digitalWrite(red4, LOW);
  
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
  digitalWrite(sda_pin, LOW);
  digitalWrite(sck_pin, LOW);
  digitalWrite(cs_pin, HIGH);
} // End of setup.


// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    //String newValue = "Time since boot: " + String(millis()/1000);
    //Serial.println("Setting new characteristic value to \"" + newValue + "\"");
    // Set the characteristic's value to be the array of bytes that is actually a string.
    //pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }
  delay(1000); // Delay a second between loops.
} // End of loop

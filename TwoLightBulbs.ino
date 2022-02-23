#include "HomeSpan.h"         // Always start by including the HomeSpan library
#include "BLEDevice.h"
//#include "BLEScan.h"

// The remote service we wish to connect to.
static BLEUUID serviceUUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("6e400002-b5a3-f393-e0a9-e50e24dcca9e");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.println((char*)pData);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
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
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
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
  } // onResult
}; // MyAdvertisedDeviceCallbacks


struct BigLight: Service::LightBulb{

  int lightNumber;                               // store the pin number connected to a hypothetical relay that turns the Table Lamp on/off
  SpanCharacteristic *lightPower;             // store a reference to the On Characteristic
  
  BigLight(int lightNumber) : Service::LightBulb(){       // constructor() method for TableLamp defined with one parameter.  Note we also call the constructor() method for the LightBulb Service.

    lightPower=new Characteristic::On();      // instantiate the On Characteristic and save it as lampPower
    this->lightNumber=lightNumber;                   // save the pin number for the hypothetical relay
  //  pinMode(lampPin,OUTPUT);                 // configure the pin as an output using the standard Arduino pinMode function                       
    
  } // end constructor()
  
  boolean update(){                          // update() method

   // digitalWrite(lampPin,lampPower->getNewVal());      // use standard Arduino digitalWrite function to change the ledPin to either high or low based on the value requested by HomeKit
      clickButton(lightPower->getNewVal(),lightNumber);
      delay(100);
    return(true);                            // return true to let HomeKit (and the Home App Client) know the update was successful
  
  } // end update()
  
};

void setup() {

  // Example 2 expands on Example 1 by implementing two LightBulbs, each as their own Accessory
 
  Serial.begin(115200);

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
  homeSpan.begin(Category::Lighting,"HomeSpan LightBulbs");  // initialize HomeSpan - note the name is now "HomeSpan LightBulbs"

  // Here we create the first LightBulb Accessory just as in Example 1

  new SpanAccessory();                            // Begin by creating a new Accessory using SpanAccessory(), which takes no arguments 
  
    new Service::AccessoryInformation();            // HAP requires every Accessory to implement an AccessoryInformation Service, which has 6 required Characteristics
      new Characteristic::Name("My Left Lamp");      // Name of the Accessory, which shows up on the HomeKit "tiles", and should be unique across Accessories
      new Characteristic::Manufacturer("SUPERCOM");   // Manufacturer of the Accessory (arbitrary text string, and can be the same for every Accessory)
      new Characteristic::SerialNumber("1122-221");    // Serial Number of the Accessory (arbitrary text string, and can be the same for every Accessory)
      new Characteristic::Model("30-Volt Lamp");     // Model of the Accessory (arbitrary text string, and can be the same for every Accessory)
      new Characteristic::FirmwareRevision("0.9");    // Firmware of the Accessory (arbitrary text string, and can be the same for every Accessory) 
      new Characteristic::Identify();                 // Create the required Identify  
      
    new Service::HAPProtocolInformation();          // Create the HAP Protcol Information Service  
      new Characteristic::Version("1.1.0");           // Set the Version Characteristicto "1.1.0" as required by HAP

    //new Service::LightBulb();                       // Create the Light Bulb Service
   //   new Characteristic::On();                       // This Service requires the "On" Characterstic to turn the light on and off
        new BigLight(0);
  // Now we create a second Accessory, which is just a duplicate of Accessory 1 with the exception of changing the Name from "My Table Lamp" to "My Floor Lamp"

  new SpanAccessory();                            // Begin by creating a new Accessory using SpanAccessory(), which takes no arguments 
  
    new Service::AccessoryInformation();            // HAP requires every Accessory to implement an AccessoryInformation Service, which has 6 required Characteristics
      new Characteristic::Name("My Right Lamp");      // Name of the Accessory, which shows up on the HomeKit "tiles", and should be unique across Accessories
      new Characteristic::Manufacturer("SUPERCOM");   // Manufacturer of the Accessory (arbitrary text string, and can be the same for every Accessory)
      new Characteristic::SerialNumber("1122-222");    // Serial Number of the Accessory (arbitrary text string, and can be the same for every Accessory)
      new Characteristic::Model("30-Volt Lamp");     // Model of the Accessory (arbitrary text string, and can be the same for every Accessory)
      new Characteristic::FirmwareRevision("0.9");    // Firmware of the Accessory (arbitrary text string, and can be the same for every Accessory) 
      new Characteristic::Identify();                 // Create the required Identify  
      
    new Service::HAPProtocolInformation();          // Create the HAP Protcol Information Service  
      new Characteristic::Version("1.1.0");           // Set the Version Characteristicto "1.1.0" as required by HAP

    //new Service::LightBulb();                       // Create the Light Bulb Service
     // new Characteristic::On();                       // This Service requires the "On" Characterstic to turn the light on and off
      new BigLight(1);
  // That's it - our device now has two Accessories!

  // IMPORTANT: You should NOT have to re-pair your device with HomeKit when moving from Example 1 to Example 2.  HomeSpan will note
  // that the Attribute Database has been updated, and will broadcast a new configuration number when the program restarts.  This should
  // cause all iOS and MacOS HomeKit Controllers to automatically update and reflect the new configuration above.

} // end of setup()

//////////////////////////////////////

void loop(){
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
  //  String newValue = "Time since boot: " + String(millis()/1000);
  //  Serial.println("Setting new characteristic value to \"" + newValue + "\"");
    
    // Set the characteristic's value to be the array of bytes that is actually a string.
    //pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
    //clickButton(true,0);
    homeSpan.poll();         // run HomeSpan!
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }
  
  
} // end of loop()

uint8_t buffer[6]={0x01,0x40,0x01,0x00,0x01,0x01};
void clickButton(boolean state,int button){
 // buffer
  if(state){
    buffer[3]=0x01; 
   }else{
    buffer[3]=0x00; 
    }
 //  if(button==0){
   // buffer[5]=0x00;
 // }else{
//    buffer[4]=0x01; 
   // buffer[5]=0x01; 
   // }
    buffer[5]=button;
  //01 40 01 xx 01 yy
 // Serial.print("state");
 //  Serial.println(buffer[3],HEX);
 //  Serial.print("button");
 //  Serial.println(buffer[5],HEX);
 for(int i=0;i<6;i++){
  Serial.print(buffer[i],HEX);
  }
  Serial.println();
  pRemoteCharacteristic->writeValue(buffer, 6);
}

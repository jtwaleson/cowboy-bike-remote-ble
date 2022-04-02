
/**

  This was based on the following sample:




  NimBLE_Secure_Client Demo:

   This example demonstrates the secure passkey protected conenction and communication between an esp32 server and an esp32 client.
   Please note that esp32 stores auth info in nvs memory. After a successful connection it is possible that a passkey change will be ineffective.
   To avoid this clear the memory of the esp32's between security testings. esptool.py is capable of this, example: esptool.py --port /dev/ttyUSB0 erase_flash.

    Created: on Jan 08 2021
        Author: mblasee
*/

static uint8_t unlockCmd[] = { 1 };
static uint8_t lockCmd[] = { 0 };
static uint8_t lightsOffCmd[] = {0x0A, 0x10, 0x00, 0x01, 0x00, 0x01, 0x02, 0x00, 0x00, 0xD4, 0xB1};
static uint8_t lightsOnCmd[] = {0x0A, 0x10, 0x00, 0x01, 0x00, 0x01, 0x02, 0x00, 0x01, 0x15, 0x71};

#include <NimBLEDevice.h>
#define   CONFIG_NIMBLE_CPP_ENABLE_RETURN_CODE_TEXT


class ClientCallbacks : public NimBLEClientCallbacks
{
    uint32_t onPassKeyRequest()
    {
      Serial.println("Client Passkey Request");
      /** return the passkey to send to the server */
      /** Change this to be different from NimBLE_Secure_Server if you want to test what happens on key mismatch */
      return YOUR_6_DIGIT_PASSCODE; // e.g. 123456, no quotes.
    };
    void onAuthenticationComplete(ble_gap_conn_desc* desc) {
      Serial.println("Auth completed");
      if (!desc->sec_state.encrypted) {
        Serial.println("Encrypt connection failed - disconnecting");
        /** Find the client with the connection handle provided in desc */
        NimBLEDevice::getClientByID(desc->conn_handle)->disconnect();
        return;
      }
    };
    bool onConfirmPIN(uint32_t pass_key) {
      Serial.print("The passkey YES/NO number: ");
      Serial.println(pass_key);
      /** Return false if passkeys don't match. */
      return true;
    };
};
static ClientCallbacks clientCB;

static void uglyCallback(NimBLERemoteCharacteristic* characteristic, unsigned char* data, unsigned int len, bool q) {
Serial.print("Callback data for handle: ");
Serial.println(characteristic->getHandle());
for (int i = 0; i < len; i++) {
  Serial.print(data[i], HEX);
  Serial.print(" ");
}
Serial.println();
Serial.println();
}
//
//class NotifyCallbacks : public NimBLECharacteristicCallbacks 
//{
//    void onRead (NimBLECharacteristic *pCharacteristic)
//    {
//      Serial.println("onRead");
//    };
//    void onWrite (NimBLECharacteristic *pCharacteristic)
//    {
//      Serial.println("onWri");
//    };
//    void onNotify (NimBLECharacteristic *pCharacteristic)
//    {
//      Serial.println("onNotify");
//    };
//    void onStatus (NimBLECharacteristic *pCharacteristic)
//    {
//      Serial.println("onStatus");
//    };
//};
//static NotifyCallbacks notifyCB;

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting NimBLE Client");

  NimBLEDevice::init("");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  NimBLEDevice::setSecurityAuth(false, true, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_ONLY);
  //  NimBLEScan *pScan = NimBLEDevice::getScan();
  //  NimBLEScanResults results = pScan->start(5);

  NimBLEUUID serviceUuid("c0b0a000-18eb-499d-b266-2f2910744274");
  NimBLEUUID unlockChar("c0b0a001-18eb-499d-b266-2f2910744274");
  NimBLEUUID uartServiceUuid("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
  NimBLEUUID writeUartChar("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
  NimBLEUUID notifyUartChar("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

  //  for (int i = 0; i < results.getCount(); i++)
  //  {
  //    NimBLEAdvertisedDevice device = results.getDevice(i);
  //    Serial.println(device.getName().c_str());
  //
  //    if (device.isAdvertisingService(serviceUuid))
  //    {
  NimBLEClient *pClient = NimBLEDevice::createClient(NimBLEAddress("xx:xx:xx:xx:xx:xx", 1)); TODO your mac address here
  pClient->setClientCallbacks(&clientCB, false);

  Serial.println("Connecting");
  //      if (pClient->connect(&device))
  if (pClient->connect())
  {
    Serial.println("Connection, going into secure mode");
    pClient->secureConnection();
    Serial.println("waiting a bit..");
    Serial.println("Getting services");
//    pClient->discoverAttributes();
    Serial.print("MTU: ");
    Serial.println(pClient->getMTU());

//    std::vector< NimBLERemoteService * > *allServices = pClient->getServices(false);
//    for (std::vector<NimBLERemoteService>::size_type i = 0; i != allServices->size(); i++) {
//      NimBLERemoteService *service = allServices->at(i);
//      Serial.println(service->toString().c_str());
//      std::vector< NimBLERemoteCharacteristic * > *allCharacteristics = service->getCharacteristics(false);
//      for (std::vector<NimBLERemoteCharacteristic>::size_type j = 0; j != allCharacteristics->size(); j++) {
//        NimBLERemoteCharacteristic *characteristic = allCharacteristics->at(j);
//        Serial.print("- ");
////        Serial.print(characteristic->toString().c_str());
//        Serial.print(characteristic->getHandle());
//        Serial.print(" [");
//        if (characteristic->canWrite()) {
//          Serial.print("write,");
//        }
//        if (characteristic->canRead()) {
//          Serial.print("read,");
//        }
//        if (characteristic->canWriteNoResponse()) {
//          Serial.print("write-no-response,");
//        }
//        if (characteristic->canNotify()) {
//          Serial.print("notify,");
//        }
//        if (characteristic->canIndicate()) {
//          Serial.print("indicate,");
//        }
//        if (characteristic->canBroadcast()) {
//          Serial.print("broadcast,");
//        }
//        Serial.println("]");
////        std::vector< NimBLERemoteDescriptor  * > *allDescriptors = characteristic->getDescriptors(false);
////        for (std::vector<NimBLERemoteDescriptor >::size_type k = 0; k != allDescriptors->size(); k++) {
////          NimBLERemoteDescriptor  *descriptor = allDescriptors->at(i);
////          Serial.print("  #");
////          Serial.println(descriptor->toString().c_str());
////        }
//      }
//        Serial.println("");
//      Serial.println("____________________________________");
//      
//      continue;
//
//    }
        Serial.println("");
        Serial.println("");
        Serial.println("");
        Serial.println("");
    
    NimBLERemoteService *pService = pClient->getService(serviceUuid);
    NimBLERemoteService *pUartService = pClient->getService(uartServiceUuid);
    if (pService != nullptr)
    {
      NimBLERemoteCharacteristic *pUnlock = pService->getCharacteristic(unlockChar);
      NimBLERemoteCharacteristic *pWrite = pUartService->getCharacteristic(writeUartChar);
      NimBLERemoteCharacteristic *pRx = pUartService->getCharacteristic(notifyUartChar);

      pRx->subscribe(true, &uglyCallback, false);

      if (pWrite == nullptr) {
        Serial.println("this is a problem!!");
      }
      std::string x = pWrite->toString();
      Serial.println(x.c_str());

      Serial.println("unlock service");
      if (pUnlock == nullptr)
      {
        Serial.println("not found");
      } else {
        Serial.println("current lock status");
        Serial.println("unlocking...");

        pUnlock->writeValue(unlockCmd, 1, false);
        Serial.println("unlocked");
        delay(5 * 1000);
        pWrite->writeValue(lightsOffCmd, 11);
        Serial.println("light off");
        delay(2 * 1000);
        pWrite->writeValue(lightsOnCmd, 11);
        Serial.println("light on");
        delay(2 * 1000);
        pUnlock->writeValue(lockCmd, 1, false);
        Serial.println("locked");
      }
    }
  }
  else
  {
    // failed to connect
    Serial.println("failed to connect");
  }
  Serial.println("stopping and going to deep sleep");
  NimBLEDevice::deleteClient(pClient);
  Serial.flush();
  esp_sleep_enable_timer_wakeup(1000 * 1000000);
  esp_deep_sleep_start();

  //    }
  //  }
}

void loop()
{
}

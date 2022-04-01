
/**

This was based on the following sample:




  NimBLE_Secure_Client Demo:

   This example demonstrates the secure passkey protected conenction and communication between an esp32 server and an esp32 client.
   Please note that esp32 stores auth info in nvs memory. After a successful connection it is possible that a passkey change will be ineffective.
   To avoid this clear the memory of the esp32's between security testings. esptool.py is capable of this, example: esptool.py --port /dev/ttyUSB0 erase_flash.

    Created: on Jan 08 2021
        Author: mblasee
*/

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


void setup()
{
  Serial.begin(115200);
  Serial.println("Starting NimBLE Client");

  NimBLEDevice::init("");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  NimBLEDevice::setSecurityAuth(false, true, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_ONLY);
  NimBLEScan *pScan = NimBLEDevice::getScan();
  NimBLEScanResults results = pScan->start(5);

  NimBLEUUID serviceUuid("c0b0a000-18eb-499d-b266-2f2910744274");
  //  NimBLEUUID unlockChar("C0B0A001-18EB-499D-B266-2F2910744274");
  NimBLEUUID unlockChar("c0b0a001-18eb-499d-b266-2f2910744274");

  for (int i = 0; i < results.getCount(); i++)
  {
    NimBLEAdvertisedDevice device = results.getDevice(i);
    Serial.println(device.getName().c_str());

    if (device.isAdvertisingService(serviceUuid))
    {
      NimBLEClient *pClient = NimBLEDevice::createClient();
      pClient->setClientCallbacks(&clientCB, false);

      Serial.println("Connecting");
      if (pClient->connect(&device))
      {
        Serial.println("Connection, going into secure mode");
        pClient->secureConnection();
        Serial.println("waiting a bit..");
        Serial.println("Getting services");
        NimBLERemoteService *pService = pClient->getService(serviceUuid);
        if (pService != nullptr)
        {
          NimBLERemoteCharacteristic *pUnlock = pService->getCharacteristic(unlockChar);

          Serial.println("unlock service");
          if (pUnlock == nullptr)
          {
            Serial.println("not found");
          } else {
            Serial.println("current lock status");
            Serial.println("unlocking...");
            uint8_t foo [1] = { 1 };

            foo[0] = pUnlock->readValue<uint8_t>() == 0 ? 1 : 0;

            if (pUnlock->writeValue(foo, 1, false)) {
              Serial.println("yes");

            } else {
              Serial.println("no");

            }
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

    }
  }
}

void loop()
{
}

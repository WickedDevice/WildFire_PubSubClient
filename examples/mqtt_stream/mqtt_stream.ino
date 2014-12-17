/*
 Example of using a Stream object to store the message payload

 Uses WildFire_SPIFlash library: https://github.com/WickedDevice/WildFire_SPIFlash

  - connects to an MQTT server
  - publishes "hello world" to the topic "outTopic"
  - subscribes to the topic "inTopic"
*/

#include <SPI.h>
#include <WildFire.h>
#include <WildFire_CC3000.h>
#include <WildFire_PubSubClient.h>
#include <WildFire_SPIFlash.h>

WildFire_CC3000 cc3000;
WildFire wf;

#define WLAN_SSID       "XXXXXXXXXX"
#define WLAN_PASS       "XXXXXXXXXX"

// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

// We're going to set our broker IP and union it to something we can use
union ArrayToIp {
  byte array[4];
  uint32_t ip;
};

ArrayToIp server = { 172, 16, 0, 2 };

WildFire_CC3000_Client client;
WildFire_SPIFlash flash;
WildFire_PubSubClient mqttclient(server.ip, 1883, callback, client, cc3000, flash);

void callback(char* topic, byte* payload, unsigned int length) {
  flash.seek(1);

  // do something with the message
  for(uint8_t i=0; i<length; i++) {
    Serial.write(flash.read());
  }
  Serial.println();

  // Reset position for the next message to be stored
  flash.seek(1);
}

void setup()
{
  wf.begin();
  Serial.begin(115200);
  Serial.println(F("Hello, CC3000!\n"));

  if(flash.initialize()){
    Serial.println(F("SPI Flash Initialized Successfully"));
  }
  else{
    Serial.println(F("SPI Flash Failed To Initialize"));
  }

  connectToNetwork();

  // connect to the broker
  if (!client.connected()) {
    client = cc3000.connectTCP(server.ip, 1883);
  }

  if (mqttclient.connect("arduinoClient")) {
    mqttclient.publish("outTopic","hello world");
    mqttclient.subscribe("inTopic");
  }

  flash.begin();
  flash.seek(1);

}

void loop()
{
  mqttclient.loop();
}

void connectToNetwork(){

  displayDriverMode();

  Serial.println(F("\nInitialising the CC3000 ..."));
  if (!cc3000.begin()) {
    Serial.println(F("Unable to initialise the CC3000! Check your wiring?"));
    for(;;);
  }

  uint16_t firmware = checkFirmwareVersion();

  displayMACAddress();

  Serial.println(F("\nDeleting old connection profiles"));
  if (!cc3000.deleteProfiles()) {
    Serial.println(F("Failed!"));
    while(1);
  }

  /* Attempt to connect to an access point */
  char *ssid = WLAN_SSID;             /* Max 32 chars */
  Serial.print(F("\nAttempting to connect to ")); Serial.println(ssid);

  /* NOTE: Secure connections are not available in 'Tiny' mode! */
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }

  Serial.println(F("Connected!"));

  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP()) {
    delay(100); // ToDo: Insert a DHCP timeout!
  }

  /* Display the IP address DNS, Gateway, etc. */
  while (!displayConnectionDetails()) {
    delay(1000);
  }
}

/**************************************************************************/
/*!
    @brief  Displays the driver mode (tiny of normal), and the buffer
            size if tiny mode is not being used

    @note   The buffer size and driver mode are defined in cc3000_common.h
*/
/**************************************************************************/
void displayDriverMode(void)
{
  #ifdef CC3000_TINY_DRIVER
    Serial.println(F("CC3000 is configure in 'Tiny' mode"));
  #else
    Serial.print(F("RX Buffer : "));
    Serial.print(CC3000_RX_BUFFER_SIZE);
    Serial.println(F(" bytes"));
    Serial.print(F("TX Buffer : "));
    Serial.print(CC3000_TX_BUFFER_SIZE);
    Serial.println(F(" bytes"));
  #endif
}

/**************************************************************************/
/*!
    @brief  Tries to read the CC3000's internal firmware patch ID
*/
/**************************************************************************/
uint16_t checkFirmwareVersion(void)
{
  uint8_t major, minor;
  uint16_t version;

#ifndef CC3000_TINY_DRIVER
  if(!cc3000.getFirmwareVersion(&major, &minor))
  {
    Serial.println(F("Unable to retrieve the firmware version!\r\n"));
    version = 0;
  }
  else
  {
    Serial.print(F("Firmware V. : "));
    Serial.print(major); Serial.print(F(".")); Serial.println(minor);
    version = major; version <<= 8; version |= minor;
  }
#endif
  return version;
}

/**************************************************************************/
/*!
    @brief  Tries to read the 6-byte MAC address of the CC3000 module
*/
/**************************************************************************/
void displayMACAddress(void)
{
  uint8_t macAddress[6];

  if(!cc3000.getMacAddress(macAddress))
  {
    Serial.println(F("Unable to retrieve MAC Address!\r\n"));
  }
  else
  {
    Serial.print(F("MAC Address : "));
    cc3000.printHex((byte*)&macAddress, 6);
  }
}


/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;

  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

#include <EEPROM.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>

//NOTE: DmxSimple.h and Artnet.h buffer sizes decreased froim 512 to 128 due to memory limitations
#include <DmxSimple.h>
#include <Artnet.h>

//When set to false, runs in normal mode
//TODO: Add switch to support configuration mode on power on
bool configurationMode = true;

//define button and status LED pin
const int led_pin_R = 3;
const int led_pin_G = 4;
const int led_pin_B = 5;
const int button_pin = 6;

//EEPROM Paramter Addresses
byte ipEEPROMaddr = 0;
byte macEEPROMaddr = 20;

//Normal Mode MAC Address
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
String mac_str = "DE-AD-BE-EF-FE-ED";

//Normal Mode IP Address
byte ip[] = { 2, 2, 2, 2 };
String ip_str;

struct request {
  String link;
  String param;
  String val;
};

//server port
EthernetServer server(80);

//Artnet-DMX Variables
Artnet artnet;
const int startUniverse = 0;  // CHANGE FOR YOUR SETUP most software this is 1, some software send out artnet first universe as 0.

// Check if we got all universes
//const int maxUniverses = numberOfChannels / 512 + ((numberOfChannels % 512) ? 1 : 0);
const byte maxUniverses = 1;
bool universesReceived[maxUniverses];
bool sendFrame = 1;
int previousDataLength = 0;


void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for Leonardo only
  }
  delay(100);
  Serial.println(F("Initializing..."));
  delay(100);

  //set up status LED and button pins
  // pinMode(led_pin_R, OUTPUT);
  // pinMode(led_pin_G, OUTPUT);
  // pinMode(led_pin_B, OUTPUT);
  // pinMode(button_pin, INPUT);

  //load data from EEPROM
  for (int i = ipEEPROMaddr; i < ipEEPROMaddr + 4; i++) {
    ip[i] = EEPROM.read(i);
  }
  for (int i = 0; i < 6; i++) {
    mac[i] = EEPROM.read(i + macEEPROMaddr);
  }
  delay(100);

  //convert ip and mac bytes to strings
  ip_str = IP_Byte2Str(ip);
  Serial.println(ip_str);
  delay(100);

  mac_str = MAC_Byte2Str(mac);
  Serial.println(mac_str);
  delay(100);


  //check if in configuration or normal mode
  //replace with button or switch
  if (configurationMode) {
    //Configuration Mode
    Serial.println(F("Configuration Mode..."));

    //Configuration Mode MAC/IP
    byte config_mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
    byte config_ip[] = { 192, 168, 1, 99 };

    //Start Server
    Ethernet.begin(config_mac, config_ip);
    server.begin();
    Serial.print(F("server is at "));
    Serial.println(Ethernet.localIP());
  } else {
    //Normal Mode
    Serial.println(F("Normal Mode..."));

    //Start Artnet
    artnet.begin(mac, ip);
    // this will be called for each packet received
    artnet.setArtDmxCallback(onDmxFrame);

    //Configure DMX
    DmxSimple.usePin(3);
    DmxSimple.maxChannel(30);
  }
}


void loop() {
  if (!configurationMode) {
    artnet.read();
  } else {
    configurationLoop();
  }
}

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {
  sendFrame = 1;

  // Store which universe has got in
  if ((universe - startUniverse) < maxUniverses)
    universesReceived[universe - startUniverse] = 1;

  for (int i = 0; i < maxUniverses; i++) {
    if (universesReceived[i] == 0) {
      sendFrame = 0;
      break;
    }
  }

  // read universe and put into the right part of the display buffer
  for (int i = 0; i < length; i++) {
    DmxSimple.write(i + 1, data[i]);
  }
  previousDataLength = length;

  if (sendFrame) {
    // Reset universeReceived to 0
    memset(universesReceived, 0, maxUniverses);
  }
}


void configurationLoop() {
  // Create a client connection
  EthernetClient client = server.available();
  if (client) {
    String readString;
    Serial.println(F("Client Connected"));
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        //read char by char HTTP request

        if (readString.length() < 100) {
          //store characters to string
          readString += c;
          //Serial.print(c);
        }
        //if HTTP request has ended
        if (c == '\n') {
          Serial.print(F("Received: "));
          Serial.println(readString);  //print to serial monitor for debuging

          request r;
          parseRequest(readString, r);
          Serial.println(F("Parse Request"));
          Serial.print(F("Link: "));
          Serial.println(r.link);
          Serial.print(F("Param: "));
          Serial.println(r.param);
          Serial.print(F("Value: "));
          Serial.println(r.val);

          if (r.param.equals("ip_address")) {
            if (validateIP(r.val)) {
              ip_str = r.val;
            }
          } else if (r.param.equals("mac_address")) {
            if (validateMAC(r.val)) {
              r.val.toUpperCase();
              mac_str = r.val;
            }
          }

          //clearing string for next read
          readString = "";

          //Handle Webpages
          if (r.link.equals("/edit-ip")) {
            loadEditPage(client, "ip_address", "IP Address", ip_str);
            Serial.println(F("Loading Edit IP Page"));

          } else if (r.link.equals("/favicon.ico")) {
            //Ignore favicon request
          } else if (r.link.equals("/edit-mac")) {
            loadEditPage(client, "mac_address", "MAC Address", mac_str);
            Serial.println(F("Loading Edit MAC Page"));
          } else {
            loadHomePage(client);
            Serial.println(F("Loading Home Page"));
          }

          //stopping client
          delay(1);
          client.stop();
        }
      }
    }
  }
}

void loadHomePage(EthernetClient client) {
  client.println(F("HTTP/1.1 200 OK"));  //send new page
  client.println(F("Content-Type: text/html"));
  client.println();
  client.println(F("<HTML>"));
  client.println(F("<HEAD>"));
  client.println(F("<TITLE>Artnet to DMX Converter</TITLE>"));
  client.println(F("</HEAD>"));
  client.println(F("<BODY>"));
  client.println(F("<H1>Artnet to DMX Converter</H1>"));
  client.print(F("<p>IP Address: "));
  client.print(ip_str);
  client.println(F("   <a href=\"/edit-ip\">Edit</a></p>"));
  client.print(F("<p>MAC Address: "));
  client.print(mac_str);
  client.println(F("   <a href=\"/edit-mac\">Edit</a></p>"));
  client.println(F("<br />"));
  client.println(F("</BODY>"));
  client.println(F("</HTML>"));
}

void loadEditPage(EthernetClient client, String id, String label, String def) {
  client.println(F("HTTP/1.1 200 OK"));  //send new page
  client.println(F("Content-Type: text/html"));
  client.println();
  client.println(F("<HTML>"));
  client.println(F("<HEAD>"));
  client.println(F("<TITLE>Artnet to DMX Converter IP EDIT</TITLE>"));
  client.println(F("</HEAD>"));
  client.println(F("<BODY>"));
  client.println(F("<H1>Artnet to DMX Converter</H1>"));
  client.print(F("<H2>Configure "));
  client.print(label);
  client.println(F("</H2>"));
  client.println(F("<hr />"));
  client.println(F("<br />"));
  client.println(F("<form action=\"/\" method=\"get\">"));
  client.print(F("<label for=\""));
  client.print(id);
  client.print(F("\">"));
  client.print(label);
  client.println(F(":</label><br>"));
  client.print(F("<input type=\"text\" maxlength=\"20\" name=\""));
  client.print(id);
  client.print(F("\" id=\""));
  client.print(id);
  client.print(F("\" value="));
  client.print(def);
  client.println(F("><br>"));
  client.println(F("<input type=\"submit\" name\"submit\" value=\"Submit\">"));
  client.println(F("</form>"));
  client.println(F("<br />"));
  client.println(F("</BODY>"));
  client.println(F("</HTML>"));
}

bool validateIP(const String& ip) {
  String ipOctets[4];
  byte octetInd = 0;
  for (int i = 0; i < ip.length(); i++) {
    char curChar = ip.charAt(i);
    if (curChar == '.') {
      //return if empty octet
      if (ipOctets[octetInd].length() == 0) {
        return false;
      }
      octetInd++;
    } else {
      //return if any non-number characters
      if (!(!isAlpha(curChar) & isAlphaNumeric(curChar))) {
        return false;
      }
      ipOctets[octetInd] += curChar;
    }
  }
  if (octetInd != 3) {
    return false;
  }

  //convert to int
  int ipOctetInts[4];
  for (byte i = 0; i < 4; i++) {
    ipOctetInts[i] = ipOctets[i].toInt();
    if (ipOctetInts[i] > 255 || ipOctetInts[i] < 0) {
      //return if number out of octet range
      return false;
    }
  }

  //save to EEPROM
  for (byte i = 0; i < 4; i++) {
    EEPROM.write(i + ipEEPROMaddr, byte(ipOctetInts[i]));
  }

  return true;
}

bool validateMAC(const String& mac) {
  String macOctets[6];
  int octetInd = 0;
  // mac = mac + "-";
  for (int i = 0; i < mac.length(); i++) {
    char curChar = mac.charAt(i);
    if (curChar == '-') {
      //return if empty octet
      if (macOctets[octetInd].length() == 0) {
        return false;
      }
      octetInd++;
    } else {
      //return if any non-hex characters
      if (!isHexadecimalDigit(curChar)) {
        return false;
      }
      macOctets[octetInd] += curChar;
    }
  }
  if (octetInd != 5) {
    return false;
  }

  //convert to int
  int macOctetInts[6];
  for (int i = 0; i < 6; i++) {
    char c[5] = "0000\0";
    macOctets[i].toCharArray(c, 5);
    Serial.print("String: ");
    Serial.println(c);
    macOctetInts[i] = (int)strtol(c, NULL, 16);
    Serial.print("Number: ");
    Serial.println(macOctetInts[i]);
    if (macOctetInts[i] > 255 || macOctetInts[i] < 0) {
      //return if octet out of range
      return false;
    }
  }

  //save to EEPROM
  for (int i = 0; i < 6; i++) {
    EEPROM.write(i + macEEPROMaddr, byte(macOctetInts[i]));
    Serial.println(macOctetInts[i]);
  }
  return true;
}

String IP_Byte2Str(byte ipBytes[]) {
  String IP_str = "";

  for (int i = 0; i < 4; i++) {
    IP_str = IP_str + String(int(ipBytes[i]));
    if (i < 3) {
      IP_str = IP_str + ".";
    }
  }
  return IP_str;
}

String MAC_Byte2Str(byte macBytes[]) {
  String MAC_str = "";

  for (int i = 0; i < 6; i++) {
    MAC_str = MAC_str + String(int(macBytes[i]), HEX);
    if (i < 5) {
      MAC_str = MAC_str + "-";
    }
  }
  MAC_str.toUpperCase();
  return MAC_str;
}

void parseRequest(const String& str, const request& r) {

  byte linkInd = str.indexOf(" ", 4);
  r.link = str.substring(4, linkInd);
  Serial.print(F("Link: "));
  Serial.println(r.link);

  byte hasQ = r.link.indexOf("?");
  byte hasEq = r.link.indexOf("=");
  if (hasQ > 0 && hasEq > 0) {
    r.param = r.link.substring(hasQ + 1, hasEq);
    Serial.print(F("Paramter: "));
    Serial.println(r.param);
    r.val = r.link.substring((hasEq + 1));
    Serial.print(F("Value: "));
    Serial.println(r.val);
  }
}

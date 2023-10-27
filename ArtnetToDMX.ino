#include <EEPROM.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <DmxSimple.h>
#include <Artnet.h>

//EEPROM Paramter Addresses
byte ipEEPROMaddr = 0;
byte macEEPROMaddr = 20;

//Normal Mode MAC/IP
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
String mac_str = "DE-AD-BE-EF-FE-ED";

byte ip[] = { 2, 2, 2, 2 };
String ip_str;

EthernetServer server(80);  //server port
String readString;

bool configurationMode = true;

//Artnet-DMX Variables
//NOTE: the artnet library header file was modified to reduce the buffer size from 530 to 30. This is a workaround for reducing the memory.
Artnet artnet;
const int startUniverse = 0;  // CHANGE FOR YOUR SETUP most software this is 1, some software send out artnet first universe as 0.

// Check if we got all universes
//const int maxUniverses = numberOfChannels / 512 + ((numberOfChannels % 512) ? 1 : 0);
const int maxUniverses = 1;
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
    configurationMode = true;

    //Configuration Mode MAC/IP
    byte config_mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
    byte config_ip[] = { 192, 168, 1, 99 };

    Serial.println(F("Configuration Mode..."));
    Ethernet.begin(config_mac, config_ip);
    server.begin();
    Serial.print(F("server is at "));
    Serial.println(Ethernet.localIP());
  } else {
    Serial.println(F("Normal Mode..."));
    artnet.begin(mac, ip);
    // this will be called for each packet received
    artnet.setArtDmxCallback(onDmxFrame);

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


          int linkInd = readString.indexOf(" ", 4);
          String link = readString.substring(4, linkInd);
          Serial.print(F("Link: "));
          Serial.println(link);

          int hasQ = link.indexOf("?");
          int hasEq = link.indexOf("=");
          if (hasQ > 0 && hasEq > 0) {
            String param = link.substring(hasQ + 1, hasEq);
            Serial.print(F("Paramter: "));
            Serial.println(param);
            String val = link.substring((hasEq + 1));
            Serial.print(F("Value: "));
            Serial.println(val);

            if (param.equals("ip_address")) {
              if (validateIP(val)) {
                ip_str = val;
              }
            } else if (param.equals("mac_address")) {
              if (validateMAC(val)) {
                val.toUpperCase();
                mac_str = val;
              }
            }
          }
          //clearing string for next read
          readString = "";

          if (link.equals("/edit-ip")) {
            loadEditPage(client, "ip_address", "IP Address", ip_str);
            Serial.println(F("Loading Edit IP Page"));

          } else if (link.equals("/favicon.ico")) {
          } else if (link.equals("/edit-mac")) {
            loadEditPage(client, "mac_address", "MAC Address", mac_str);
            Serial.println(F("Loading Edit MAC Page"));
          } else {
            loadHomePage(client);
            Serial.println(F("Loading Home Page"));
          }
          delay(1);
          //stopping client
          client.stop();
          //controls the Arduino if you press the buttons
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
  client.print(F("<input type=\"text\" name=\""));
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

bool validateIP(String ip) {
  String ipOctets[4];
  int octetInd = 0;
  ip = ip + ".";
  for (int i = 0; i < ip.length(); i++) {
    char curChar = ip.charAt(i);
    // Serial.println(curChar);
    if (curChar == '.') {
      // Serial.println("Octet " + String(octetInd) + ": " + ipOctets[octetInd]);
      if (ipOctets[octetInd].length() == 0) {
        Serial.println(F("no characters in this octet"));
        return false;
      }
      octetInd++;
    } else {
      //return if any non-number characters
      if (!(!isAlpha(curChar) & isAlphaNumeric(curChar))) {
        Serial.println(F("Not number"));
        return false;
      }
      ipOctets[octetInd] += curChar;
    }
  }
  if (octetInd != 4) {
    return false;
  }

  //convert to int
  int ipOctetInts[4];
  for (int i = 0; i < 4; i++) {
    ipOctetInts[i] = ipOctets[i].toInt();
    Serial.println(ipOctetInts[i]);
    if (ipOctetInts[i] > 255 || ipOctetInts[i] < 0) {
      Serial.println(F("Number out of range"));
      return false;
    }
  }

  //save to EEPROM
  for (int i = 0; i < 4; i++) {
    EEPROM.write(i + ipEEPROMaddr, byte(ipOctetInts[i]));
  }

  return true;
}

bool validateMAC(String mac) {
  String macOctets[6];
  int octetInd = 0;
  mac = mac + "-";
  for (int i = 0; i < mac.length(); i++) {
    char curChar = mac.charAt(i);
    // Serial.println(curChar);
    if (curChar == '-') {
      // Serial.println("Octet " + String(octetInd) + ": " + macOctets[octetInd]);
      if (macOctets[octetInd].length() == 0) {
        Serial.println(F("no characters in this octet"));
        return false;
      }
      octetInd++;
    } else {
      //return if any non-hex characters
      if (!isHexadecimalDigit(curChar)) {
        Serial.println(F("Not Hexademical"));
        return false;
      }
      macOctets[octetInd] += curChar;
    }
  }
  if (octetInd != 6) {
    return false;
  }

  //convert to int
  int macOctetInts[6];
  for (int i = 0; i < 6; i++) {
    char c[3];
    macOctets[i].toCharArray(c, 3);
    macOctetInts[i] = (int)strtol(c, NULL, 16);
    Serial.println(macOctetInts[i]);
    if (macOctetInts[i] > 255 || macOctetInts[i] < 0) {
      Serial.println(F("Number out of range"));
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
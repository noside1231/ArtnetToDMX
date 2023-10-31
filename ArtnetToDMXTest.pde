
import java.util.*;
import oscP5.*;
import controlP5.*;
import ch.bildspur.artnet.*;

//OSC
OscP5 oscP5;
int OSCport = 9998;

//Artnet
ArtNetClient artnet;
byte[] dmxData = new byte[40];
String artnetAddress = "192.168.1.6";

//Serial
//import processing.serial.*;

// The serial port:
//Serial myPort;



//ControlP5
ControlP5 cp5;

Slider R_Slider;
Slider G_Slider;
Slider B_Slider;


void setup() {

  size(1400, 800, P3D);

  frameRate(25);

  for (int i = 0; i < dmxData.length; i++) {
    dmxData[i] = 0;
  }

  artnet = new ArtNetClient(null, 6454, 6454);
  //artnet = new ArtNetClient(null, 8888, 8888);
  artnet.start();

  //printArray(Serial.list());
  //myPort = new Serial(this, Serial.list()[15], 9600);

  oscP5 = new OscP5(this, OSCport);

  cp5 = new ControlP5(this);
  PFont font = createFont("Arial", 12, false); // use true/false for smooth/no-smooth
  ControlFont pfont = new ControlFont(font, 12);

  int slider2Dscale = height/5;
  int sliderscale = height/5;
  int buttonScale = 30;
  int knobScale = height / 10;

  R_Slider = cp5.addSlider("Red").setPosition(10, 10).setSize(20, 100).setFont(pfont).setValue(0).setId(0);
  G_Slider = cp5.addSlider("Green").setPosition(80, 10).setSize(20, 100).setFont(pfont).setValue(0).setId(1);
  B_Slider = cp5.addSlider("Blue").setPosition(150, 10).setSize(20, 100).setFont(pfont).setValue(0).setId(2);
}

void draw() {
  background (0);


  int curTime = millis();

  dmxData[0] = (byte) (map(R_Slider.getValue(), 0, 100, 0, 255));
  dmxData[1] = (byte) (map(G_Slider.getValue(), 0, 100, 0, 255));
  dmxData[2] = (byte) (map(B_Slider.getValue(), 0, 100, 0, 255));







  //myPort.write(dmxData[5]);

  //for (int i = 0; i < 20; i++) {
  //  print(binary(dmxData[i]) + " ");
  //}
  //println();



  artnet.unicastDmx(artnetAddress, 0, 0, dmxData);
}



void oscEvent(OscMessage theOscMessage) {
  String[] m = new String[4];
  m = split(theOscMessage.addrPattern(), '/');
  String id = m[1].toLowerCase(); //Device
  String arg = m[2].toLowerCase(); //Argument
  String param = ""; //value
  if (m.length == 4) {
    param = m[3].toLowerCase();
  }
  int value = 0;
  try {
    value = (int) (100*theOscMessage.get(0).floatValue());
    println("VALUE: " + value);
  }
  catch (Exception E) {
  }
  String test = "";
  try {
    test = theOscMessage.get(0).stringValue();
    println("VALUE: " + test);
    value = 10;
  }
  catch (Exception E) {
  }


  println(m);
  println("ID: " + id);
  println("ARG: " + arg);
}

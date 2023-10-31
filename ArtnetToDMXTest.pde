
import java.util.*;
import controlP5.*;
import ch.bildspur.artnet.*;

//Artnet
ArtNetClient artnet;
byte[] dmxData = new byte[3];
String artnetAddress = "192.168.1.6";

//ControlP5
ControlP5 cp5;

Slider R_Slider;
Slider G_Slider;
Slider B_Slider;


void setup() {

  size(200, 140, P3D);

  for (int i = 0; i < dmxData.length; i++) {
    dmxData[i] = 0;
  }

  artnet = new ArtNetClient(null, 6454, 6454);
  artnet.start();

  cp5 = new ControlP5(this);
  PFont font = createFont("Arial", 12, false); // use true/false for smooth/no-smooth
  ControlFont pfont = new ControlFont(font, 12);

  R_Slider = cp5.addSlider("Red").setPosition(10, 10).setSize(20, 100).setFont(pfont).setValue(0).setId(0);
  G_Slider = cp5.addSlider("Green").setPosition(80, 10).setSize(20, 100).setFont(pfont).setValue(0).setId(1);
  B_Slider = cp5.addSlider("Blue").setPosition(150, 10).setSize(20, 100).setFont(pfont).setValue(0).setId(2);
}

void draw() {
  background (0);

  dmxData[0] = (byte) (map(R_Slider.getValue(), 0, 100, 0, 255));
  dmxData[1] = (byte) (map(G_Slider.getValue(), 0, 100, 0, 255));
  dmxData[2] = (byte) (map(B_Slider.getValue(), 0, 100, 0, 255));

  artnet.unicastDmx(artnetAddress, 0, 0, dmxData);
}

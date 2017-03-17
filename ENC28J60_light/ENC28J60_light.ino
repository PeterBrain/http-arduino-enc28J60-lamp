/*
 * Special thanks to:
 * Nick Farina (nfarina) - Creator of Homebridge https://github.com/nfarina/homebridge
 * to Adrian Rudman (rudders) - Creator of http-plugin https://github.com/rudders/homebridge-http
 * to Jean-Claude Wippler (jcw) - Creator of the EtherCard Library https://github.com/jcw/ethercard
 * and to -->ME<--... i'm genious (pretty large ego) ;P
 * 
 * If anyone of you has got some superior skills and knows a more efficient way to code,
 * you are welcome to create an issue
 * 
 * 
 * Pin connection Diagramm for ENC28J60
 *  CS - Pin 10
 *  SI - Pin 11
 *  SO - Pin 12
 *  SCK - Pin 13
 *  GND - GND
 *  VCC - 3.3V
 * 
 * 
 * LED+ (with 270Ω Resistor) - Digital 9 //I didn't calculate the resistor value, but the led is still fine
 * LED- - GND
 * 
 * Physical switch - Digital 8
 */

#include <EtherCard.h> //Library downloaded from https://github.com/jcw/ethercard

static byte mymac[] = {0x74,0x69,0x69,0x2D,0x30,0x31}; //ethernet mac address - must be unique on your network
static byte myip[] = {10,0,0,20}; //ethernet interface ip address
static byte gwip[] = {10,0,0,138}; //gateway ip address

byte Ethernet::buffer[500]; //tcp/ip send and receive buffer
BufferFiller bfill;

//output pin o arduino
int light;
int d_input;

//global
double out; //global variable for the brightness level at or shortly before output
//double out_before; //for smooth brihgtness change... this variable contains the previous output value
int state; //not only for debugging... even to remember, if the light is on or off
int state_before; //previous state

//brightness
int count_direction; //to get a smooth brightness change, we have to know in which direction it should change - 1 is up, 0 is down
int i_smooth; //counting value in for loop
int brightness_delay; //delay between after one brightness step

//physical
int io_before; //this variable contains the previous 
int io; //is the value of the external Input or physical Switch (digitalInput Pin)
int phys; //handover variable

void setup() {
  //set pins
  light = 9;
  d_input = 8;

  //global
  out = 0;
  state = 0;
  state_before = 0;
  
  //brightness
  i_smooth = 0;
  brightness_delay = 2; //in ms

  //physical
  io_before = 0;
  io = 0;
  phys = 0;
  
  Serial.begin(9600); //debugging settings
  pinMode(light, OUTPUT); //define output pin
  pinMode(d_input, INPUT_PULLUP);
  //pullup is required, because there is no reference value
  //(at a relais - is it possible connect it as an opener (NO instead of NC) and forget "_PULLUP"??)

  //initialize Ethernet
  ether.begin(sizeof Ethernet::buffer, mymac);
  ether.staticSetup(myip, gwip);
}

void loop() {
  word len = ether.packetReceive(); //copy received packets to data tbuffer
  word pos = ether.packetLoop(len); //parse received data

  io = digitalRead(d_input); //read digital Pin (physical Input)

  if (io != io_before) {
    if (io == HIGH) {
      phys = 0; //return value if physical input is 1
    } else {phys = 1;}
  } else {phys = 3;} //this is needed to simulate single pulse
  delay(brightness_delay);
  io_before = io; //this is needed to simulate single pulse

  char* pos_buffer = (char *) Ethernet::buffer + pos;
  char* on = strstr(pos_buffer, "GET /on"); //detects on command
  char* off = strstr(pos_buffer, "GET /off"); //detects off command
  int io_status_cmp = strncmp("GET /io_status/", pos_buffer, 15); //compare header with snippet to detect if lamp is on or off
  int lvl_status_cmp = strncmp("GET /lvl_status/", pos_buffer, 16); //compare header with snippet to detect if there is a brightness set
  int lvl = strncmp("GET /lvl/", pos_buffer, 9); //compare header with snippet to detect if there is a brightness set - 0 is equal
  
  if ((off != 0 || phys == 0) && state == 1) { //if conditions for switching off
    smooth_brightness(state, 0, 1);
    state = 0;
  }
  
  else if (on != 0 || phys == 1 || lvl == 0) { //if conditions for switching on or change brightness
    
    if (lvl == 0) {
      char* brightness_val = strtok(pos_buffer, "GET /lvl/"); //seperates percentage value from response
      out = atoi(brightness_val); //converts brightness value from char to integer
      out = (out/100)*255; //convert 0-100% into 0-255 range
    }

    if (phys == 1) {out = 255;}
    //physical switch always set to 100% or any other fixed value

    smooth_brightness(state, out, 0);
    state = 1;
  }
  
  if (pos) { //check if valid tcp data is received
    int out_lvl = (out/255)*100; //convert 0-255 range to 0-100 range
    
    if (io_status_cmp == 0) {ether.httpServerReply(status_io_reply(state));} //status reply for on/off
    else if (lvl_status_cmp == 0) {ether.httpServerReply(status_lvl_reply(out_lvl));} //status reply for brightness level
    else {ether.httpServerReply(reply());} //send (web page data) reply, essential for homebridge
  }
}

//i didn't like the aggressive change of brightness... for loop at its finest
int smooth_brightness(int current_state, int output, int on_off) {
  if (current_state == 0) {i_smooth = 0;} //set loop end condition to 0 to turn lamp off
  
  //brightness up- or downwards
  if (i_smooth > output || on_off == 1) {count_direction = 0;}
  else if (i_smooth < output) {count_direction = 1;}
  
  if (count_direction == 1) { 
    for (i_smooth; i_smooth <= output; i_smooth++) {
      analogWrite(light, i_smooth);
      delay(brightness_delay);
    }
  } else {
    for (i_smooth; output <= i_smooth; i_smooth--) {
      analogWrite(light, i_smooth);
      delay(brightness_delay);
    }
  }
  
  output = i_smooth;
}

//can be turned into interface for switching on or off and brightness... or only to display accessory uptime
static word reply() {
  long t = millis() / 1000; //
  word d = t / 86400; //days
  word h = t / 3600; //hours
  byte m = (t / 60) % 60; //minutes
  byte s = t % 60; //seconds
  bfill = ether.tcpOffset();
  bfill.emit_p(PSTR(
    "HTTP/1.0 200 OK\r\n" //essential, otherwise homebridge will get no reply (must be 200, i think)
    "Content-Type: text/html\r\n"
    "Pragma: no-cache\r\n"
    "\r\n" //put html after this
    //"<meta http-equiv='refresh' content='1'/>"
    "<title>HomeKit | Homebridge | ENC28J60 | RaspberryPI | Arduino</title>"
    "<h1>Accessory uptime: $D Days $D$D:$D$D:$D$D</h1>" //shows uptime of the accessory (lamp) since last power up
    ), d/10, h/10, h%10, m/10, m%10, s/10, s%10);
  return bfill.position();
}

//Status reply for lamp on or off
static word status_io_reply(int output_io) {
  bfill = ether.tcpOffset();
  bfill.emit_p(PSTR(
    "HTTP/1.0 200 OK\r\n" //essential, otherwise homebridge will get no reply
    "Content-Type: text/html\r\n"
    "Pragma: no-cache\r\n"
    "\r\n"
    "$D" //return status on or off
    ), output_io);
  return bfill.position();
}

//Status reply for lamp brightness level
static word status_lvl_reply(int output_lvl) {
  bfill = ether.tcpOffset();
  bfill.emit_p(PSTR(
    "HTTP/1.0 200 OK\r\n" //essential, otherwise homebridge will get no reply
    "Content-Type: text/html\r\n"
    "Pragma: no-cache\r\n"
    "\r\n"
    "$D" //return status brightness level
    ), output_lvl);
  return bfill.position();
}

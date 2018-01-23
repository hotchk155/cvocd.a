#include <Wire.h>
#define P_LED 11
#define P_YELLOW 19
#define P_GREEN 20
#define P_RED 21

#define P_CAL_BUTTON 1
#define P_TEST_BUTTON 4

#define I2C_ADDR 0b1001100
void setup() {  
  
  pinMode(P_LED, OUTPUT);
  pinMode(P_YELLOW, OUTPUT);
  pinMode(P_GREEN, OUTPUT);
  pinMode(P_RED, OUTPUT);

  pinMode(P_CAL_BUTTON, INPUT_PULLUP);
  pinMode(P_TEST_BUTTON, INPUT_PULLUP);
  
  for(;;) {
    digitalWrite(P_YELLOW, digitalRead(P_CAL_BUTTON));
    digitalWrite(P_RED, digitalRead(P_TEST_BUTTON));
  }
  
  digitalWrite(P_YELLOW,HIGH);
  delay(1000);
  digitalWrite(P_YELLOW,LOW);
  digitalWrite(P_GREEN,HIGH);
  delay(1000);
  digitalWrite(P_GREEN,LOW);
  digitalWrite(P_RED,HIGH);
  delay(1000);
  digitalWrite(P_RED,LOW);
  
  Wire.begin();
  Serial.begin(9600);
  Serial.println("Begin");
  
  Serial1.begin(31250);
  
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(0x01);
  Wire.write(0b00011111);
  Wire.endTransmission();

  Wire.beginTransmission(I2C_ADDR);
  Wire.write(0x02);
  Wire.write(0b0);
  Wire.endTransmission();
  
}



int read_mv() {
  unsigned result = ((int)Wire.read() << 8) | Wire.read();
  int q = result & 0x3FFF;
  if(result & 0x4000) {
    return 0;
  }
  else {
    return (q * 2 * 0.30518);
  }
}
byte note = 24;


#define MIDI_CC_NRPN_HI 		99
#define MIDI_CC_NRPN_LO 		98
#define MIDI_CC_DATA_HI 		6
#define MIDI_CC_DATA_LO 		38

#define NRPNH_GLOBAL                    1
#define NRPNH_CV1		        21
#define NRPNH_CV2		        22
#define NRPNH_CV3		        23
#define NRPNH_CV4		        24

#define NRPNL_CAL_SCALE  	        98
#define NRPNL_CAL_OFS  		        99
#define NRPNL_SAVE                      100

int scale_adj[4];

void all_notes_off() {
  for(int i=0; i<128; ++i) {
    Serial1.write((byte)0x90);
    Serial1.write((byte)note);
    Serial1.write((byte)0x00);
    delay(2);
  }
}

// Fetch the voltage output for a note
boolean test_note(byte note, int mv[4]) {
  // Send the note
  Serial1.write((byte)0x90);
  Serial1.write((byte)note);
  Serial1.write((byte)0x7F);
  
  delay(200);
  
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(0x06);
  Wire.endTransmission(false);  
  Wire.requestFrom(I2C_ADDR,8,true);
  
  int q=1000;
  while(Wire.available() < 8 && q>0) {
    --q;
    delayMicroseconds(5);
  }
  mv[0] = read_mv();
  mv[1] = read_mv();
  mv[2] = read_mv();
  mv[3] = read_mv();

  Serial1.write((byte)0x90);
  Serial1.write((byte)note);
  Serial1.write((byte)0x00);
  
  return !!q;
}

void send_nrpn(byte ph, byte pl, byte dh, byte dl) {
  Serial1.write((byte)0xB0);
  Serial1.write((byte)MIDI_CC_NRPN_HI);
  Serial1.write((byte)ph);
  
  Serial1.write((byte)0xB0);
  Serial1.write((byte)MIDI_CC_NRPN_LO);
  Serial1.write((byte)pl);
  
  Serial1.write((byte)0xB0);
  Serial1.write((byte)MIDI_CC_DATA_HI);
  Serial1.write((byte)dh);

  Serial1.write((byte)0xB0);
  Serial1.write((byte)MIDI_CC_DATA_LO);
  Serial1.write((byte)dl);

  delay(5);  
}

void set_scale_adj(byte which, int amount) 
{
  if(amount > 63) amount = 63;
  if(amount < -63) amount = -63;
  send_nrpn(NRPNH_CV1 + which, NRPNL_CAL_SCALE, 0, amount + 64);
}
void set_ofs_adj(byte which, int amount) 
{
  if(amount > 63) amount = 63;
  if(amount < -63) amount = -63;
  send_nrpn(NRPNH_CV1 + which, NRPNL_CAL_OFS, 0, amount + 64);
}

#define OCTAVES 8
boolean calibrate_gain(byte which, int& scale, boolean& done) 
{
  int mv[4] = {0};
  int result = 0;
  int last_result = 0;
  int note = 36;
  int delta = 0;
  double  delta_total = 0;
  int i;
  Serial.print("=== CALIBRATE GAIN ");
  Serial.println(which);
  
  set_scale_adj(which,scale);
  set_ofs_adj(which,0);
  
  for(i=0; i<OCTAVES; ++i) {
    if(!test_note(note,mv)) {
      Serial.println("*** COMMS ERROR ***");
      return false;
    }
    Serial.print("C");
    Serial.print(i+1, DEC);
    Serial.print("->");
    Serial.print(mv[which], DEC);
    Serial.print("mV ");
    result = mv[which];
    if(i>0) {
      delta = result - last_result;
      delta_total += delta;
      Serial.print("[octave step ");
      Serial.print(delta);
      Serial.print("mV]");
      if(delta < 800 || delta > 1200) {
        Serial.println(" *** FAIL");
        return false;
      }
    }
    Serial.println("");
    last_result = result;
    note += 12;
  }
  delta_total = (delta_total/(i-1));
  Serial.print(" Gain error ");
  Serial.println(delta_total-1000);

  int gain_correction = 0.5 + 4.0 * (1000 - delta_total); 
  gain_correction = constrain(gain_correction, -63, 63);  
  if(abs(delta_total-1000) > 0.2) {
      Serial.print(" Gain correction ");
      Serial.print(gain_correction);
      set_scale_adj(which,gain_correction);
      Serial.println(" set");
      scale = gain_correction;
  }
  else {
    done = true;
  }
  return true;  
}


boolean calibrate_offset(byte which, int &ofs) 
{
  int mv[4] = {0};
  int note;
  int expected;
  int error;
  double error_total = 0;
  double min_error = 99999999;
  int this_ofs = 0;
  int i;
 
  
  Serial.print("=== CALIBRATE OFFSET ");
  Serial.println(which);  
  ofs = 0;
  // multiple attempts to find the offset value which works best
  for(int count = 0; count < 5; ++count) {
    set_ofs_adj(which,this_ofs);
    note = 36;
    expected = 1000;
    for(i=0; i<OCTAVES; ++i) {
      if(!test_note(note,mv)) {
        Serial.println("*** COMMS ERROR ***");
        return false;
      }      
      error = expected - mv[which];
      error_total += error;
      expected += 1000;      
      note += 12;
    }
  
    error_total = (error_total/i);
    // one unit is 2mV
    int ofs_correction = (0.5 + error_total/2.0);
    ofs_correction = ofs_correction + ofs; // remember there is already an offset 
    this_ofs = constrain(ofs_correction, -63, 63);  
    
    if(abs(error_total) < min_error) {
      // tracking best value
      min_error = error_total;
      ofs = this_ofs;
    }     
  }

  // store best value
  set_ofs_adj(which,ofs);
  Serial.print("Correction ");
  Serial.print(ofs);
  Serial.println(" set");
  
  // re-evaluate
  note = 36;
  expected = 1000;
  for(i=0; i<OCTAVES; ++i) {
    if(!test_note(note,mv)) {
      Serial.println("*** COMMS ERROR ***");
      return false;
    }
    
    error = expected - mv[which];
    error_total += error;
    expected += 1000;
    
    Serial.print("C");
    Serial.print(i+1, DEC);
    Serial.print("->");
    Serial.print(mv[which], DEC);
    Serial.print("mV");
    Serial.print(" expected ");
    Serial.print(expected);
    Serial.print("mV");
    Serial.print(" error ");
    Serial.print(error);
    Serial.println("mV");    
    note += 12;
  }
  error_total = (error_total/i);
  Serial.print("Mean offset error ");
  Serial.print(error_total);
  Serial.println("mV");
   
  return true;    
}

void loop() {

  int scale[4] = {0};
  int ofs[4] = {0};
  for(int i=0; i<4; ++i) {
    all_notes_off();
    delay(1000);
    boolean done = false;
    while(!done) {
      calibrate_gain(i, scale[i], done);
    }  
    calibrate_offset(i, ofs[i]);
  }
  Serial.println("Done...");
  Serial.print("scale ");
  for(int i=0; i<4; ++i) {
    Serial.print(scale[i]);
    Serial.print(" ");
  }
  Serial.println("");
  Serial.print("ofs ");
  for(int i=0; i<4; ++i) {
    Serial.print(ofs[i]);
    Serial.print(" ");
  }
  
  digitalWrite(P_LED, HIGH);
  delay(10);
  digitalWrite(P_LED, LOW);
  delay(4000);  
  for(;;);
}






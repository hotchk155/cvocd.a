#include <Wire.h>
#define P_LED 11
#define I2C_ADDR 0b1001100
void setup() {
  pinMode(P_LED, OUTPUT);
  Wire.begin();
  Serial.begin(9600);
  Serial.print("Begin");
  
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
boolean calibrate(byte which, int& scale, int &ofs, boolean& scale_done, boolean &ofs_done) 
{
  int mv[4] = {0};
  int result = 0;
  int last_result = 0;
  int note = 36;
  int expected = 1000;
  int error;
  double error_total = 0;
  int delta = 0;
  double  delta_total = 0;
  int i;
  Serial.print("=== CALIBRATE CV ");
  Serial.println(which);
  
  set_scale_adj(which,scale);
  set_ofs_adj(which,ofs);
  
  for(i=0; i<OCTAVES; ++i) {
    if(!test_note(note,mv)) {
      Serial.println("*** COMMS ERROR ***");
      return false;
    }
    Serial.print("C");
    Serial.print(i+1, DEC);
    Serial.print("->");
    Serial.print(mv[which], DEC);
    Serial.print("mV [");
    result = mv[which];
    if(i>0) {
      delta = result - last_result;
      delta_total += delta;
      Serial.print(delta);
      Serial.print("diff, ");
      if(delta < 800 || delta > 1200) {
        Serial.println(" *** FAIL");
        return false;
      }
    }
    error = result - expected;
    error_total += error;
    expected += 1000;
    Serial.print(error);
    Serial.print(" err]");
    last_result = result;
    Serial.println(", ");
    note += 12;
  }
  delta_total = (delta_total/(i-1));
  Serial.print("Gain error ");
  Serial.print(delta_total-1000);

  error_total = (error_total/i);
  Serial.print(", offet error ");
  Serial.println(error_total);
  
  int gain_correction = 0.5 + 4.0 * (1000 - delta_total); 
  gain_correction = constrain(gain_correction, -63, 63);
  //correction *= 8; // correction is applied over 8 octaves
//  correction /= 2; // 2 DAC units per mV

  int ofs_correction = 0.5 + error_total/2.0;
  ofs_correction = constrain(ofs_correction, -63, 63);
  
  if(!scale_done) {
    if(gain_correction != 0) {
      Serial.print("Gain correction ");
      Serial.print(gain_correction);
      set_scale_adj(which,gain_correction);
      Serial.println(" set");
      scale = gain_correction;
    }
    else {
      Serial.print("Offset correction ");
      Serial.print(ofs_correction);
      set_ofs_adj(which,ofs_correction);
      Serial.println(" set");
      ofs = ofs_correction;
      scale_done = true;
    }
  }
  else {
     return true;
  }
  return false;  
}


void loop() {

  int scale[4] = {0};
  int ofs[4] = {0};
  for(int i=0; i<4; ++i) {
    all_notes_off();
    delay(1000);
    boolean scale_done = false;
    boolean ofs_done = false;
    for(;;) {
      if(calibrate(i, scale[i], ofs[i], scale_done, ofs_done)) {
        break;
      }
    }  
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






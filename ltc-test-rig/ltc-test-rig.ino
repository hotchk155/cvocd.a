#include <Wire.h>
#define P_LED 11
#define P_YELLOW 19
#define P_GREEN 20
#define P_RED 21

#define P_CAL_BUTTON 1
#define P_TEST_BUTTON 4


#define GAIN_CAL_CYCLES 5
#define OCTAVES 8

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

#define I2C_ADDR 0b1001100
void setup() {  

  pinMode(P_LED, OUTPUT);
  pinMode(P_YELLOW, OUTPUT);
  pinMode(P_GREEN, OUTPUT);
  pinMode(P_RED, OUTPUT);

  pinMode(P_CAL_BUTTON, INPUT_PULLUP);
  pinMode(P_TEST_BUTTON, INPUT_PULLUP);


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

  digitalWrite(P_YELLOW,HIGH);
  digitalWrite(P_GREEN,HIGH);      
  digitalWrite(P_RED,HIGH);      

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
void all_notes_off() {
  for(int i=0; i<128; ++i) {
    Serial1.write((byte)0x90);
    Serial1.write((byte)i);
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
void save_calibration() 
{
  send_nrpn(NRPNH_GLOBAL, NRPNL_SAVE, 0, 0);
}

/*
boolean xxxcalibrate_gain(byte which, int& scale, boolean& done) 
{
  int mv[4] = {
    0  };
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
*/

/////////////////////////////////////////////////////////////////////////
// GAIN CALIBRATION
boolean calibrate_gain(byte which, int &gain) 
{

  int mv[4];
  int note;
  int result;
  int last_result;
  int delta;
  double delta_total;
  double min_error;
  int this_gain;
  int i;


  Serial.print("Calibrating gain on CV");
  Serial.println(which);  
  
  // start from zero
  gain = 0;
  this_gain = 0;
  min_error = 99999999;
  
  // multiple attempts to find the offset value which works best
  for(int count = 0; count < GAIN_CAL_CYCLES; ++count) {
    
    Serial.print("try ");
    Serial.println(this_gain);
    // set the offset in CVOCD
    set_scale_adj(which,this_gain);
    set_ofs_adj(which,0);
    
    // prepare for scan through octaves
    note = 36;
    delta_total = 0;
    delta = 0;
    for(i=0; i<OCTAVES; ++i) {
      if(!test_note(note,mv)) {
        Serial.println("*** COMMS ERROR ***");
      }
      result = mv[which];
      if(i>0) {
        // get difference between this and the last result
        delta = result - last_result;
        if(delta < 800 || delta > 1200) {
          Serial.print(" *** FAIL - OUT OF TOLERANCE ");
          Serial.println(delta);
          return false;
        }
        // subtract the expected 1000mV between octaves so that we get the 
        // deviation from the expected difference
        delta -= 1000.0;
        
        // total up all the deviations from expected
        delta_total += delta;
      }
      
      // store last result
      last_result = result;
      
      // ready for the next octave
      note += 12;
      
      Serial.print("C");
      Serial.print(i+1, DEC);
      Serial.print("->");
      Serial.print(result);
      Serial.print(" diff ");
      Serial.println(delta);
      //Serial.print(".");      
    }
    
    // get the mean deviation from 1V/octave across all octaves
    delta_total = (delta_total/(i-1));
    Serial.print("mean ");
    Serial.println(delta_total);
    
    // is this the best one yet?
    if(abs(delta_total) < min_error) {
      min_error = abs(delta_total);
      
      // remember the gain value that 
      gain = this_gain;
      
      Serial.println("STORED");
    }     

    int gain_correction = 0.5 - 4.0 * delta_total; 
    this_gain = constrain(gain_correction, -63, 63);  
  }

  // store best value
  set_scale_adj(which,gain);
  Serial.println("");
  Serial.print("GAIN correction=");
  Serial.println(gain);
}



/////////////////////////////////////////////////////////////////////////
#define OFFSET_CAL_CYCLES 5
boolean calibrate_offset(byte which, int &ofs) 
{
  int mv[4];
  int note;
  int expected;
  int error;
  double error_total;
  double min_error;
  int this_ofs;
  int i;


  Serial.print("Calibrating offset on CV");
  Serial.print(which);  
  ofs = 0;
  this_ofs = 0;
  min_error = 99999999;
  // multiple attempts to find the offset value which works best
  for(int count = 0; count < OFFSET_CAL_CYCLES; ++count) {
    set_ofs_adj(which,this_ofs);
    note = 36;
    expected = 1000;
    error_total = 0;
    for(i=0; i<OCTAVES; ++i) {
      if(!test_note(note,mv)) {
        Serial.println("*** COMMS ERROR ***");
        return false;
      }      
      error = expected - mv[which];
      error_total += error;
      expected += 1000;      
      Serial.print(".");
      note += 12;
    }

    error_total = (error_total/i);
    // one unit is 2mV
    int ofs_correction = (0.5 + error_total/2.0);
    ofs_correction = ofs_correction + ofs; // remember there is already an offset 
    this_ofs = constrain(ofs_correction, -63, 63);  

    if(abs(error_total) < min_error) {
      // tracking best value
      min_error = abs(error_total);
      ofs = this_ofs;
    }     
  }

  // store best value
  set_ofs_adj(which,ofs);
  Serial.println("");
  Serial.print("OFFSET correction = ");
  Serial.println(ofs);
  return true;    
}

void octave_check(byte which) 
{  
  // re-evaluate
  int note = 36;
  int error;
  int i;
  double error_total = 0;
  int expected = 1000;
  int mv[4] = {   0  };
  
  Serial.print("Calibration check CV");
  Serial.println(which);  

  for(i=0; i<OCTAVES; ++i) {
    if(!test_note(note,mv)) {
      Serial.println("*** COMMS ERROR ***");
    }

    error = mv[which] - expected;
    error_total += error;

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
    expected += 1000;
    note += 12;
  }
}

boolean calibrate() {

  int scale[4] = {
    0  };
  int ofs[4] = {
    0  };
  for(int i=0; i<4; ++i) {
    all_notes_off();
    delay(1000);
    octave_check(i);    
    if(!calibrate_gain(i, scale[i])) {
        return false;
     }
    if(!calibrate_offset(i, ofs[i])) {
      return false;
    }
    octave_check(i);    
  }
  save_calibration();
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
}


void scale_check() 
{  
  // re-evaluate
  int note = 36;
  float expected = 1000;
  int mv[4] = {   0  };
  
  Serial.println("Calibration check");
  all_notes_off();
  delay(1000);
 
  while(note <= 120) {
    if(!test_note(note,mv)) {
      Serial.println("*** COMMS ERROR ***");
    }
    int e = (int)(0.5+expected);
    Serial.print(note);
    Serial.print("|");
    Serial.print(e);
    Serial.print("|");
    Serial.print(mv[0]);
    Serial.print("|");
    Serial.print(mv[1]);
    Serial.print("|");
    Serial.print(mv[2]);
    Serial.print("|");
    Serial.print(mv[3]);
    Serial.print("|");
    Serial.print(mv[0]-e);
    Serial.print("|");
    Serial.print(mv[1]-e);
    Serial.print("|");
    Serial.print(mv[2]-e);
    Serial.print("|");
    Serial.print(mv[3]-e);
    Serial.println("");    
    expected += 1000.0/12.0;
    note ++;
  }
}

void loop() {
  if(!digitalRead(P_CAL_BUTTON)) {
    digitalWrite(P_YELLOW,HIGH);
    digitalWrite(P_GREEN,LOW);
    digitalWrite(P_RED,LOW);
    if(calibrate()) {
      digitalWrite(P_YELLOW,LOW);
      digitalWrite(P_GREEN,HIGH);      
    }
    else {
      digitalWrite(P_YELLOW,LOW);
      digitalWrite(P_RED,HIGH);      
    }
  }   
  if(!digitalRead(P_TEST_BUTTON)) {
    digitalWrite(P_YELLOW,HIGH);
    digitalWrite(P_GREEN,LOW);
    digitalWrite(P_RED,LOW);
    scale_check();
    digitalWrite(P_YELLOW,LOW);
    digitalWrite(P_GREEN,HIGH);      
  }   
}







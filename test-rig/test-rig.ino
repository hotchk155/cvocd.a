//////////////////////////////////////////////////////////////
//
// CVOCD / MUTANT BRIAN CALIBRATION RIG
//
// FIRMWARE V3
//
//////////////////////////////////////////////////////////////


#include <SPI.h>
#include <EEPROM.h>
#define P_SS        0
#define P_NEOC      4
#define ADC_CONFIG  0b00000110

// Pin definitions
#define P_LEDR1         21
#define P_LEDR2         19          
#define P_LEDR3         17
#define P_LEDB1         20          
#define P_LEDB2         18
#define P_LEDB3         16         
#define P_BUTTON1       22
#define P_BUTTON2       23
#define P_BUTTON3       11

// MIDI message numbers
#define MIDI_CC_NRPN_HI 		99
#define MIDI_CC_NRPN_LO 		98
#define MIDI_CC_DATA_HI 		6
#define MIDI_CC_DATA_LO 		38

// NRPNs used for configuration
#define NRPNH_GLOBAL        1
#define NRPNH_CV1		        21
#define NRPNH_CV2		        22
#define NRPNH_CV3		        23
#define NRPNH_CV4		        24
#define NRPNL_CAL_SCALE  	  98
#define NRPNL_CAL_OFS  		  99
#define NRPNL_SAVE          100

#define GAIN_CAL_CYCLES     5
#define OCTAVES             8

#define ZERO_VOLTS_THRESHOLD 100
//////////////////////////////////////////////////////////////////////////
// SYSEX PATCH MAPS OUTPUTS 1,2,3,4 TO RESPECTIVE MIDI CHANNEL
byte calibration_patch[] = {
0xF0, 0x00, 0x7F, 0x15, 0x01, 0x02, 0x00, 0x01, 0x01, 0x0C, 0x01, 0x0F, 0x0B, 0x01, 0x01, 0x00, 
0x0B, 0x02, 0x00, 0x01, 0x0B, 0x03, 0x00, 0x00, 0x0B, 0x04, 0x00, 0x7F, 0x0B, 0x05, 0x00, 0x01, 
0x0B, 0x07, 0x00, 0x03, 0x0B, 0x08, 0x00, 0x03, 0x0C, 0x01, 0x01, 0x00, 0x0C, 0x02, 0x00, 0x02, 
0x0C, 0x03, 0x00, 0x00, 0x0C, 0x04, 0x00, 0x7F, 0x0C, 0x05, 0x00, 0x01, 0x0C, 0x07, 0x00, 0x03, 
0x0C, 0x08, 0x00, 0x03, 0x0D, 0x01, 0x01, 0x00, 0x0D, 0x02, 0x00, 0x03, 0x0D, 0x03, 0x00, 0x00, 
0x0D, 0x04, 0x00, 0x7F, 0x0D, 0x05, 0x00, 0x01, 0x0D, 0x07, 0x00, 0x03, 0x0D, 0x08, 0x00, 0x03, 
0x0E, 0x01, 0x01, 0x00, 0x0E, 0x02, 0x00, 0x04, 0x0E, 0x03, 0x00, 0x00, 0x0E, 0x04, 0x00, 0x7F, 
0x0E, 0x05, 0x00, 0x01, 0x0E, 0x07, 0x00, 0x03, 0x0E, 0x08, 0x00, 0x03, 0x15, 0x01, 0x0B, 0x01, 
0x15, 0x10, 0x00, 0x00, 0x15, 0x0E, 0x00, 0x40, 0x16, 0x01, 0x0C, 0x01, 0x16, 0x10, 0x00, 0x00, 
0x16, 0x0E, 0x00, 0x40, 0x17, 0x01, 0x0D, 0x01, 0x17, 0x10, 0x00, 0x00, 0x17, 0x0E, 0x00, 0x40, 
0x18, 0x01, 0x0E, 0x01, 0x18, 0x10, 0x00, 0x00, 0x18, 0x0E, 0x00, 0x40, 0x1F, 0x01, 0x00, 0x00, 
0x1F, 0x0C, 0x01, 0x00, 0x20, 0x01, 0x00, 0x00, 0x20, 0x0C, 0x01, 0x00, 0x21, 0x01, 0x00, 0x00, 
0x21, 0x0C, 0x01, 0x00, 0x22, 0x01, 0x00, 0x00, 0x22, 0x0C, 0x01, 0x00, 0x23, 0x01, 0x00, 0x00, 
0x23, 0x0C, 0x01, 0x00, 0x24, 0x01, 0x00, 0x00, 0x24, 0x0C, 0x01, 0x00, 0x25, 0x01, 0x00, 0x00, 
0x25, 0x0C, 0x01, 0x00, 0x26, 0x01, 0x00, 0x00, 0x26, 0x0C, 0x01, 0x00, 0x27, 0x01, 0x00, 0x00, 
0x27, 0x0C, 0x01, 0x00, 0x28, 0x01, 0x00, 0x00, 0x28, 0x0C, 0x01, 0x00, 0x29, 0x01, 0x00, 0x00, 
0x29, 0x0C, 0x01, 0x00, 0x2A, 0x01, 0x00, 0x00, 0x2A, 0x0C, 0x01, 0x00, 0xF7
};

//////////////////////////////////////////////////////////////////////////
// SYSEX PATCH FOR CALIBRATION TEST WHICH OUTPUTS SAME CV ON EACH OUTPUT
byte test_patch[] = {
0xF0, 0x00, 0x7F, 0x15, 0x01, 0x02, 0x00, 0x01, 0x01, 0x0C, 0x01, 0x0F, 0x0B, 0x01, 0x01, 0x00, 0x0B, 0x02, 
0x00, 0x01, 0x0B, 0x03, 0x00, 0x00, 0x0B, 0x04, 0x00, 0x7F, 0x0B, 0x05, 0x00, 0x01, 0x0B, 0x07, 0x00, 0x03, 
0x0B, 0x08, 0x00, 0x03, 0x0C, 0x01, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x0C, 0x03, 0x00, 0x00, 0x0C, 0x04,
0x00, 0x00, 0x0C, 0x05, 0x00, 0x00, 0x0C, 0x07, 0x00, 0x00, 0x0C, 0x08, 0x00, 0x00, 0x0D, 0x01, 0x00, 0x00, 
0x0D, 0x02, 0x00, 0x00, 0x0D, 0x03, 0x00, 0x00, 0x0D, 0x04, 0x00, 0x00, 0x0D, 0x05, 0x00, 0x00, 0x0D, 0x07, 
0x00, 0x00, 0x0D, 0x08, 0x00, 0x00, 0x0E, 0x01, 0x00, 0x00, 0x0E, 0x02, 0x00, 0x00, 0x0E, 0x03, 0x00, 0x00, 
0x0E, 0x04, 0x00, 0x00, 0x0E, 0x05, 0x00, 0x00, 0x0E, 0x07, 0x00, 0x00, 0x0E, 0x08, 0x00, 0x00, 0x15, 0x01, 
0x0B, 0x01, 0x15, 0x10, 0x00, 0x00, 0x15, 0x0E, 0x00, 0x40, 0x16, 0x01, 0x0B, 0x01, 0x16, 0x10, 0x00, 0x00, 
0x16, 0x0E, 0x00, 0x40, 0x17, 0x01, 0x0B, 0x01, 0x17, 0x10, 0x00, 0x00, 0x17, 0x0E, 0x00, 0x40, 0x18, 0x01, 
0x0B, 0x01, 0x18, 0x10, 0x00, 0x00, 0x18, 0x0E, 0x00, 0x40, 0x1F, 0x01, 0x00, 0x00, 0x1F, 0x0C, 0x01, 0x00, 
0x20, 0x01, 0x00, 0x00, 0x20, 0x0C, 0x01, 0x00, 0x21, 0x01, 0x00, 0x00, 0x21, 0x0C, 0x01, 0x00, 0x22, 0x01, 
0x00, 0x00, 0x22, 0x0C, 0x01, 0x00, 0x23, 0x01, 0x00, 0x00, 0x23, 0x0C, 0x01, 0x00, 0x24, 0x01, 0x00, 0x00, 
0x24, 0x0C, 0x01, 0x00, 0x25, 0x01, 0x00, 0x00, 0x25, 0x0C, 0x01, 0x00, 0x26, 0x01, 0x00, 0x00, 0x26, 0x0C, 
0x01, 0x00, 0x27, 0x01, 0x00, 0x00, 0x27, 0x0C, 0x01, 0x00, 0x28, 0x01, 0x00, 0x00, 0x28, 0x0C, 0x01, 0x00, 
0x29, 0x01, 0x00, 0x00, 0x29, 0x0C, 0x01, 0x00, 0x2A, 0x01, 0x00, 0x00, 0x2A, 0x0C, 0x01, 0x00, 0xF7 
};

#define EEPROM_COOKIE_ADDR 9
#define EEPROM_COOKIE_VALUE 123
#define ADC_COMP_ADDR 10
int ADC_COMP[4];

void write_adc_comp() {
  int addr = ADC_COMP_ADDR;
  for(int i = 0; i<4; ++i) {
    EEPROM.write(addr++, (ADC_COMP[i]&0xFF00)>>8);
    EEPROM.write(addr++, ADC_COMP[i]&0x00FF);
  }
  EEPROM.write(EEPROM_COOKIE_ADDR, EEPROM_COOKIE_VALUE);
}
void read_adc_comp() {
  if(EEPROM.read(EEPROM_COOKIE_ADDR) != EEPROM_COOKIE_VALUE) {
    memset(ADC_COMP,0,sizeof ADC_COMP);
    write_adc_comp();
  }
  else {
    int addr = ADC_COMP_ADDR;
    for(int i = 0; i<4; ++i) {
      ADC_COMP[i] = ((int)EEPROM.read(addr++))<<8;
      ADC_COMP[i] |= (int)EEPROM.read(addr++);
    }    
  }
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// ADC INTERFACE FOR THE MAX1167 ADC
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// Setup the ADC
void adc_setup() {
  pinMode(P_SS, OUTPUT);
  pinMode(P_NEOC, INPUT_PULLUP);  
  SPI.begin();
  digitalWrite(P_SS, LOW);
  SPI.transfer(ADC_CONFIG); 
  digitalWrite(P_SS, HIGH);
  delay(10);   
}

// fetch just a single ADC channel reading
// value in micro volts
unsigned int adc_read(unsigned int chan) {
  //digitalWrite(P_SS, LOW);
  //SPI.transfer(((3-chan)<<5) | ADC_CONFIG);
  //digitalWrite(P_SS, HIGH);
  
  //delay(1);
  SPI.beginTransaction(SPISettings(1200000UL, MSBFIRST, SPI_MODE0));
  digitalWrite(P_SS, LOW);
  SPI.transfer(((3-chan)<<5) | ADC_CONFIG);
//  while(digitalRead(P_NEOC));
  delayMicroseconds(10);
  byte b1 =  SPI.transfer(0);
  byte b0 =  SPI.transfer(0);
  digitalWrite(P_SS, HIGH);  
  SPI.endTransaction();
  long result = ((unsigned)b1)<<8 | b0; // DAC units, 16 bits, 8192 mV range  
  result += ADC_COMP[chan];
  if(result < 0) result = 0;
  return (unsigned int)result;
}

#define ITERATIONS 1
  //long sum = (4+adc_read(chan))>>3;
unsigned int mv_read(unsigned int chan) {
  long sum = adc_read(chan);  
  for(int i=1; i<ITERATIONS; ++i) {
    sum += adc_read(chan);
  }
  return (4+(sum/ITERATIONS))>>3;
}
    

//////////////////////////////////////////////////////////////////////////





//////////////////////////////////////////////////////////////////////////
// SEND A SYSEX BLOCK
void send_sysex(byte *d, int len) {
  while(len--) {
    Serial1.write((byte)*d);    
    ++d;
    delay(1);
  }
}

//////////////////////////////////////////////////////////////////////////
// SEND NRPN MESSAGES
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


//////////////////////////////////////////////////////////////////////////
// TURN ALL NOTES OFF ON MIDI CHANNEL 0
void all_notes_off() {

  /*
  for(int i=0; i<128; ++i) {
    Serial1.write((byte)0x90);
    Serial1.write((byte)i);
    Serial1.write((byte)0x00);
    delay(2);
  }*/

  for(int i=0; i<4; ++i) {
    Serial1.write((byte)0x90|i);
    Serial1.write((byte)0);
    Serial1.write((byte)0x7f);
    delay(2);
    Serial1.write((byte)0x90|i);
    Serial1.write((byte)0);
    Serial1.write((byte)0x00);
    delay(2);
  }
  
  
}

//////////////////////////////////////////////////////////////////////////
// SEND NRPN TO SET GAIN
void set_scale_adj(byte which, int amount) 
{
  if(amount > 63) amount = 63;
  if(amount < -63) amount = -63;
  send_nrpn(NRPNH_CV1 + which, NRPNL_CAL_SCALE, 0, amount + 64);
}

//////////////////////////////////////////////////////////////////////////
// SEND NRPN TO SET OFFSET
void set_ofs_adj(byte which, int amount) 
{
  if(amount > 63) amount = 63;
  if(amount < -63) amount = -63;
  send_nrpn(NRPNH_CV1 + which, NRPNL_CAL_OFS, 0, amount + 64);
}

//////////////////////////////////////////////////////////////////////////
// SEND NRPN TO COMMIT NEW VALUES
void save_calibration() 
{
  send_nrpn(NRPNH_GLOBAL, NRPNL_SAVE, 0, 0);
}


//////////////////////////////////////////////////////////////////////////
// SEND MIDI NOTE THEN READ ALL CV OUTPUTS
void test_note(byte chan, byte note, unsigned int *mv) {
  
  // Send the note
  Serial1.write((byte)0x90|chan);
  Serial1.write((byte)note);
  Serial1.write((byte)0x7F);

  // delay to allow outputs to settle
  delay(100);

  // read the outputs. round to nearest whole mV
  mv[0] = mv_read(0);
  mv[1] = mv_read(1);
  mv[2] = mv_read(2);
  mv[3] = mv_read(3);

  // Turn the note off
  Serial1.write((byte)0x90|chan);
  Serial1.write((byte)note);
  Serial1.write((byte)0x00);

}

/*
//////////////////////////////////////////////////////////////////////////
// SANITY CHECK ENSURES ALL CV LEADS CONNECTED IN RIGHT ORDER
boolean sanity_check() {
  unsigned int mv[4];
  send_sysex(sanity_patch, sizeof sanity_patch);
  delay(2000);
  for(int i=0; i<4; ++i) {
    test_note(i, 100, mv);
    Serial.print("Sanity check output ");                 
    Serial.print(i, DEC);                 
    Serial.print(": ");                 
    Serial.print(mv[0], DEC);
    Serial.print(", ");                 
    Serial.print(mv[1], DEC);
    Serial.print(", ");                 
    Serial.print(mv[2], DEC);
    Serial.print(", ");                 
    Serial.println(mv[3], DEC);
    
    for(int j=0; j<4; ++j) {
      if(i==j) {
        if(mv[j] < 100) {
          Serial.print("**** output should be HIGH:");                 
          Serial.println(j, DEC);                 
          return false;
        }
      }
      else {
        if(mv[j] > 100) {
          Serial.print("**** output should be LOW:");                 
          Serial.println(j, DEC);                 
          return false;          
        }
      }
    }
    test_note(i, 0, mv);
  }
  Serial.println("Sanity check OK");                 
  return true;
}
*/

/////////////////////////////////////////////////////////////////////////
// GAIN CALIBRATION
boolean calibrate_gain(byte which, int &gain) 
{

  unsigned int mv[4];
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
    
    // prepare for scan through octaves
    note = 36;
    delta_total = 0;
    delta = 0;
    for(i=0; i<OCTAVES; ++i) {
      test_note(which,note,mv);
      for(int j=0; j<4; ++j) {
        if(j==which) {
          continue;
        }
        if(mv[j] > ZERO_VOLTS_THRESHOLD) {
          Serial.print("*** VOLTAGE ERROR *** Channel ");
          Serial.print(j);
          Serial.print(" has unexpected non-zero output of ");
          Serial.print(mv[j]);
          Serial.println(" mV - check cables are connected correctly ");
          return false;
        }
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


//////////////////////////////////////////////////////////////////////////
// OFFSET CALIBRATION
#define OFFSET_CAL_CYCLES 5
boolean calibrate_offset(byte which, int &ofs) 
{
  unsigned int mv[4];
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
      test_note(which,note,mv);
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

/*
//////////////////////////////////////////////////////////////////////////
// OCTAVE CHECK
void octave_check(byte which) 
{  
  // re-evaluate
  int note = 36;
  int error;
  int i;
  double error_total = 0;
  int expected = 1000;
  unsigned int mv[4] = {   0  };
  
  Serial.print("Calibration check CV");
  Serial.println(which);  

  for(i=0; i<OCTAVES; ++i) {
    if(!test_note(0,note,mv)) {
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
*/

//////////////////////////////////////////////////////////////////////////
// CALIBRATION
boolean calibrate() {

  int scale[4] = {0};
  int ofs[4] = {0};
  for(int i=0; i<4; ++i) {
    all_notes_off();
    delay(1000);
    //octave_check(i);    
    if(!calibrate_gain(i, scale[i])) {
        return false;
     }
    if(!calibrate_offset(i, ofs[i])) {
      return false;
    }
    //octave_check(i);    
  }
  save_calibration();
  Serial.println("Done...");
  Serial.print("scale");
  for(int i=0; i<4; ++i) {
    Serial.print(":");
    Serial.print(scale[i]);
  }
  Serial.print("; ofs");
  for(int i=0; i<4; ++i) {
    Serial.print(":");
    Serial.print(ofs[i]);
  }
  Serial.println(";");
}

void pad(int val, int pad, boolean sign) {
  int a = fabs(val);
  if(a>=1000) pad -= 4;
  else if(a>=100) pad -= 3;
  else if(a>=10) pad -= 2;
  else --pad;
  if(sign) --pad;
  while(pad-->0) {
    Serial.print(" ");
  }
  if(sign && val>=0) {
    Serial.print("+");    
  }
  Serial.print(val, DEC);
}

//////////////////////////////////////////////////////////////////////////
// MEASURE CV FOR EACH NOTE
void scale_check() 
{  
  // re-evaluate
  int note = 36;
  float expected = 1000;
  unsigned int mv[4] = {   0  };
  
  //Serial.println("Calibration check");
  //all_notes_off();
  //delay(1000);
 
  while(note <= 120) {
    test_note(0,note,mv);
    int e = (int)(0.5+expected);
    pad(note,3,false);
    Serial.print(" | ");
    pad(e,4,false);
    Serial.print(" | ");
    pad(mv[0],4,false);
    Serial.print(" | ");
    pad(mv[1],4,false);
    Serial.print(" | ");
    pad(mv[2],4,false);
    Serial.print(" | ");
    pad(mv[3],4,false);
    Serial.print(" | ");
    pad(((int)mv[0])-e,4,true);
    Serial.print(" | ");
    pad(((int)mv[1])-e,4,true);
    Serial.print(" | ");
    pad(((int)mv[2])-e,4,true);
    Serial.print(" | ");
    pad(((int)mv[3])-e,4,true);
    Serial.println("");    
    expected += 1000.0/12.0;
    note ++;
  }
}

void adc_test() {
  int adc_test_channel = 0;
  double smoothed = -1;
  
  for(;;) {
    unsigned int result = adc_read(adc_test_channel);
    if(smoothed < 0) {
      smoothed = result;
    }
    else {
      smoothed = 0.9 * smoothed + 0.1 * result;
    }
    unsigned int value = (int)(0.5 + smoothed);    
    int error = result - value;
    int warning = 0;
    if(error < -16) {error = -16; warning = 1;}
    if(error > 16) {error = 16; warning = 1;}
    Serial.print(adc_test_channel);
    Serial.print(") ");
    if(ADC_COMP[adc_test_channel] > 0) Serial.print("+");
    Serial.print(ADC_COMP[adc_test_channel]);
    Serial.print(" ");
    Serial.print(value, DEC);
    Serial.print(" [");    
    for(int i=-16; i<=16; ++i) {
      Serial.print((i==error)? "*": (i==0)? "+" : ".");
    }
    Serial.print("] ");
    Serial.print(((long)(10.0*(smoothed/8.0))), DEC);
    Serial.print(" mV x10 ");
    
    if(warning) {
      Serial.print("***");
      Serial.print(result, DEC);
      Serial.print(" = ");
      Serial.print(((long)(10.0*(result/8.0))), DEC);
      Serial.print("mV x 10 ***");
    }
    Serial.println();
    delay(5);

    if(!digitalRead(P_BUTTON1)) {
      if(++adc_test_channel>3) {
        adc_test_channel = 0;
      }
      Serial.print(">>> Change to ADC channel ");
      Serial.println(adc_test_channel);
      delay(500);
      smoothed = -1;      
    }    

    if(!digitalRead(P_BUTTON2)) {
      ++ADC_COMP[adc_test_channel];
      write_adc_comp();      
      Serial.print(">>> Increment ADC compensation for channel ");
      Serial.println(adc_test_channel);
      delay(500);
    }

    if(!digitalRead(P_BUTTON3)) {
      --ADC_COMP[adc_test_channel];
      write_adc_comp();      
      Serial.print(">>> Decrement ADC compensation for channel ");
      Serial.println(adc_test_channel);
      delay(500);
    }
    
  }
}
//////////////////////////////////////////////////////////////////////////
// SETUP
void setup() {  

  // pin modes
  pinMode(P_LEDR1, OUTPUT);
  pinMode(P_LEDR2, OUTPUT);
  pinMode(P_LEDR3, OUTPUT);
  pinMode(P_LEDB1, OUTPUT);
  pinMode(P_LEDB2, OUTPUT);
  pinMode(P_LEDB3, OUTPUT);
  pinMode(P_BUTTON1, INPUT_PULLUP);
  pinMode(P_BUTTON2, INPUT_PULLUP);
  pinMode(P_BUTTON3, INPUT_PULLUP);

  
  // USB Serial 
  Serial.begin(9600);
  Serial.println("Begin");  

  // Hard serial (MIDI)
  Serial1.begin(31250);

  digitalWrite(P_LEDR1, HIGH); delay(100); digitalWrite(P_LEDR1, LOW); 
  digitalWrite(P_LEDR2, HIGH); delay(100); digitalWrite(P_LEDR2, LOW); 
  digitalWrite(P_LEDR3, HIGH); delay(100); digitalWrite(P_LEDR3, LOW); 

  // ADC
  adc_setup(); 

  digitalWrite(P_LEDB1, HIGH); delay(100); digitalWrite(P_LEDB1, LOW); 
  digitalWrite(P_LEDB2, HIGH); delay(100); digitalWrite(P_LEDB2, LOW); 
  digitalWrite(P_LEDB3, HIGH); delay(100); digitalWrite(P_LEDB3, LOW); 

  read_adc_comp();
  
  //if(!digitalRead(P_BUTTON1)) {
    adc_test();  
  //}
}

//////////////////////////////////////////////////////////////////////////
// MAIN LOOP
void loop() {

  if(!digitalRead(P_BUTTON1)) {
      Serial.println("--- CALIBRATE ---");
      //digitalWrite(P_YELLOW,HIGH);
      //digitalWrite(P_GREEN,HIGH);
      //digitalWrite(P_RED,HIGH);
      send_sysex(calibration_patch, sizeof calibration_patch);
      delay(2000);
      calibrate();
      //digitalWrite(P_GREEN,LOW);
      //digitalWrite(P_RED,LOW);
      //if(calibrate()) {
        //delay(1000);
        //if(!sanity_check()) {
          //digitalWrite(P_YELLOW,LOW);
          //digitalWrite(P_RED,HIGH);      
        //}
        //else {
          //digitalWrite(P_YELLOW,LOW);
          //digitalWrite(P_GREEN,HIGH);      
        //}
      //}
      //else {
        //digitalWrite(P_YELLOW,LOW);
        //digitalWrite(P_RED,HIGH);      
      //}
  }   
  if(!digitalRead(P_BUTTON2)) {
//    digitalWrite(P_YELLOW,HIGH);
//    digitalWrite(P_GREEN,LOW);
//    digitalWrite(P_RED,LOW);
    Serial.println("--- TEST ---");
    send_sysex(test_patch, sizeof test_patch);
    delay(2000);
    scale_check();
//    digitalWrite(P_YELLOW,LOW);
//    digitalWrite(P_GREEN,HIGH);      
  } 
}




#include <Wire.h>
#define P_LED 11
#define I2C_ADDR 0b1001100
void setup() {
  pinMode(P_LED, OUTPUT);
  Wire.begin();
  Serial.begin(9600);
  
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
void loop() {

  Serial1.write((byte)0x90);
  Serial1.write((byte)note);
  Serial1.write((byte)0x7F);
  
  digitalWrite(P_LED, HIGH);
  delay(10);
  digitalWrite(P_LED, LOW);
  delay(300);  
  
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(0x06);
  Wire.endTransmission(false);  
  Wire.requestFrom(I2C_ADDR,8,true);
  
  int q=1000;
  while(Wire.available() < 8 && q>0) {
    --q;
    delayMicroseconds(5);
  }
  int v1 = read_mv();
  int v2 = read_mv();
  int v3 = read_mv();
  int v4 = read_mv();
  
  if(q) {
    Serial.print((int)note);
    Serial.print("|");
    Serial.print(v1);
    Serial.print("|");
    Serial.print(v2);
    Serial.print("|");
    Serial.print(v3);
    Serial.print("|");
    Serial.println(v4);
    
    Serial1.write((byte)0x90);
    Serial1.write((byte)note);
    Serial1.write((byte)0x00);
    if(++note >= 127) note = 24;    
  }      
}

#include <SPI.h>
#include <SD.h>
//#include <Wire.h>
#include <SoftwareSerial.h>
//#include <SparkFunTMP102.h>

#define RADIOENPIN 4
#define RADIOPIN 6
 
#include <string.h>

char datastring[46];

char c;
bool newData;
char messageType[6];
byte data_number;
byte data_offset;
char gpsTime[] = "000000";
char gpsLat[] = "0000000000";
char gpsHemLat = 'X';
char gpsLon[] = "00000000000";
char gpsHemLon = 'X';
char gpsAlt[] = "0000000";

SoftwareSerial ss(9, 8);
//TMP102 sensor0;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }
  ss.begin(9600);
  Serial.print("Initializing SD card...");
  if (!SD.begin(10)) { //chipSelect = 10
    Serial.println("Card failed, or not present");
    while (1);
  }
  Serial.println("card initialized.");
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
    if (dataFile) {
      dataFile.println("UTC;LAT;LON;ALT;TMP");
      dataFile.close();
    }
    else {
      Serial.println("error opening datalog.txt");
    }

  delay(100);
  /*Wire.begin(); //Join I2C Bus

  if(!sensor0.begin())
  {
    Serial.println("Cannot connect to TMP102.");
    Serial.println("Is the board connected? Is the device ID correct?");
    while(1);
  }
  Serial.println("Connected to TMP102!");
  */

  /* SETUP RTTY */
  pinMode(RADIOENPIN, OUTPUT); // EN
  pinMode(RADIOPIN, OUTPUT); // TX
  digitalWrite(RADIOENPIN, HIGH); // enable MTX2
  /* END SETUP RTTY */
  
  delay(100);
}

int gpsstrcmp(const char *str1, const char *str2) {
  str1 += 2;
  while (*str1 && *str1 == *str2)
    ++str1, ++str2;
  return *str1;
}

bool gps_encode(char chr) {
  bool found_data = false;
  switch(chr) {
    case '$':
      data_number = 0;
      data_offset = 0;
      return false;
    case '\r':
      return false;
    case '\n':
      return false;
    case '*':
      if (!gpsstrcmp(messageType, "GGA")) {
        return true;
      } else {
        return false;
      }
    case ',':
      data_number++;
      data_offset = 0;
      return false;
  }

  if ((data_number == 0) && (data_offset < 5)) {
      messageType[data_offset] = chr;
      data_offset++;
  }

  if ((data_number == 1) && (data_offset < 6) && (!gpsstrcmp(messageType, "GGA"))) {
      gpsTime[data_offset] = chr;
      data_offset++;
  }

  if ((data_number == 2) && (data_offset < 10) && (!gpsstrcmp(messageType, "GGA"))) {
      gpsLat[data_offset] = chr;
      data_offset++;
  }

  if ((data_number == 3) && (data_offset < 1) && (!gpsstrcmp(messageType, "GGA"))) {
      gpsHemLat = chr;
      data_offset++;
  }

  if ((data_number == 4) && (data_offset < 11) && (!gpsstrcmp(messageType, "GGA"))) {
      gpsLon[data_offset] = chr;
      data_offset++;
  }

  if ((data_number == 5) && (data_offset < 1) && (!gpsstrcmp(messageType, "GGA"))) {
      gpsHemLon = chr;
      data_offset++;
  }

  if ((data_number == 9) && (data_offset < 7) && (!gpsstrcmp(messageType, "GGA"))) {
      gpsAlt[data_offset] = chr;
      data_offset++;
  }

  return found_data;
}

void loop() {
 ss.begin(9600);
//____________________________GPS__________________________________________GPS
  for (unsigned long start = millis(); millis() - start < 1000;) {
    while (ss.available()) {
      c = ss.read();
      //Serial.write(c); // uncomment this line if you want to see the GPS data flowing
      if (gps_encode(c)) // Did a new valid sentence come in?
        newData = true;
    }
  }

//____________________________TMP__________________________________________TMP
  //sensor0.wakeup();
  //temperature = sensor0.readTempC();
  //sensor0.sleep();

  if (newData) {
    Serial.println(gpsTime);
    Serial.println(gpsLat);
    Serial.println(gpsHemLat);
    Serial.println(gpsLon);
    Serial.println(gpsHemLon);
    Serial.println(gpsAlt);
   //Serial.println(temperature);
  }

//____________________________SD___________________________________________SD
  if (newData) {
    File dataFile = SD.open("datalog.txt", FILE_WRITE);
    if (dataFile) {
      dataFile.write(gpsTime);
      dataFile.write(';');
      dataFile.write(gpsLat);
      dataFile.write(gpsHemLat);
      dataFile.write(';');
      dataFile.write(gpsLon);
      dataFile.write(gpsHemLon);
      dataFile.write(';');
      dataFile.write(gpsAlt);
      //dataFile.write(';');
      //dataFile.println(temperature);
      dataFile.close();
    }
    else {
      Serial.println("error opening datalog.txt");
    }
  }

  Serial.println("transmitting RTTY payload...");
  // put your main code here, to run repeatedly:
  //sprintf(radiostring, "%s: Hallo Welt!", call);

  sprintf(datastring,"%s, %s %c, %s %c, %s\n", gpsTime, gpsLat, gpsHemLat, gpsLon, gpsHemLon, gpsAlt); // Puts the text in the datastring
  Serial.print("payload: ");
  Serial.println(datastring);
  ss.end();
  rtty_txstring (datastring);
  Serial.println("RTTY transmission completed!");
  delay(5000);
}

void rtty_txbyte (char c)
{
  /* Simple function to sent each bit of a char to 
     ** rtty_txbit function. 
    ** NB The bits are sent Least Significant Bit first
    **
    ** All chars should be preceded with a 0 and 
    ** proceded with a 1. 0 = Start bit; 1 = Stop bit
    **
    */
 
  int i;
 
  rtty_txbit (0); // Start bit
 
  // Send bits for for char LSB first 
 
  for (i=0;i<7;i++) // Change this here 7 or 8 for ASCII-7 / ASCII-8
  {
    if (c & 1) rtty_txbit(1); 
 
    else rtty_txbit(0); 
 
    c = c >> 1;
 
  }
 
  rtty_txbit (1); // Stop bit
  rtty_txbit (1); // Stop bit
}

void rtty_txstring (char * string)
{
 
  /* Simple function to sent a char at a time to 
     ** rtty_txbyte function. 
    ** NB Each char is one byte (8 Bits)
    */
 
  char c;
 
  c = *string++;
 
  while ( c != '\0')
  {
    rtty_txbyte (c);
    c = *string++;
  }
}
 
void rtty_txbit (int bit)
{
  if (bit)
  {
    // high
    digitalWrite(RADIOPIN, HIGH);
  }
  else
  {
    // low
    digitalWrite(RADIOPIN, LOW);
 
  }
 
  //delayMicroseconds(3370); // 300 baud
  delayMicroseconds(10000); 
  delayMicroseconds(10150);
}

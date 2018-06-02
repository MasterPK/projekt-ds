#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <OneWire.h> 

// definování pinů pro SDA a RST
#define SS_PIN 48
#define RST_PIN 49

MFRC522 mfrc522(SS_PIN, RST_PIN);
String read_rfid;

void dump_byte_array(byte *buffer, byte bufferSize) {
  read_rfid = "";
  for (byte i = 0; i < bufferSize; i++) {
    read_rfid = read_rfid + String(buffer[i], HEX);
  }
}

//definovani teplomeru





char messageFromPC[32] ;
String data[30];
int pocetPolozek = 0;

const int numChars = 512;
char receivedChars[numChars];

boolean newData = false;

#define POHYB_CIDLO 7
#define TEPL_CIDLO 2
#define ZVONEK 24
#define DVERE_HLAVNI 25
#define DVERE_VEDLEJSI 26
#define NAPAJENI 27
#define DVERE_CIDLO X

//teplota

int DS18S20_Pin = TEPL_CIDLO; //čidlo je připojeno na pinu 2
 
OneWire ds(DS18S20_Pin);

float getTemp(){
 
byte data[12];
byte addr[8];
 
if ( !ds.search(addr)) {
ds.reset_search();
return -1000;
}
 
if ( OneWire::crc8( addr, 7) != addr[7]) {
Serial.println("CRC is not valid!");
return -1000;
}
 
if ( addr[0] != 0x10 && addr[0] != 0x28) {
Serial.print("Device is not recognized");
return -1000;
}
 
ds.reset();
ds.select(addr);
ds.write(0x44,1);
byte present = ds.reset();
ds.select(addr);
ds.write(0xBE);
for (int i = 0; i < 9; i++) { // potřebujeme 9 bytů
data[i] = ds.read();
}
 
ds.reset_search();
 
byte MSB = data[1];
byte LSB = data[0];
 
float tempRead = ((MSB << 8) | LSB);
float TemperatureSum = tempRead / 16;
 
return TemperatureSum;
 
}


//displej

void poslat_nextion(String zprava) {
  Serial2.print(zprava);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  delay(100);
}

#define nextion Serial2 // port pro komunikaci s displejem Nextion



String Nextion_receive(boolean read_data) { //returns generic

  boolean answer = false; // znacka
  char bite; // promenna pro ulozeni znaku
  String cmd; // promenna pro ulozeni textu
  byte countEnd = 0; // pocitadlo
  unsigned long previous; // cas spusteni
  int timeout = 1000; // doba po kterou se ceka na prichozi data
  previous = millis();

  do { // cekani na spravnou odpoved
    if (nextion.available() > 0) { // kdyz jsou k dispozici data, precti data
      bite = nextion.read();
      cmd += bite;
      if ((byte)bite == 0xff) countEnd++;
      if (countEnd == 3) answer = true;
    }
  }
  while (!answer && !((unsigned long)(millis() - previous) >= timeout)); // ceka na spravnou hodnotu, nebo uplynuti casu
  return cmd;
}
//------------------------------------------------------------------
String check_display() { // kontrola prijatych dat
  if (nextion.available() > 0) // kontroluje obsah pameti, pokud nen nic odeslano, dalsi cast programu se neprovede
  {
    return Nextion_receive(true); // precist hodnoty z serial portu
  }
}
//------------------------------------------------------------------
void nextion_init(int speed_init) { // nastaveni pri spusteni displeje
  nextion.begin(speed_init);
}



//načítání dat





void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;
 
 // if (Serial.available() > 0) {
    while (Serial3.available() > 0 && newData == false) {
        rc = Serial3.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

boolean showNewData() {
    if (newData == true) {
      for(int i=0;i<pocetPolozek;i++){
        data[i]="";
      }

      newData = false;
      
      char * strtokIndx; 
      
      strtokIndx = strtok(receivedChars, ";"); 

      if(strlen(strtokIndx) == 0) {
          return false;
        }
      
      pocetPolozek = atoi(strtokIndx);

     if(pocetPolozek==0){
      return false;
     }
      
      for(int i=0;i<pocetPolozek;i++){
        strtokIndx = strtok(NULL,";");      
        strcpy(messageFromPC, strtokIndx);
        data[i]=messageFromPC;
      };
      
      Serial.println("DATA:");
      
      for(int i=0;i<pocetPolozek;i++){
        Serial.println(data[i]);
      };
      
      
      Serial.println("Aktualizace: OK, "+(String)pocetPolozek);
      return true;
      
      
    }
    return false;
}


boolean update_rfid(){
  Serial.println("SEND Serial3: /arduino_rfid.php");
  Serial3.print("/arduino_rfid.php");
  delay(500);
  unsigned long timeout = millis();
  while(newData==false){
   recvWithStartEndMarkers();
   if (millis() - timeout > 10000) {
        break;
      }
  };
  if(showNewData()==true){
    return true;
  }else{
    return false;
  }
}



void setup() {
  Serial.begin(115200);
  Serial3.begin(115200);//Jakákoliv odeslaná zpráva se zpracovává jako http požadavek
  nextion_init(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(POHYB_CIDLO,INPUT);
  pinMode(TEPL_CIDLO,INPUT);
  
  pinMode(ZVONEK,OUTPUT);
  pinMode(DVERE_HLAVNI,OUTPUT);
  pinMode(DVERE_VEDLEJSI,OUTPUT);
  pinMode(NAPAJENI,OUTPUT);
  
  digitalWrite(ZVONEK,HIGH);
  digitalWrite(DVERE_HLAVNI,HIGH);
  digitalWrite(DVERE_VEDLEJSI,HIGH);
  digitalWrite(NAPAJENI,HIGH);
  
  delay(5000);
  boolean update_status=update_rfid();
  while(update_status==false){
    update_status=update_rfid();
  }
}

  
unsigned long aktualni_millis, predchozi_millis_lcd=0, predchozi_millis_temp=0, predchozi_millis_rfid=0;
boolean predchozi_stav=false;
unsigned long stav_lcd;

void loop() {

  aktualni_millis = millis();
  
  //načtení rfid z rpi jednou za 10 min
  if (aktualni_millis - predchozi_millis_rfid > 600000) {
    
    predchozi_millis_rfid = aktualni_millis;
    update_rfid();
  };

    //odeslání teploty do databaze jedno za 60 min
  if (aktualni_millis - predchozi_millis_temp > 3600000) {
    predchozi_millis_temp = aktualni_millis;
    int temperature = getTemp();
    Serial.println("SEND SERIAL3: /teplota_arduino.php?teplota=" + String(temperature));
    Serial3.println("/teplota_arduino.php?teplota=" + String(temperature));
  };

  //kontrola dveří
  
  /*boolean aktualni_stav=digitalRead(CIDLO_DVERE);
  if(aktualni_stav==false && predchozi_stav==true){
    Serial3.println("/dvere.php");
    Serial.println("DVERE OTEVRENY");
    predchozi_stav=false;
  }else if(aktualni_stav==true && predchozi_stav==false){
    Serial.println("DVERE ZAVRENY");
    predchozi_stav=true;
  }*/



//   pohybové čidlo
  if (digitalRead(POHYB_CIDLO)) {
    Serial.println("MOVE DETECT->DISP ON");
    poslat_nextion("sleep=0");
    stav_lcd = millis();
  } else {
    if (aktualni_millis - stav_lcd > 30000) {
      Serial.println("30 SEC NO MOVE DETECT->DISP OFF");
      poslat_nextion("sleep=1");
      return;
    };
  }

  //zobrazení teploty na lcd
  if (aktualni_millis - predchozi_millis_lcd > 1000) {
    predchozi_millis_lcd = aktualni_millis;
    int temperature = getTemp();
    Serial.println("SEND NEXTION: teplota.txt=\"" + String(temperature) + "\"");
    poslat_nextion("teplota.txt=\"" + String(temperature) + "\"");
  };




  String displej = Nextion_receive(true);

  if (displej[1] == '5' && displej[2] == '1') {
    digitalWrite(ZVONEK, LOW);
    delay(2000);
    while (Nextion_receive(true)[2] != '2') {};
    digitalWrite(ZVONEK, HIGH);
    Serial3.println("/zvonek.php");
  }

  if (displej[1] == '2' && displej[2] == '2') {
    Serial3.println("/pin.php?povoleno=1");
    digitalWrite(DVERE_HLAVNI, LOW);
    digitalWrite(DVERE_VEDLEJSI, LOW);
    delay(1000);
    digitalWrite(DVERE_VEDLEJSI, HIGH);
    delay(1000);
    digitalWrite(DVERE_HLAVNI, HIGH);
  }

  if (displej[1] == '3' && displej[2] == '2') {
    Serial3.println("/pin.php?povoleno=0");
  }


  if ( ! mfrc522.PICC_IsNewCardPresent())
    return;

  // Pokud nalezena
  if ( ! mfrc522.PICC_ReadCardSerial())
    return;

  dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);// z nějakého důvodu zůstává předchozí karta v mezipaměti
  dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);// proto je potřeba provést čtení dvakrát
  Serial.println("RFID READ: "+read_rfid);
  int kontrola=0;
  
  for(int i=0;i<pocetPolozek;i++){
    if(data[i]==read_rfid){
      kontrola=1;
    }
  }
  
  Serial.println("CHECK RFID: "+String(kontrola));
  
  if (kontrola == 1) {
    Serial.println("SEND Serial3: /pristup.php?RFID="+read_rfid);
    Serial3.println("/pristup.php?RFID="+read_rfid);
    poslat_nextion("page povoleno");
    digitalWrite(DVERE_HLAVNI, LOW);
    digitalWrite(DVERE_VEDLEJSI, LOW);
    delay(1000);
    digitalWrite(DVERE_VEDLEJSI, HIGH);
    delay(1000);
    digitalWrite(DVERE_HLAVNI, HIGH);
  } else {
    Serial3.println("/pristup.php?RFID="+read_rfid);
    poslat_nextion("page zamitnuto");
  }


  delay(1000);

}


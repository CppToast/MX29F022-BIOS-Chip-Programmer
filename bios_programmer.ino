#include <SimpleShell.h>

#define BAUDRATE 9600

#define MAX_RETRIES 10

#define SHIFT_DATA A3
#define SHIFT_CLOCK A4
#define SHIFT_LATCH A5

#define EEPROM_WE A2
#define EEPROM_OE 10
#define EEPROM_CE 11

#define EEPROM_A0 A0
#define EEPROM_A1 A1

#define EEPROM_Q0 2
#define EEPROM_Q1 3
#define EEPROM_Q2 4
#define EEPROM_Q3 5
#define EEPROM_Q4 6
#define EEPROM_Q5 7
#define EEPROM_Q6 8
#define EEPROM_Q7 9



// Utility functions

long hexStringToInt(String value) {
  return strtol(value.c_str(), NULL, HEX);
}

String intToHexString(long value, int len = 0) {
  String str = String(value, HEX);
  while(str.length() < len) str = "0" + str;
  return str;
}

String intToBinString(long value, int len = 0) {
  String str = String(value, BIN);
  while(str.length() < len) str = "0" + str;
  return str;
}

char intToPrintableChar(int value){
  if(isprint(value)){
    return (char) value;
  }else{
    return '.';
  }
}




// EEPROM interface functions

void eepromSetDataRead(){
  digitalWrite(EEPROM_WE, HIGH);
  for (int i = EEPROM_Q0; i <= EEPROM_Q7; i++) {
    pinMode(i, INPUT);
  }
}

void eepromSetDataWrite(){
  digitalWrite(EEPROM_WE, LOW);
  for (int i = EEPROM_Q0; i <= EEPROM_Q7; i++) {
    pinMode(i, OUTPUT);
  }
}

void eepromSetAddress(long address) {
  shiftOut(SHIFT_DATA, SHIFT_CLOCK, LSBFIRST, (address >> 2));
  shiftOut(SHIFT_DATA, SHIFT_CLOCK, LSBFIRST, (address >> 10));
  digitalWrite(EEPROM_A0, (address & 1));
  digitalWrite(EEPROM_A1, (address & 2));

  digitalWrite(SHIFT_LATCH, LOW);
  digitalWrite(SHIFT_LATCH, HIGH);
  digitalWrite(SHIFT_LATCH, LOW);
}

void eepromSetData(byte data) {
  eepromSetDataWrite();
  for (int i = EEPROM_Q0; i <= EEPROM_Q7; i++) {
    digitalWrite(i, data % 2);
    data = data >> 1;
  }
}

void eepromWrite(long address, byte data, bool leaveCEon = false) {
  eepromSetDataWrite();
  eepromSetAddress(address);
  eepromSetData(data);

  digitalWrite(EEPROM_CE, LOW);
  digitalWrite(EEPROM_WE, LOW);
  
  digitalWrite(EEPROM_WE, HIGH);
  if(!leaveCEon) digitalWrite(EEPROM_CE, HIGH);
}

byte eepromRead() {
  eepromSetDataRead();
  digitalWrite(EEPROM_CE, LOW);
  digitalWrite(EEPROM_OE, LOW);

  byte data = 0;
  for (int i = EEPROM_Q7; i >= EEPROM_Q0; i--) {
    data = data << 1;
    data += digitalRead(i);
    
  }

  digitalWrite(EEPROM_OE, HIGH);
  digitalWrite(EEPROM_CE, HIGH);

  return data;
}

byte eepromReadFromAddress(long address) {
  eepromSetAddress(address);
  return eepromRead();
}

void eepromReset() {
  eepromWrite(0, 0xf0);
}

void eepromProgram(long address, byte data){
  for(int i = 0; i < MAX_RETRIES; i++){
    eepromWrite(0x555, 0xaa);
    eepromWrite(0x2aa, 0x55);
    eepromWrite(0x555, 0xa0);
    eepromWrite(address, data, 1);
    digitalWrite(EEPROM_CE, HIGH);
    eepromReset();

    if(eepromReadFromAddress(address) == data) return;
    delay(1);
  }
  
  Serial.print("Program at address ");
  Serial.print(intToHexString(address));
  Serial.println(" failed!");
}

void eepromErase() {
  eepromWrite(0x555, 0xaa);
  eepromWrite(0x2aa, 0x55);
  eepromWrite(0x555, 0x80);
  eepromWrite(0x555, 0xaa);
  eepromWrite(0x2aa, 0x55);
  eepromWrite(0x555, 0x10, 1);
  delay(3000);
  eepromReset();
}


void eepromUnlock() {
  int count = 1;
  for(int i = 0; i < 1000; i++) {
    eepromWrite(0x555, 0xaa);
    eepromWrite(0x2aa, 0x55);
    eepromWrite(0x555, 0x80);
    eepromWrite(0x555, 0xaa);
    eepromWrite(0x2aa, 0x55);
    eepromWrite(0x555, 0x20);

    digitalWrite(EEPROM_OE, HIGH);
    eepromSetAddress(0b1001000000);
    digitalWrite(EEPROM_CE, LOW);

    digitalWrite(EEPROM_WE, LOW);
    digitalWrite(EEPROM_WE, HIGH);

    delay(10);

    eepromSetAddress(0b1000000010);
    digitalWrite(EEPROM_OE, LOW);
    digitalWrite(EEPROM_CE, LOW);

    if(eepromRead() == 0) {
      eepromReset();
      return;
    }
  }
  eepromReset();
  Serial.println("Failed!");
}














// Shell functions

void shEepromSetAddress(String *args) {

  // iterate through the arguments
  // TODO: extract this code into a separate function because this is ugly
  int index = 0;
  int newIndex = 0;
  int count = 0;
  String token;
  while(newIndex >= 0){
    newIndex = args->indexOf(' ', index);
    token = args->substring(index, newIndex);
    token.trim();
    if(token == "") {
      index++;
      continue;
    }
    
    switch(count) {
      case 0:
        count++;
        index = newIndex + 1;
        continue;
      case 1:
        eepromSetAddress(hexStringToInt(token));
        break;
      default:
        Serial.println("Excess arguments ignored!");
        return;
    }

    count++;
    index = newIndex + 1;
  }
}

void shEepromSetData(String *args) {

  // iterate through the arguments
  // TODO: extract this code into a separate function because this is ugly
  int index = 0;
  int newIndex = 0;
  int count = 0;
  String token;
  while(newIndex >= 0){
    newIndex = args->indexOf(' ', index);
    token = args->substring(index, newIndex);
    token.trim();
    if(token == "") {
      index++;
      continue;
    }
    
    switch(count) {
      case 0:
        count++;
        index = newIndex + 1;
        continue;
      case 1:
        eepromSetData(hexStringToInt(token));
        break;
      default:
        Serial.println("Excess arguments ignored!");
        return;
    }

    count++;
    index = newIndex + 1;
  }
}



void shEepromReadData(String *args) {

  long address = -1;
  int byteCount = 1;
  int rowCount = 1;

  // iterate through the arguments
  // TODO: extract this code into a separate function because this is ugly
  int index = 0;
  int newIndex = 0;
  int count = 0;
  String token;
  while(newIndex >= 0){
    newIndex = args->indexOf(' ', index);
    token = args->substring(index, newIndex);
    token.trim();
    if(token == "") {
      index++;
      continue;
    }
    
    switch(count) {
      case 0:
        count++;
        index = newIndex + 1;
        continue;
      case 1:
        address = hexStringToInt(token);
        break;
      case 2:
        byteCount = token.toInt();
        break;
      case 3:
        rowCount = token.toInt();
        break;
      default:
        Serial.println("Excess arguments ignored!");
    }

    count++;
    index = newIndex + 1;

    
  }

  if(address == -1) {
    Serial.print(intToHexString(eepromRead(), 2));
    Serial.print(" ");
    Serial.print(intToBinString(eepromRead(), 8));
    Serial.print(" ");
    Serial.print(intToPrintableChar(eepromRead()));
    Serial.println();

    return;
  }

  
  for(int r = 0; r < rowCount; r++) {
    Serial.print(intToHexString(address, 5));
    Serial.print(": ");
    
    for(int i = 0; i < byteCount; i++) {
      Serial.print(intToHexString(eepromReadFromAddress(address + i), 2));
      Serial.print(" ");
    }

    Serial.print("| ");
    for(int i = 0; i < byteCount; i++) {
      Serial.print(intToPrintableChar(eepromReadFromAddress(address + i)));
    }
    Serial.println();

    address += byteCount;
  }
  
}

void shEepromWriteData(String *args) {

  long address = -1;
  int data = -1;

  // iterate through the arguments
  // TODO: extract this code into a separate function because this is ugly
  int index = 0;
  int newIndex = 0;
  int count = 0;
  String token;
  while(newIndex >= 0){
    newIndex = args->indexOf(' ', index);
    token = args->substring(index, newIndex);
    token.trim();
    if(token == "") {
      index++;
      continue;
    }
    
    switch(count) {
      case 0:
        count++;
        index = newIndex + 1;
        continue;
      case 1:
        address = hexStringToInt(token);
        break;
      case 2:
        data = hexStringToInt(token);
        break;
      default:
        Serial.println("Excess arguments ignored!");
    }

    count++;
    index = newIndex + 1;

    
  }

  if(address == -1) {
    Serial.println("You must specify an address!");
    return;
  }

  if(data == -1) {
    Serial.println("You must specify data to write!");
    return;
  }

  eepromWrite(address, data);
}

void shEepromProgram(String *args) {

  long address = -1;

  // iterate through the arguments
  // TODO: extract this code into a separate function because this is ugly
  int index = 0;
  int newIndex = 0;
  int count = 0;
  String token;
  while(newIndex >= 0){
    newIndex = args->indexOf(' ', index);
    token = args->substring(index, newIndex);
    token.trim();
    if(token == "") {
      index++;
      continue;
    }
    
    switch(count) {
      case 0:
        count++;
        index = newIndex + 1;
        continue;
      case 1:
        address = hexStringToInt(token);
        break;
      default:
        eepromProgram(address + count - 2, hexStringToInt(token));
    }

    count++;
    index = newIndex + 1;

    
  }

  if(address == -1) {
    Serial.println("You must specify an address!");
    return;
  }

}

void shEepromReset(String *args) {
  eepromReset();
}

void shEepromErase(String *args) {
  eepromErase();
}

void shEepromUnlock(String *args) {
  eepromUnlock();
}






















// Main functions

void setup() {
  // put your setup code here, to run once:
  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLOCK, OUTPUT);
  pinMode(SHIFT_LATCH, OUTPUT);

  pinMode(EEPROM_WE, OUTPUT);
  pinMode(EEPROM_OE, OUTPUT);
  pinMode(EEPROM_CE, OUTPUT);

  pinMode(EEPROM_A0, OUTPUT);
  pinMode(EEPROM_A1, OUTPUT);

  digitalWrite(EEPROM_WE, HIGH);
  digitalWrite(EEPROM_OE, HIGH);
  digitalWrite(EEPROM_CE, HIGH);
  

  // Shell.registerCommand(new ShellCommand(argtest, F("argtest")));
  // Shell.registerCommand(new ShellCommand(test, F("test")));
  Shell.registerCommand(new ShellCommand(shEepromSetAddress, F("seta")));
  Shell.registerCommand(new ShellCommand(shEepromSetData, F("setd")));
  Shell.registerCommand(new ShellCommand(shEepromReadData, F("read")));
  Shell.registerCommand(new ShellCommand(shEepromWriteData, F("write")));
  Shell.registerCommand(new ShellCommand(shEepromProgram, F("program")));
  Shell.registerCommand(new ShellCommand(shEepromErase, F("erase")));
  Shell.registerCommand(new ShellCommand(shEepromReset, F("reset")));
  Shell.registerCommand(new ShellCommand(shEepromUnlock, F("unlock")));

  // aliases
  Shell.registerCommand(new ShellCommand(shEepromReadData, F("r")));
  Shell.registerCommand(new ShellCommand(shEepromWriteData, F("w")));
  Shell.registerCommand(new ShellCommand(shEepromProgram, F("p")));
  Shell.registerCommand(new ShellCommand(shEepromErase, F("e")));

  Serial.begin(BAUDRATE);
  Serial.println();
  //Serial.println("This is just some padding for the serial to start working correctly... please ignore.");
  Serial.println(" -=MX29F022 BIOS Chip Programmer=-");
  Serial.println("          by Fishie </><");
  Shell.begin(BAUDRATE);
  
}

void loop() {
  Shell.handleEvent();
}

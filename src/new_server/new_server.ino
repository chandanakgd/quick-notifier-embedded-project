#include <EEPROM.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include "FS.h"
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Wire.h>


LiquidCrystal_I2C lcd(0x3F, 16, 4);


//#define int COUNT 20;
 
const char* ssid = "TIM_E5-213e";
const char* password = "10166158";
//const char* ssid = "test";
//const char* password = "password";

//const char* ssid = "Dinuka";
//const char* password = "12345678";
const int COUNT = 20;

IPAddress ip(192,168,1,101);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);


const String key = "100200";

const byte n_rows = 3;
const byte n_cols = 3;

char keys[n_rows][n_cols] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'}
//  {'*','0','#'}
};

int readmsgpos = -1;

byte colPins[n_rows] = {D0, D3, D4};
byte rowPins[n_cols] = {D5, D6, D7};

Keypad myKeypad = Keypad( makeKeymap(keys), colPins, rowPins, n_rows, n_cols);

int eeprom_size = 0;

String senders[COUNT];
String msgs[COUNT];
String msgUser[COUNT];
int pos = 0;

String missedClLst[COUNT];
String clTime[COUNT];
String clDate[COUNT];
String clUser[COUNT];
int clPos = 0;

int isRinging = 0;

String newCalls[5];
String newCallUser[5];
int newCallPos = 0;

int selector = 0;
int readcallpos = -1;

String users[10];
String pins[10];

String curr_user = "";
int user_count = 0;


int pin_count;
String pin = "";

ESP8266WebServer server(80);

void handleRoot() {
  Serial.println("root request");
  server.send(200, "text/plain", "NodeMCU is a successful webserver! : COM07");
}

void test() {
  Serial.println("test request");
 

  server.send(200, "text/plain", "NodeMCU test : COM07");
}

void authenticate(){
  String key_ = server.arg("key");
  String pin = server.arg("pin");
  String user = server.arg("user");

  for(int i=0; i<user_count; i++){
    if(user == users[i]){
      server.send(200, "text/plain", "user_not_available");
      return;
    }
  }
  
  if(key == key_){

    String val = user+"$"+pin;
    if(eeprom_size == 0){
      EEPROM.write(0,'^');
      EEPROM.commit();  
      eeprom_size = 1;
      
      writeToEEPROM(val, eeprom_size);
    }else{
      writeToEEPROM("&"+val, eeprom_size);
    }
    
    
     Serial.println("Authenticated");
     server.send(200, "text/plain", "authenticated");
     loadFromEEPROM();
  }else{
    Serial.println("Not Authenticated");
     server.send(200, "text/plain", "not_authenticated");
  }

}
void endRinging(){
  String caller = server.arg("caller");
    String user = server.arg("user");
    printToLCD("Call Ended...");
    HTTPClient http;
    
    caller = urlencode(caller);
//    time_ = urlencode(time_);
//    date_ = urlencode(date_);
//    user = urlencode(user);
    http.begin("http://192.168.1.105/end_ringing?caller="+caller+"&user="+user);
    
    int httpCode = http.GET();
    if(httpCode == HTTP_CODE_OK)  {
        String response = http.getString();
        Serial.println(response);
      
    }  else  {
      Serial.println("Error in HTTP request");
    }
    
    http.end();

   int index = -1;
    for(int i=newCallPos; i>=0;i--){
  
      if(newCalls[i].equals(server.arg("caller")) && newCallUser[i].equals(user)){
        index = i;
        break;
      }
    }
  
    if(index != -1){
      for(int i=index; i<newCallPos-1;i++){
        newCalls[i] = newCalls[i+1];
        newCallUser[i] = newCallUser[i+1];
      }
      newCallPos--;
    }
    
    server.send(200, "text/plain", "call answered notification received..!");
    
    delay(1000);
    printToLCD(String(clPos)+" missed calls   "+String(pos)+" new msgs      1 - View msgs    2 - View calls");


 

    
}

void missedCall(){
  String caller = server.arg("caller");
  String time_ = server.arg("time");
  String date_ = server.arg("date");
  String user = server.arg("user");
  
  missedClLst[clPos] = caller;
  clTime[clPos] = time_;
  clDate[clPos] = date_;
  clUser[clPos] = user;

  if(clPos == COUNT - 1){
    removeOldMissedCallData();
  }else{
    clPos++;
  }

  readcallpos = clPos - 1;
 
  HTTPClient http;
  caller = urlencode(caller);
  time_ = urlencode(time_);
  date_ = urlencode(date_);
  user = urlencode(user);
  
  http.begin("http://192.168.1.105/missed_call?caller="+caller+"&time="+time_+"&date="+date_+"&user="+user);

  int httpCode = http.GET();
  if(httpCode == HTTP_CODE_OK)  {
      String response = http.getString();
      Serial.println(response);
    
  }  else  {
    Serial.println("Error in HTTP request");
  }

  http.end();

  caller = server.arg("caller");
  time_ = server.arg("time");
  date_ = server.arg("date");
  user = server.arg("user");
   Serial.println(user+" missed a call from "+caller+" on "+date_+" at "+time_);
  if(selector == 0){
    printToLCD(user+" missed a call from "+caller+" at "+time_+" on "+date_);
  }

  int index = -1;
  for(int i=newCallPos; i>=0;i--){

    if(newCalls[i].equals(server.arg("caller")) && newCallUser[i].equals(user)){
      index = i;
      break;
    }
  }

  if(index != -1){
    for(int i=index; i<newCallPos-1;i++){
      newCalls[i] = newCalls[i+1];
      newCallUser[i] = newCallUser[i+1];
    }
    newCallPos--;
  }

  delay(2000);

  if(selector == 0){
    printToLCD(String(clPos)+" missed calls   "+String(pos)+" new msgs      1 - View msgs    2 - View calls");
  }
  server.send(200, "text/plain", "missed call notification received..!");
}

void newMsg(){

 String msg = server.arg("msg");
 String sender = server.arg("sender");
 String user = server.arg("user");
  
  msgs[pos] = msg;
  senders[pos] = sender;
  msgUser[pos] = user;
   if(pos == COUNT - 1){
    removeOldMsgData();
  }else{
    pos++;
  }
  readmsgpos = pos - 1;
  HTTPClient http;
  msg = urlencode(msg);
  sender = urlencode(sender);
  
  http.begin("http://192.168.1.105/sms_recieved?msg="+msg+"&sender="+sender+"&user="+user);

  int httpCode = http.GET();
  if(httpCode == HTTP_CODE_OK)  {
    String response = http.getString();
    Serial.println(response);
  }  else  {
    Serial.println("Error in HTTP request "+ http.getString()+" "+String(httpCode));
  }

  http.end();
  msg = server.arg("msg");
  sender = server.arg("sender");
  user = server.arg("user");
  Serial.println("Msg : "+msg + " by : "+sender +" to : "+user );
  if(selector == 0){
    printToLCD("Msg : "+msg + " by : "+sender +" to : "+user );
  }
  delay(4000);

  if(selector == 0){
    printToLCD(String(clPos)+" missed calls   "+String(pos)+" new msgs      1 - View msgs    2 - View calls");
  }

  server.send(200, "text/plain", "Message Recieved : COM07");
  
  
}
//
//void f(){
//  SPIFFS.begin();
//  SPIFFS.format();
//
//  File f = SPIFFS.open("/data.txt", "a+");
//  Serial.println("satartsdflkjklsdflk");
//  if (!f) {
//    Serial.println("file open failed");
//  } else {
//    Serial.println("file open success");
//  }
//  
//  f.print("ABC");
//  f.flush();
//  f.close();
//  Serial.println("write");
//}

void newCall(){
  server.send(200, "text/plain", "call notification received..!");
   
//   
    String caller = server.arg("caller");
    String user = server.arg("user");
    Serial.println("Call recieving by " + caller +" to " + user);
    if(selector == 0){
      printToLCD("Call recieving by " + caller +" to " + user);
    }
    newCalls[newCallPos] = caller;
    newCallUser[newCallPos] = user;
    if(pos == 5 - 1){
      
    }else{
      newCallPos++;  
    }
    
  HTTPClient http;
  caller = urlencode(caller);
  
  
  http.begin("http://192.168.1.105/new_call?caller="+caller+"&user="+user);

  int httpCode = http.GET();
  if(httpCode == HTTP_CODE_OK)  {
      String response = http.getString();
      Serial.println(response);
    
  }  else  {
    Serial.println("Error in HTTP request");
  }

  http.end();

}

void getUser(){
  String pin = server.arg("pin");
  boolean found = false;
  for(int i=0;i<user_count;i++){
    if(pins[i] == pin){
      found = true;
      server.send(200, "text/plain", users[i]);
      break;
    }
  }
  if(!found){
    server.send(404, "text/plain", "Not found");
  }
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void loadFromEEPROM(){
  int j = 0;
  String str = "";
  boolean userRead = false;
  for(int i=0; i<eeprom_size+1; i++){
    char ch = char(EEPROM.read(i));
    if(i==0 && ch != '^'){
      eeprom_size = 0;
      break;
    }
    
    if(ch == '#'){
      eeprom_size = i;
      pins[j] = str;
      Serial.println("Pin : " + str);
      str = "";
      userRead = false;
      j++;
      break;
    }
    if(ch == '$'){
      users[j] = str;
      Serial.println("User : " + str);
      str = "";
      userRead = true;
      continue;
    }
    if(ch == '&' && userRead){
      pins[j] = str;
      Serial.println("Pin : " + str);
      str = "";
      userRead = false;
      j++;
      continue;
    }
    if(ch != '^'){
      str += ch;  
    }
    
    user_count++;
    
  }
}

void writeToEEPROM(String val, int addr){
  int j = 0;
  for(int i = addr; i < addr+val.length(); i++){
    EEPROM.write(i,val[j++]);
    eeprom_size = i;
  }
  eeprom_size++;
  EEPROM.write(eeprom_size,'#');
  EEPROM.commit();
  
 // eeprom_size++;
}


void clear_(){
  for(int i = 0; i < 512; i++){
    EEPROM.write(i,0);
  }
  EEPROM.commit();
  server.send(200, "text/plain", "All Cleared");
}

void details(){
  loadFromEEPROM();
  Serial.println("WiFi status : "+String(WiFi.status()));
  Serial.println("Connected to wifi with IP : "+String(WiFi.localIP()));
  Serial.println();
  Serial.println("===============================================================");
  Serial.println("User Count : "+String(user_count));
  for(int i=0; i < user_count; i++){
    Serial.println("User : "+users[i] + " -> PIN : " + pins[i]);
  }
  Serial.println();
  Serial.println("===============================================================");
  Serial.println("EEPROM SIZE : "+String(eeprom_size));
   String str = "";
  for(int i = 0; i < 255; i++){
    char ch = EEPROM.read(i);
    str += ch;
    if (ch == '#'){
      break;
    }
  }
  Serial.println("EEPROM contains : "+str);
  Serial.println();
  Serial.println("===============================================================");
  Serial.println("===============================================================");
  server.send(200,"text/plain", "Printed");
  
}

void printEEPROM(){
  String str = "";
  eeprom_size = 0;
  for(int i = 0; i < 255; i++){
    char ch = EEPROM.read(i);
    str += ch;
    if (ch == '#'){
      break;
    }
    eeprom_size++;
  }
  
  Serial.println("EEPROM contains : "+str);
  Serial.println("EEPROM size : "+String(eeprom_size));
  server.send(200, "text/plain", str);
  
}
void setup(void){

  Serial.begin(115200);
  EEPROM.begin(512);
  
 // f();
  WiFi.begin(ssid, password);
  WiFi.config(ip,gateway,subnet);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
   // digitalWrite(RELAY_PIN, 1);
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/test", test);
  server.on("/new_msg", newMsg); 
  server.on("/new_call", newCall);
  server.on("/end_ringing", endRinging);
  server.on("/missed_call", missedCall);
  server.on("/auth", authenticate);
  server.on("/get_user", getUser);
  server.on("/clear", clear_);
  server.on("/printe", printEEPROM);
  server.on("/details", details);

  server.onNotFound(handleNotFound);
  
  server.begin();

  lcd.begin(16,4);
  lcd.init();
  lcd.backlight();
  
  Serial.println("HTTP server started");
  printToLCD("Welcome To Our Quick Notifier. Group 17. CO321 - Embedded System E13041 E13170 E13234");


  printToLCD(String(pos)+" new msgs     "+String(clPos)+" missed calls      1 - View msgs    2 - View calls");
  printEEPROM();
  loadFromEEPROM();
//Serial.println("Setup completed..");
  
}


void loop(void){


  if(newCallPos>0){
    for(int i=0;i<newCallPos;i++){
       printToLCD("Call recieving by " + newCalls[i] +" to " + newCallUser[i]);
       delay(3000);
    }
  }

  char k = myKeypad.getKey();
 
  if (k != NULL){
    Serial.print("Key pressed: ");
    Serial.println(k);

    
    if(selector!= 5 && k == '2'){
      selector = 2;
      printToLCD("Selected to view calls");
    }
    if(selector!= 5 && k == '1'){
     
      printToLCD("Enter pin:");
      lcd.setCursor(-4,2);
      selector = 5;
      pin_count = 0;
      k = '\0';
      
    }

    
    
    if(selector == 1 && k == '6'){
      if (readmsgpos >= pos){
        printToLCD("No any next msgs");
        readmsgpos = pos - 1;
        delay(1000);
      }else{
        printToLCD("View next msg");
        delay(1000);

        for(int i=readmsgpos; i < pos; i++){
          if(msgUser[i] == curr_user){
            readmsgpos = i;
          }
        }
        if(readmsgpos >= pos){
          printToLCD("No any next msgs");
        }else{
          printToLCD("From: "+senders[readmsgpos]+" : "+msgs[readmsgpos]);  
        }
        
        readmsgpos++;
      }
    }
    if(selector == 1 && k == '4'){
      if (readmsgpos < 0){
        printToLCD("No any previous msgs");
        readmsgpos = 0;
        delay(1000);
      }else{
        printToLCD("View previous msg");
        delay(1000);
        
        for(int i=readmsgpos; i >= 0; i--){
          if(msgUser[i] == curr_user){
            readmsgpos = i;
          }
        }
        if(readmsgpos == -1){
          printToLCD("No any previous msgs");
        }else{
          printToLCD("From: "+senders[readmsgpos]+" : "+msgs[readmsgpos]);  
        }
        readmsgpos--;
      }
    }
    if(selector == 2 && k == '6'){
      if (readcallpos >= clPos){
        printToLCD("No any next missed call");
        readcallpos = clPos - 1;
        delay(1000);
      }else{
        printToLCD("View next missed call");
        delay(1000);
//        printToLCD(clUser[readcallpos]+" missed call from "+missedClLst[readcallpos]+" @ "+clTime[readcallpos]+" on "+clDate[readcallpos]);
        printToLCD(clUser[readcallpos]+" missed call from "+missedClLst[readcallpos]+" @ "+clTime[readcallpos]);
        readcallpos++;
      }
    }
    if(selector == 2 && k == '4'){
      if (readcallpos < 0){
        printToLCD("No any previous missed call");
        delay(1000);
        readcallpos = 0;
      }else{
        printToLCD("View previous missed call");
        delay(1000);
//        printToLCD(clUser[readcallpos]+" missed call from "+missedClLst[readcallpos]+" @ "+clTime[readcallpos]+" on "+clDate[readcallpos]);
        printToLCD(clUser[readcallpos]+" missed call from "+missedClLst[readcallpos]+" @ "+clTime[readcallpos]);
        readcallpos--;
      }
    }
    if(selector!= 5 && k == '5'){
      printToLCD(String(clPos)+" missed calls   "+String(pos)+" new msgs      1 - View msgs    2 - View calls");
      curr_user = "";
      selector = 0;
    }


    if(selector == 5){
      
      lcd.setCursor(-4+pin_count, 2);
      switch(k){
        case '1':
          pin += "1";
          pin_count++;
          lcd.print("*");
          break;
       case '2':
          pin += "2";
          pin_count++;
          lcd.print("*");
          break;
       case '3':
          pin += "3";
          pin_count++;
          lcd.print("*");
          break;
        case '4':
          pin += "4";
          pin_count++;
          lcd.print("*");
          break;
        case '5':
          pin += "5";
          pin_count++;
          lcd.print("*");
          break;
        case '6':
          pin += "6";
          pin_count++;
          lcd.print("*");
          break;
        case '7':
          pin += "7";
          pin_count++;
          lcd.print("*");
          break;
        case '8':
          pin += "8";
          pin_count++;
          lcd.print("*");
          break;
        case '9':
          pin += "9";
          pin_count++;
          lcd.print("*");
          break;      
      }

      if(pin_count == 4){
        selector = 0;
        
        boolean found = false;
        curr_user = "";
        for(int i=0;i<user_count;i++){
          if(pins[i] == pin){
            found = true;
            curr_user = users[i];
            break;
          }
        }
        if(!found){
          printToLCD("Incorrect pin!!!");
        }else{
          selector = 1; 
          printToLCD("Selected to view msgs"); 
        }
        pin = "";
        pin_count = 0;
      }
     
      
    }
  }

  server.handleClient();
}


void removeOldMissedCallData(){
  for(int i=1;i<COUNT;i++){
    missedClLst[i-1] = missedClLst[i];
    clTime[i-1] = clTime[i];
    clDate[i-1] = clDate[i];
    clUser[i-1] = clUser[i];
  }
}


void removeOldMsgData(){
  for(int i=1;i<COUNT;i++){
    senders[i-1] = senders[i];
    msgs[i-1] = msgs[i];
    
  }
}

String urldecode(String str)
{
    
    String encodedString="";
    char c;
    char code0;
    char code1;
    int i;
    for (i =0; i < str.length(); i++){
        c=str.charAt(i);
      if (c == '+'){
        encodedString+=' ';  
      }else if (c == '%') {
        i++;
        code0=str.charAt(i);
        i++;
        code1=str.charAt(i);
        c = (h2int(code0) << 4) | h2int(code1);
        encodedString+=c;
      } else{
        
        encodedString+=c;  
      }
      
      yield();
    }
    
   return encodedString;
}

String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    int i;
    for (i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;
    
}

unsigned char h2int(char c)
{
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}



void printToLCD(String massage){
  lcd.clear();
  int startP=0;
  int endP=massage.indexOf(' ');
  int nextSpace=endP;
  //Serial.println(massage.length());
  int count=0;
  
  while(count<4){
    if(count<2){
      lcd.setCursor(0,count);  
    }else{
      lcd.setCursor(-4,count);  
    }
    
    nextSpace=massage.indexOf(' ', endP+1);
    if((nextSpace-startP)<=16 && nextSpace!=-1){
        endP=nextSpace;
        delay(10);
    }else if((nextSpace-startP)>16){
        count+=1;
        Serial.println(massage.substring(startP,endP+1));
        lcd.print(massage.substring(startP,endP+1));
        
        startP=endP+1;
    }else if((massage.length()-startP)<=16 && nextSpace==-1){
        Serial.println(massage.substring(startP));
        lcd.print(massage.substring(startP));
        
        break;
    }else{
        Serial.println(massage.substring(startP,endP+1));
        lcd.print(massage.substring(startP,endP+1));
//        Serial.println(massage.substring(endP+1));
//        lcd.print(massage.substring(endP+1));
//        break;  
        count++;
        startP = endP+1;
    }
    if(count==4){
//     
      delay(2000);
      lcd.clear();
      count=0;  
    }
  }
}

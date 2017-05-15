
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <Keypad.h>
#include <WiFiClient.h>
//#include <TimedAction.h>
#include <ESP8266WebServer.h>

//#define int COUNT 20
const char* ssid = "TIM_E5-213e";
const char* password = "10166158";
//const char* ssid = "Dinuka";
//const char* password = "12345678";

const int COUNT = 20;

LiquidCrystal_I2C lcd(0x3F, 16, 4);


const byte n_rows = 3;
const byte n_cols = 3;

char keys[n_rows][n_cols] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'}
//  {'*','0','#'}
};

int readmsgpos = -1;
int selector = 1;
int readcallpos = -1;

byte colPins[n_rows] = {D0, D3, D4};
byte rowPins[n_cols] = {D5, D6, D7};

Keypad myKeypad = Keypad( makeKeymap(keys), colPins, rowPins, n_rows, n_cols);



IPAddress ip(192,168,1,105);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

String senders[COUNT];
String msgs[COUNT];
String msgUser[COUNT];
int pos = 0;

String missedClLst[COUNT];
String clTime[COUNT];
String clDate[COUNT];
String clUser[COUNT];
int clPos = 0;

String newCalls[5];
String newCallUser[5];
int newCallPos = 0;

ESP8266WebServer server(80);

void handleRoot() {
  Serial.println("root request");
  server.send(200, "text/plain", "NodeMCU is a successful webserver! : COM03");
}

void sms_recieved(){
  
 String msg = server.arg("msg");
 String sender = server.arg("sender");
 String user = server.arg("user");
  Serial.println("Msg : "+msg + " by : "+sender +" to : "+user );
  printToLCD("msg: "+msg + " by: "+sender +" to: "+user );
  msgs[pos] = msg;
  senders[pos] = sender;
   msgUser[pos] = user;
   if(pos == COUNT - 1){
    removeOldMsgData();
  }else{
    pos++;
  }
 readmsgpos = pos - 1;
  
  server.send(200, "text/plain", "Message recieved : "+msg+" by : "+sender);
  delay(4000);
  printToLCD(String(clPos)+" missed calls   "+String(pos)+" new msgs      1 - View msgs    2 - View calls");
}

void printAll(){
  String all = "";
  for(int i=0; i<=pos;i++){
    Serial.println(msgs[i]);
    all = all +"\n"+ msgs[i];
  }
  server.send(200, "text/plain", "All Printed : "+all);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

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
  server.on("/sms_recieved", sms_recieved);
  server.on("/print", printAll);
  server.on("/new_call", newCall);
  server.on("/end_ringing", endRinging);
  server.on("/missed_call", missedCall);
  server.begin();

  Serial.println("HTTP server started");
  
  lcd.begin(16,4);
  lcd.init();
  lcd.backlight();
  printToLCD("Welcome To Our Quick Notifier. Group 17. Embedded System CO321");
 // TimedAction numberThread = TimedAction(500,clientHandler);

 printToLCD(String(clPos)+" missed calls   "+String(pos)+" new msgs      1 - View msgs    2 - View calls");
  
}

void newCall(){
    
  server.send(200, "text/plain", "call notification recieved");
  String caller = server.arg("caller");
    String user = server.arg("user");
    Serial.println("Call recieving by " + caller +" to " + user);
    printToLCD("Call recieving by " + caller +" to " + user);
     newCalls[newCallPos] = caller;
    newCallUser[newCallPos] = user;
    if(pos == 5 - 1){
      
    }else{
      newCallPos++;  
    }
  
 // printToLCD(String(clPos)+" missed calls   "+String(pos)+" new msgs      1 - View msgs    2 - View calls");
}

void endRinging(){
   server.send(200, "text/plain", "call ring ended notification recieved");  
  String caller = server.arg("caller");
    String user = server.arg("user");
    printToLCD("Call Ended...");

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
 
    
    delay(1000);
    printToLCD(String(clPos)+" missed calls   "+String(pos)+" new msgs      1 - View msgs    2 - View calls");
    
  
}

void missedCall(){
  server.send(200, "text/plain", "missed call notification recieved");
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
  Serial.println(user+" missed a call from "+caller+" on "+date_+" at "+time_);
  printToLCD(user+" missed a call from "+caller+" at "+time_+" on "+date_);
  

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
  printToLCD(String(clPos)+" missed calls   "+String(pos)+" new msgs      1 - View msgs    2 - View calls");
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

void loop() {

  if(newCallPos>0){
    for(int i=0;i<newCallPos;i++){
       printToLCD("Call recieving by " + newCalls[i] +" to " + newCallUser[i]);
       delay(3000);
    }
  }

  server.handleClient();
  char k = myKeypad.getKey();
 
  if (k != NULL){
    Serial.print("Key pressed: ");
    Serial.println(k);

    if(k == '1'){
      selector = 1;
      printToLCD("Selected to view msgs");
    }
    if(k == '2'){
      selector = 2;
      printToLCD("Selected to view calls");
    }
    if(selector == 1 && k == '6'){
      if (readmsgpos >= pos){
        printToLCD("No any next msgs");
        readmsgpos = pos - 1;
        delay(1000);
      }else{
        printToLCD("View next msg");
        delay(1000);
        printToLCD("To: "+msgUser[readmsgpos]+" from: "+senders[readmsgpos]+" : "+msgs[readmsgpos]);
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
        printToLCD("To: "+msgUser[readmsgpos]+" from: "+senders[readmsgpos]+" : "+msgs[readmsgpos]);
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
    if(k == '5'){
      printToLCD(String(clPos)+" missed calls   "+String(pos)+" new msgs      1 - View msgs    2 - View calls");
    }
  }

//  
  
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
//      while(1){
//        if(Serial.read()!=-1){
//          count=0;
//          break;
//        }
//      }
      delay(2000);
      lcd.clear();
      count=0;  
    }
  }
}
String urldecode(String str)
{
    
    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
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
    for (int i =0; i < str.length(); i++){
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

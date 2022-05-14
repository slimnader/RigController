

#include <EtherCard.h>
#include <enc28j60.h> 
#include <stdio.h>
#define STATIC 0  // set to 1 to disable DHCP (adjust myip/gwip values below)
#define PSU 1
#define MOBO A0

bool isReset;
BufferFiller bfill;

// mac address
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
// ethernet interface ip address
static byte myip[] = { 192,168,1,200 };
// gateway ip address
static byte gwip[] = { 192,168,1,1 };

// LED to control output
int ledPin10 = 10;

byte Ethernet::buffer[700];

static word okayResp(char * action, char * stat){
  bfill = ether.tcpOffset();
  bfill.emit_p(PSTR(
    "HTTP/1.0 200 Okay\r\n"
    "Content-Type: application/json\r\n"
    "Retry-After: 600\r\n"
    "\r\n"
    "{\r\n"
    "'Action':'$S',\r\n"
    "'Status':'$S'\r\n"
    "}\r\n"),action, stat);
    return bfill.position();
  }
  

 static word readingsResp(float mobo, float psu){
  char m[5];
  char ps[5]; 
  String(mobo,5).toCharArray(m,5);
  String(psu,5).toCharArray(ps,5);
    bfill = ether.tcpOffset();
    bfill.emit_p(PSTR(
    "HTTP/1.0 200 Okay\r\n"
    "Content-Type: application/json\r\n"
    "Retry-After: 600\r\n"
    "\r\n"
    "{\r\n"
    "'Motherboard Reading':'$S',\r\n"
    "'Power Supply Reading':'$S'\r\n"
    "}\r\n"),m, ps);
    return bfill.position();
  
  }

  static word okayResp(char * action, char * stat,char * reason){
     bfill = ether.tcpOffset();
    bfill.emit_p(PSTR(
    "HTTP/1.0 200 Okay\r\n"
    "Content-Type: application/json\r\n"
    "Retry-After: 600\r\n"
    "\r\n"
    "{\r\n"
    "'Action':'$S',\r\n"
    "'Status':'$S',\r\n"
    "'Message':'$S'\r\n"
    "}\r\n"),action, stat,reason);
    return bfill.position();
  }
  
  




//================================================
//Voltage Configuration
// Floats for ADC voltage & Input voltage
float adc_voltage = 0.0;
float in_voltage = 0.0;
 
// Floats for resistor values in divider (in ohms)
float R1 = 100000.0;
float R2 = 10000.0; 

// Float for Reference Voltage
float ref_voltage = 5.0;

int adc_value = 0;


 //=====================================
 //VOLTAGE READ
float getAtPin(int pin){
  float analog_val = analogRead(pin);
  float voltage=(analog_val * ref_voltage) / 1024.0; 
  float input_voltage =voltage / (R2/(R1+R2)); 
  return input_voltage;
  }
  
  bool isMobo(){
  return getAtPin(MOBO)>0.3;
  }
  bool isPSU(){
  return getAtPin(PSU)>0.3;
}







void setup () {
//  pinMode(ledPin10, OUTPUT);

  pinMode(2,OUTPUT);
  pinMode(3,OUTPUT);
   pinMode(4,OUTPUT);

  Serial.begin(115200);
  Serial.println("Trying to get an IP...");

  Serial.print("MAC: ");
  for (byte i = 0; i < 6; ++i) {
    Serial.print(mymac[i], HEX);
    if (i < 5)
      Serial.print(':');
  }
  Serial.println();
  
  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) 
    Serial.println( "Failed to access Ethernet controller");

#if STATIC
  Serial.println( "Getting static IP.");
  if (!ether.staticSetup(myip, gwip)){
    Serial.println( "could not get a static IP");
//    blinkLed();     // blink forever to indicate a problem
  }
#else

  Serial.println("Setting up DHCP");
  if (!ether.dhcpSetup()){
    Serial.println( "DHCP failed");
//    blinkLed();     // blink forever to indicate a problem
  }
#endif
  
  ether.printIp("My IP: ", ether.myip);
//  ether.printIp("Netmask: ", ether.mymask);
  ether.printIp("GW IP: ", ether.gwip);
  ether.printIp("DNS IP: ", ether.dnsip);
}

void loop () {
 
  word len = ether.packetReceive();

  
  word pos = ether.packetLoop(len);
  char* chArray;
  // IF LED10=ON turn it ON
  
if(pos && len!=0){
  chArray = (char *)Ethernet::buffer + pos;
  response(requestCode(chArray));
}
    
}

void reset(){
  digitalWrite(2,HIGH);
  delay(750);
  digitalWrite(2,LOW);
  }

void powerON(){
  digitalWrite(3,HIGH);
  delay(250);
  digitalWrite(3,LOW);
  }
void powerOFF(){
  digitalWrite(3,HIGH);
  delay(2250);
  digitalWrite(3,LOW);
}
void psuPush(){
  digitalWrite(4,HIGH);
  delay(250);
  digitalWrite(4,LOW);
  }

void startAll(){
  digitalWrite(4,HIGH);
  delay(250);
  digitalWrite(4,LOW);
  delay(1000); 
  digitalWrite(3,HIGH);
  delay(250);
  digitalWrite(3,LOW);
  
}
  

void response(int code){
  switch(code){
    case 1:{
      if(isMobo()){
      reset();
      ether.httpServerReply(okayResp("Reset","Success"));      
      }
      else{
         ether.httpServerReply(okayResp("Reset","Failure","MotherBoard is Off"));    
        }
      break;
    }
    case 2:{
        if(!isMobo()){
          powerON();
          if(!isPSU){
               ether.httpServerReply(okayResp("Power on","Success","Warning: psu was off"));     
            }
          else{  
            ether.httpServerReply(okayResp("Power on","Success"));
            }
          
          }
      }

    case 3:{
      if(isMobo()){
         powerOFF();
         ether.httpServerReply(okayResp("Power off","Success"));
      }
      else{
          ether.httpServerReply(okayResp("Power off","failure","MotherBoard is already off")); 
        }
     
      break;
    }
    case 4:{
      if(isPSU()&&isMobo()){
          ether.httpServerReply(okayResp("Toggle Psu","failure","Illegal: PSU and MOBO are both on")); 
          break;
        }
      else if(isMobo()){
           ether.httpServerReply(okayResp("Toggle Psu","failure","Illegal: Mobo already on, check chain sync"));
           break;  
        }
      else{
        psuPush();
          if(isPSU()){
              ether.httpServerReply(okayResp("Toggle Psu","Success","Turning OFF PSU"));
              break;  
            }
          else{
             ether.httpServerReply(okayResp("Toggle Psu","Success","Turning ON PSU"));
             break;  
            }
          }
      
      }
    case 5:{
       ether.httpServerReply(readingsResp(getAtPin(MOBO),getAtPin(PSU)));
       break;
      }
    case 6:{
      if(!isPSU()&&!isMobo()){
          startAll();
          ether.httpServerReply(okayResp("Hard Start","Success"));
          break;
        }
      else{
         ether.httpServerReply(okayResp("Hard Start","failure","Mobo and PSU must both be off"));
        }
    
      }
   case -1: {
    word w = okayResp("Unknown","Error","No Valid Header reader");
    Serial.println("Responding...");
   ether.httpServerReply(w);
      break;
    }   
    }
  
  }
int  requestCode(char * chArray){
  if(strstr(chArray, "GET /?p=r") != 0) {
    return 1;
    }
  
  if(strstr(chArray, "GET /?p=on") !=0){
    return 2;
    }
   if(strstr(chArray, "GET /?p=off") !=0){
    return 3;
  }
  if(strstr(chArray, "GET /?p=psu") !=0) return 4;
  
  if(strstr(chArray, "GET /?c=stat") !=0) return 5;
  if(strstr(chArray, "GET /?c=stat") !=0) return 6;


  
  return -1;
 }

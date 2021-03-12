#include <SoftwareSerial.h>

SoftwareSerial ESPserial(3, 2); // RX | TX

void mqtt_publish(SoftwareSerial &ESPserial, String msg, String topic){
  Serial.print("Publising: ");
  Serial.print(msg);
  Serial.print(", to topic: ");
  Serial.println(topic);

  int total_len = 5 + topic.length() + msg.length();
  int rem_len = 3 + topic.length() + msg.length();  
  char at_command[15] = "AT+CIPSEND=";
  if(total_len<10)
    at_command[11]=char(total_len+48);
  else{
    int first = total_len/10;
    at_command[11]=char(first+48);
    int sec = total_len%10;
    at_command[12]=char(sec+48);
  }
  strcat(at_command, "\r\n");
  ESPserial.write(at_command);
  
  delay(500);
  if (ESPserial.available()) {
    String recv = ESPserial.readString();
  }

  //Fixed Header + Topic Length
  byte pubMes[7 + topic.length() + msg.length()] = {0x30, rem_len, 0x00, topic.length()};
  
  //Topic Name
  for (int i=0;i<topic.length();i++)
    pubMes[4+i] = topic[i];

  //Property set to 0
  pubMes[4+topic.length()] = 0;
  
  //Message
  for (int i=0;i<msg.length();i++)
    pubMes[4  +topic.length()+i] = msg[i];
    
  ESPserial.write(pubMes, 5+topic.length() + msg.length());
  delay(500);
  Serial.println("Sent Pub Message");
  if (ESPserial.available()) {
    String ans = ESPserial.readString();
    Serial.print("ANS: ");
    Serial.println(ans);
  }
}

void mqtt_disconnect(SoftwareSerial &ESPserial) {
  Serial.println("\nSending Disconnect Command\n");
  ESPserial.write("AT+CIPSEND=2\r\n");
  delay(500);
  while (ESPserial.available()) {
    Serial.write(ESPserial.read());
  }
  byte ping[] = {0xE0, 0x00};
  ESPserial.write(ping, sizeof(ping));
  delay(500);
  while (ESPserial.available()) {
    Serial.write(ESPserial.read());
  }
}

void mqtt_ping(SoftwareSerial &ESPserial) {
  Serial.println("\nSending Ping Request\n");
  ESPserial.write("AT+CIPSEND=2\r\n");
  delay(500);
  while (ESPserial.available()) {
    Serial.write(ESPserial.read());
  }
  byte ping[] = {0xC0, 0x00};
  ESPserial.write(ping, sizeof(ping));
  delay(500);
  if (ESPserial.available()) {
    String ans = ESPserial.readString();
    Serial.println(ans);
  }
}

void mqtt_connect(SoftwareSerial &ESPserial, String username) {

  //TCP Connection
  ESPserial.write("AT+CIPSTART=\"TCP\",\"192.168.1.8\",1883,2\r\n");
  delay(500);
  while (ESPserial.available()) {
    Serial.write(ESPserial.read());
  }
  Serial.println();
  
  //Creating AT Command
  int total_length = 14+username.length();
  char at_command[15]="AT+CIPSEND=";
  int first = total_length/10;
  at_command[11]=char(first+48);
  int sec = total_length%10;
  at_command[12]=char(sec+48);
  strcat(at_command, "\r\n");
  
  //Sending AT Command
  ESPserial.write("AT+CIPSEND=19\r\n");
  delay(500);
  while (ESPserial.available()) {
    Serial.write(ESPserial.read());
  }
  //Creating MQTT Message
  byte message[total_length] = {0x10, total_length-2, 0x00, 0x04, 0x4d, 0x51, 0x54, 0x54, 0x04, 0x02, 0x00, 0x3c, 0x00, username.length()};
  for (int i=0; i<username.length(); i++){
    Serial.println(username[i]);
    message[14+i] = username[i];
  }
  
  Serial.println("Sent the message\n");
  ESPserial.write(message, total_length);
  delay(500);
  while (ESPserial.available()) {
    Serial.write(ESPserial.read());
  }
}

void mqtt_subscribe(SoftwareSerial &ESPserial, String topic) {
  Serial.print("\nSubscribing to Topic: ");
  Serial.print(topic);
  Serial.println();
  Serial.println(topic.length());
  int total_len = 7 + topic.length();
  int rem_len = 5 + topic.length();
  char at_command[15] = "AT+CIPSEND=";
  if(total_len<10)
    at_command[11]=char(total_len+48);
  else{
    int first = total_len/10;
    at_command[11]=char(first+48);
    int sec = total_len%10;
    at_command[12]=char(sec+48);
  }
  strcat(at_command, "\r\n");
  ESPserial.write(at_command);
  
  delay(500);
  while (ESPserial.available()) {
    Serial.write(ESPserial.read());
  }

  byte subMes[7+topic.length()] = {0x82, rem_len, 0x00, 0x01, 0x00, topic.length()};
  for (int i=0;i<topic.length();i++)
    subMes[6+i] = topic[i];
    
  subMes[6+topic.length()]= 0;
  ESPserial.write(subMes, 7+topic.length());
  delay(5000);
  Serial.println("Sent Sub Message");
  while (ESPserial.available()) {
    Serial.write(ESPserial.read());
  }
}

void connectAsClient(SoftwareSerial *esp, char *ssid, char *pass, bool &connected) {
  String modeAnswer;
  String conAnswer;
  Serial.println("Connecting....");
  esp->write("AT+CWMODE_CUR=1\r\n");
  delay(500);
  while (esp->available())
    modeAnswer = esp->readString();
  Serial.print(modeAnswer);
  delay(1000);
  if (modeAnswer == "AT+CWMODE_CUR=1\r\r\n\r\nOK\r\n") {
    connected = true;
    Serial.println("Changed Mode to Client.");
  }
  else
    return;
  Serial.print("Trying to connect to ");
  Serial.println(ssid);
  int len = 18 + strlen(ssid) + strlen(pass);
  char conCommand[len] = "AT+CWJAP_CUR=\"";
  strcat(conCommand, ssid);
  strcat(conCommand, "\",\"");
  strcat(conCommand, pass);
  strcat(conCommand, "\"\r\n");
  Serial.println(conCommand);
  esp->write(conCommand);
  delay(5000);
  while (esp->available())
    conAnswer = esp->readString();
  connected = true;
  Serial.print(conAnswer);
  delay(1000);

}



void setup() {
  Serial.begin(9600); // communication with the host computer
  ESPserial.begin(9600);
  
  mqtt_connect(ESPserial, "Paris");
  mqtt_subscribe(ESPserial, "Topic");
  
  Serial.println("Ready");
  Serial.println("");
 
}

void loop(){
  if (Serial.available()){
    String rec = Serial.readString();
    if (rec == "pub\r\n")
      mqtt_publish(ESPserial, "Correct Tag 1", "Topic");
    else if(rec == "ping\r\n")
      mqtt_ping(ESPserial);
    else if(rec == "dis\r\n")
      mqtt_disconnect(ESPserial);
  }
  if (ESPserial.available()){
    Serial.println("GOT MSG");
    String rec = ESPserial.readString();
    Serial.print("RECV: ");
    Serial.println(rec.substring(4,rec.length()));
    Serial.println(rec);
  }
}

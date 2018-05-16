/*

Com este software, pode comandar um portão de 2 folhas, utilizando um sonoff 4ch pro
Tem instruções para fecho, abertura normal e abertura de apenas 1 pessoa (abre uma folha apenas)

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>

#define BUTTON1         0                                    // (Don't Change for Sonoff 4CH)
#define BUTTON2         9                                    // (Don't Change for Sonoff 4CH)
#define BUTTON3         10                                   // (Don't Change for Sonoff 4CH)
#define BUTTON4         14                                   // (Don't Change for Sonoff 4CH)
#define RELAY1          12                                   // (Don't Change for Sonoff 4CH)
#define RELAY2          5                                    // (Don't Change for Sonoff 4CH)
#define RELAY3          4                                    // (Don't Change for Sonoff 4CH)
#define RELAY4          15                                   // (Don't Change for Sonoff 4CH)
#define LED             13                                   // (Don't Change for Sonoff 4CH)

#define MQTT_CLIENT     "ESP_Portao_Exterior"
#define MQTT_SERVER     "192.168.1.133"                      // servidor mqtt
#define MQTT_PORT       1883                                 // porta mqtt
#define MQTT_TOPIC      "portao_exterior"          // topic
#define MQTT_USER       "username"                               //user
#define MQTT_PASS       "mqttpass"                               // password

#define WIFI_SSID       "SSID"                           // wifi ssid
#define WIFI_PASS       "Wifipass"                           // wifi password

#define VERSION    "\n\n----------------  Sonoff Portao Exterior 1.01  -----------------"

bool requestRestart = false;                                 // (Do not Change)
bool sendStatus1 = false;               // (Do not Change)

int kUpdFreq = 1;                                            // Update frequency in Mintes to check for mqtt connection
int kRetries = 150;                                           // WiFi retry count. Increase if not connecting to router.

int estado_portao = 2; // 0:Fechado, 1:Aberto, 2:Posicao intermedia
int operacao_em_curso = 0; // 0: Nenhuma, 1:Abrindo, 2:Fechando, 3: Abrindo_gente

long tempo_abertura = 25; // tempo de abertura em segundos
long tempo_inicio_trabalho = 0;

bool botao1 = false;
bool botao2 = false;
bool botao3 = false;
bool botao4 = false;

unsigned long TTasks;                                        // (Do not Change)
unsigned long count1 = 0;                                    // (Do not Change)
unsigned long count2 = 0;                                    // (Do not Change)
unsigned long count3 = 0;                                    // (Do not Change)
unsigned long count4 = 0;                                    // (Do not Change)

extern "C" { 
  #include "user_interface.h" 
}

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient, MQTT_SERVER, MQTT_PORT);
Ticker btn_timer1;

void callback(const MQTT::Publish& pub) {
  if (pub.payload_string() == "stat") {
  }
  else if (pub.payload_string() == "Abrir") {
    abrir_portao();
  }
  else if (pub.payload_string() == "Abrir uma folha") {
    abrir_gente();
  }
  else if (pub.payload_string() == "Fechar") {
    fechar_portao();
  }
  else if (pub.payload_string() == "reset") {
    requestRestart = true;
  }
  sendStatus1 = true;
}

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  pinMode(BUTTON3, INPUT);
  pinMode(BUTTON4, INPUT);
  digitalWrite(LED, HIGH);
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);
  digitalWrite(RELAY3, LOW);
  digitalWrite(RELAY4, LOW);
  Serial.begin(115200);

  btn_timer1.attach(0.05, checkBotoes);

  mqttClient.set_callback(callback);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println(VERSION);
  Serial.print("\nUnit ID: ");
  Serial.print("esp8266-");
  Serial.print(ESP.getChipId(), HEX);
  Serial.print("\nConnecting to "); Serial.print(WIFI_SSID); Serial.print(" Wifi"); 
  while ((WiFi.status() != WL_CONNECTED) && kRetries --) {
    delay(500);
    Serial.print(" .");
  }
  if (WiFi.status() == WL_CONNECTED) {  
    Serial.println(" DONE");
    Serial.print("IP Address is: "); Serial.println(WiFi.localIP());
    Serial.print("Connecting to ");Serial.print(MQTT_SERVER);Serial.print(" Broker . .");
    delay(500);
    while (!mqttClient.connect(MQTT::Connect(MQTT_CLIENT).set_keepalive(90).set_auth(MQTT_USER, MQTT_PASS)) && kRetries --) {
      Serial.print(" .");
      delay(1000);
    }
    if(mqttClient.connected()) {
      Serial.println(" DONE");
      Serial.println("\n----------------------------  Logs  ----------------------------");
      Serial.println();
      mqttClient.subscribe(MQTT_TOPIC);
      blinkLED(LED, 40, 8);
      digitalWrite(LED, LOW);
    }
    else {
      Serial.println(" FAILED!");
      Serial.println("\n----------------------------------------------------------------");
      Serial.println();
    }
  }
  else {
    Serial.println(" WiFi FAILED!");
    Serial.println("\n----------------------------------------------------------------");
    Serial.println();
  }
}

void loop() { 
  mqttClient.loop();
  timedTasks();
  checkStatus();
}

void blinkLED(int pin, int duration, int n) {             
  for(int i=0; i<n; i++)  {  
    digitalWrite(pin, HIGH);        
    delay(duration);
    digitalWrite(pin, LOW);
    delay(duration);
  }
}

void button1() {
  if (!digitalRead(BUTTON1)) {
    count1++;
  } 
  else {
    if (count1 > 1 && count1 <= 40) {   
      botao1 = true;
    } 
    else if (count1 >40){
      Serial.println("\n\nSonoff Rebooting . . . . . . . . Please Wait"); 
      requestRestart = true;
    } 
    count1=0;
  }
}

void button2() {
  if (!digitalRead(BUTTON2)) {
    count2++;
  } 
  else {
    if (count2 > 1 && count2 <= 40) {   
      botao2 = true;
    } 
    count2=0;
  }
}

void button3() {
  if (!digitalRead(BUTTON3)) {
    count3++;
  } 
  else {
    if (count3 > 1 && count3 <= 40) {   
      botao3 = true;

    } 
    count3=0;
  }
}

void button4() {
  if (!digitalRead(BUTTON4)) {
    count4++;
  } 
  else {
    if (count4 > 1 && count4 <= 40) {   
      botao4 = true;
    } 
    count4=0;
  }
}
// 0: Nenhuma, 1:Abrindo, 2:Fechando, 3: Abrindo_gente
void checkBotoes()  {
  button1();
  button2();
  button3();
  button4();
  if(botao1){
    if(operacao_em_curso == 0)  abrir_portao();
    else  parar();
    botao1=0;
  }
  if(botao2){
    if(operacao_em_curso == 0)  fechar_portao();
    else  parar();
    botao2=0;
  }
  if(botao3){
    parar();
    botao3=0;
  }
  if(botao4){
    parar();
    botao4=0;
  }
}


void checkConnection() {
  if (WiFi.status() == WL_CONNECTED)  {
    if (mqttClient.connected()) {
      Serial.println("mqtt broker connection . . . . . . . . . . OK");
    } 
    else {
      Serial.println("mqtt broker connection . . . . . . . . . . LOST");
      requestRestart = true;
    }
  }
  else { 
    Serial.println("WiFi connection . . . . . . . . . . LOST");
    requestRestart = true;
  }
}

void checkStatus() {
  if (sendStatus1) {

    if(estado_portao == 0)  {
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "Fechado").set_retain().set_qos(1));
      Serial.println("estado_portao . . . . . . . . . . . . . . . . . . fechado");
    } 
    else if(estado_portao == 1) {       
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "Aberto").set_retain().set_qos(1));
      Serial.println("estado_portao . . . . . . . . . . . . . . . . . . aberto");
    }
    else {       
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "Posicao intermedia").set_retain().set_qos(1));
      Serial.println("estado_portao . . . . . . . . . . . . . . . . . . intermedio");
    }
    if(operacao_em_curso == 0)  {
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/oper", "Nenhuma").set_retain().set_qos(1));
      Serial.println("operacao_em_curso . . . . . . . . . . . . . . . . . . Nenhuma");
    } 
    else if(operacao_em_curso == 1) {       
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/oper", "Abrindo").set_retain().set_qos(1));
      Serial.println("operacao_em_curso . . . . . . . . . . . . . . . . . . Abrindo");
    }
    else if(operacao_em_curso == 2) {       
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/oper", "Fechando").set_retain().set_qos(1));
      Serial.println("operacao_em_curso . . . . . . . . . . . . . . . . . . Fechando");
    }
    else {       
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/oper", "Abrindo uma folha").set_retain().set_qos(1));
      Serial.println("operacao_em_curso . . . . . . . . . . . . . . . . . . Abrindo_gente");
    }
    sendStatus1 = false;
  }
  if (requestRestart) {
    blinkLED(LED, 400, 4);
    ESP.restart();
  }

}

void timedTasks() {
  if ((millis() > TTasks + (kUpdFreq*15000)) || (millis() < TTasks)) { 
    TTasks = millis();
    checkConnection();
    sendStatus1 = true;
  }
  if (operacao_em_curso == 1 && millis() > tempo_inicio_trabalho + 3000)
  {
    digitalWrite(RELAY3, LOW);
    digitalWrite(RELAY4, HIGH);
  }
  if (operacao_em_curso == 2 && millis() > tempo_inicio_trabalho + 3000)
  {
    digitalWrite(RELAY1, HIGH);
    digitalWrite(RELAY2, LOW);
  }
  if (operacao_em_curso >= 1 && millis() > tempo_inicio_trabalho + (tempo_abertura*1000))
  {
    if(operacao_em_curso == 1) estado_portao = 1;
    else if(operacao_em_curso == 2) estado_portao = 0;
    else if(operacao_em_curso == 3) estado_portao = 2;
    parar();
  }
}

//#######################################################################################
//#######################################################################################
//#######################################################################################
//#######################################################################################
//#######################################################################################
//#######################################################################################
//#######################################################################################
//#######################################################################################
//#######################################################################################

void abrir_portao() {
  Serial.println("ABRIR");
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, HIGH);
  tempo_inicio_trabalho = millis();
  estado_portao = 2;
  operacao_em_curso = 1;
  sendStatus1 = true;
}
//#######################################################################################
//#######################################################################################
//#######################################################################################
void abrir_gente() {
  Serial.println("ABRIR_gente");
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, HIGH);
  digitalWrite(RELAY3, LOW);
  digitalWrite(RELAY4, LOW);
  tempo_inicio_trabalho = millis();
  operacao_em_curso = 3;
  estado_portao = 2;
  sendStatus1 = true;
}
//#######################################################################################
//#######################################################################################
//#######################################################################################
void fechar_portao() {
  Serial.println("FECHAR");
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);
  digitalWrite(RELAY3, HIGH);
  digitalWrite(RELAY4, LOW);
  tempo_inicio_trabalho = millis();
  operacao_em_curso = 2;
  estado_portao = 2;
  sendStatus1 = true;
}
//#######################################################################################
//#######################################################################################
//#######################################################################################
void parar() {
  Serial.println("PARAR");
  digitalWrite(RELAY3, LOW);
  digitalWrite(RELAY4, LOW);
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);
  operacao_em_curso = 0;
  sendStatus1 = true;
}

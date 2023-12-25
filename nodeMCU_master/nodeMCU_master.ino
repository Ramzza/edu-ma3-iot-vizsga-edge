#include <SPI.h>
#include <ESP8266WiFi.h>
#include "ESP8266Firebase.h"
#include <string.h>
#define MSG_SIZE 100

#define DATABASE_URL "YOUR_FIREBASE_DB" // <databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app
#define _SSID "YOUR_WIFI_SSID"                                                                         // Your WiFi SSID
#define _PASSWORD "YOUR_WIFI_PASSWORD"                                                                  // Your WiFi Password

#define PROJECT_ID "YOUR_FIREBASE_PROJECT_ID" // Your Firebase Project ID. Can be found in project settings.

#define FIREBASE_HOST "YOUR_FIREBASE_PROJECT_ID"
#define FIREBASE_AUTH "YOUT_API_KEY"

#define KEY_TEMP "Temperature"
#define KEY_HUMID "Humidity"
#define KEY_LUMIN "Luminosity"
#define KEY_SWITCH "Switch_ON"

#define PING_INTERVAL 0
#define PING_INTERVAL_ACTIONS 0

Firebase firebase(PROJECT_ID);
char responseMsg[MSG_SIZE + 1];
char requestMsg[MSG_SIZE + 1];

int currentPingInterval[3] = {0, 0, 0};
int currentPingForActions = 0;



char temperatureMsg[MSG_SIZE + 1];
char humidityMsg[MSG_SIZE + 1];
char switchStateMsg[MSG_SIZE + 1];
char luminosityMsg[MSG_SIZE + 1];

bool isSwitchChanged = false;
bool isLedOn = false;
bool isLedChanged = false;
bool isStepperSwitchChanged = false;
bool isStepperOn = false;
bool isStepperDirectionChanged = false;
bool isStepperRight = true;
int actionCounter = 3;
int currentSwitchState = 1;

WiFiServer server(80);

SPISettings spi_settings(100000, MSBFIRST, SPI_MODE0);
//100 kHz legyen a sebesseg, a Node tud 80MHzt de az Arduino csak 16MHzt

void setRequestMsg(char msg[MSG_SIZE])
{
  // biztos ami biztos, lenullazzuk a request buffert
  strncpy(requestMsg, msg, MSG_SIZE);
}

void showUI(WiFiClient client)
{
  // HTML weboldal megjelenitese
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("");
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");

  client.print("Led is now: ");

  if (isLedOn)
  {
    client.print("On");
  }
  else
  {
    client.print("Off");
  }

  client.println("<br><br>");
  client.print("Stepper is now: ");

  if (isStepperOn)
  {
    client.print("On");
  }
  else
  {
    client.print("Off");
  }

  client.println("<br><br>");
  client.print("Stepper direction: ");

  if (isStepperRight)
  {
    client.print("RIGHT");
  }
  else
  {
    client.print("LEFT");
  }

  client.println("<br><br>");
  client.println("Temperature: " + String(temperatureMsg));
  client.println("<br><br>");

  client.println("<br><br>");
  client.println("Humidity: " + String(humidityMsg));
  client.println("<br><br>");

  client.println("<br><br>");
  client.println("Switch state: " + String(switchStateMsg));
  client.println("<br><br>");

  client.println("<br><br>");
  client.println("Luminosity: " + String(luminosityMsg));
  client.println("<br><br>");

  client.println("<br><br>");
  client.println("Last response: " + String(responseMsg));
  client.println("<br><br>");

  client.println("<a href=\"/LED=ON\"\"><button>IR LED On</button></a>");
  client.println("<a href=\"/LED=OFF\"\"><button>IR LED Off</button></a><br />");
  client.println("<br><br>");

  client.println("<a href=\"/STEPPER=ON\"\"><button>Stepper On</button></a>");
  client.println("<a href=\"/STEPPER=OFF\"\"><button>Stepper Off</button></a><br />");
  client.println("<br><br>");

  client.println("<a href=\"/STEPPER=RIGHT\"\"><button>Stepper RIGHT</button></a>");
  client.println("<a href=\"/STEPPER=LEFT\"\"><button>Stepper LEFT</button></a><br />");

  client.println("</html>");

  Serial.println("Web UI was refreshed");
  delay(1);
}

void checkButtonPressed(String request)
{
  // Ellenorizzuk, hogy a user milyen gombot nyomott meg
  if (request.indexOf("/LED=ON") != -1)
  {

    isLedChanged = isLedOn == false;
    isLedOn = true;
  }
  else if (request.indexOf("/LED=OFF") != -1)
  {
    isLedChanged = isLedOn == true;
    isLedOn = false;
  }
  else if (request.indexOf("/STEPPER=ON") != -1)
  {
    isStepperSwitchChanged = isStepperOn == false;
    isStepperOn = true;
  }
  else if (request.indexOf("/STEPPER=OFF") != -1)
  {
    isStepperSwitchChanged = isStepperOn == true;
    isStepperOn = false;
  }
  else if (request.indexOf("/STEPPER=RIGHT") != -1)
  {
    isStepperDirectionChanged = isStepperRight == false;
    isStepperRight = true;
  }
  else if (request.indexOf("/STEPPER=LEFT") != -1)
  {
    isStepperDirectionChanged = isStepperRight == true;
    isStepperRight = false;
  }
  else
  {
    isLedChanged = false;
    isStepperSwitchChanged = false;
    isStepperDirectionChanged = false;
  }
}


void fireBaseActions()
{
  currentPingForActions = currentPingForActions > 0 ? currentPingForActions-1 : PING_INTERVAL_ACTIONS;
 
  if(currentPingForActions > PING_INTERVAL_ACTIONS)
  {      
    return;
  }
  
  String irLedString = firebase.getString("IR_LED_SWITCH");
  String stepperOnString = firebase.getString("ROTATE_STEPPER");
  String stepperRightString = firebase.getString("STEPPER_DIRECTION");

  
  bool irLedOn = irLedString.equals("ON");
  bool stepperOn = stepperOnString.equals("ON");
  bool stepperRight = stepperRightString.equals("RIGHT");

  Serial.println();
  Serial.print("irLedString: ");
  Serial.println(irLedString);
  
  Serial.print("stepperOnString: ");
  Serial.println(stepperOnString);
  
  Serial.print("stepperRightString: ");
  Serial.println(stepperRightString);

  isLedChanged = isLedOn !=  irLedOn;
  isLedOn = irLedOn;

  
  isStepperSwitchChanged = isStepperOn !=  stepperOn;
  isStepperOn = stepperOn;

  
  isStepperDirectionChanged = isStepperRight !=  stepperRight;
  isStepperRight = stepperRight;
}

void writeToSlave()
{
  // Elkuldjuk a parancsot az Arduinonak
  for (int i = 0; i < MSG_SIZE; i++)
  {
    responseMsg[i] = i < strlen(requestMsg)
                         ? SPI.transfer(requestMsg[i])
                         : SPI.transfer(' ');
    delay(1);
  }

  setRequestMsg("");
}

void sendKeyValueToFirebase(String key, String value, String timeStr)
{
//  return;
  value.trim();
  timeStr.trim();
  key.trim();
  firebase.pushString(key, "{ \"time\": \"" + timeStr + "\",  \"value\": \"" + value + "\" }");
}

void handleMessageTypeFromSlave()
{
  if (NULL == strchr(responseMsg, '>') || NULL == strchr(responseMsg, '='))
  {
    // A nem "timestamp -> key = value" formaju uzenet nem erdekel
    return;
  }

  String resp = String(responseMsg);
  String messageKey = resp.substring(resp.indexOf('>') + 1, resp.indexOf('='));
  String messageValue = resp.substring(resp.indexOf('=') + 1, resp.length());
  String dateTime = resp.substring(0, resp.indexOf('>') - 1);
  String fullMessage = resp.substring(resp.indexOf('>') + 1, resp.length());

  if (messageKey.equals(KEY_TEMP))
  {
    memset(temperatureMsg, 0, sizeof(temperatureMsg));
    strcpy(temperatureMsg, responseMsg);
    currentPingInterval[0] = currentPingInterval[0] > 0 ? currentPingInterval[0]-1 : PING_INTERVAL;
    if(currentPingInterval[0] == PING_INTERVAL)
    {      
      sendKeyValueToFirebase(messageKey, messageValue, dateTime);
    }
  }
  else if (messageKey.equals(KEY_HUMID))
  {
    memset(humidityMsg, 0, sizeof(humidityMsg));
    strcpy(humidityMsg, responseMsg);
    currentPingInterval[1] = currentPingInterval[1] > 0 ? currentPingInterval[1]-1 : PING_INTERVAL;
    if(currentPingInterval[1] == PING_INTERVAL)
    {      
      sendKeyValueToFirebase(messageKey, messageValue, dateTime);
    }
  }
  else if (messageKey.equals(KEY_LUMIN))
  {
    memset(luminosityMsg, 0, sizeof(luminosityMsg));
    strcpy(luminosityMsg, responseMsg);
    currentPingInterval[2] = currentPingInterval[2] > 0 ? currentPingInterval[2]-1 : PING_INTERVAL;
    if(currentPingInterval[2] == PING_INTERVAL)
    {      
      sendKeyValueToFirebase(messageKey, messageValue, dateTime);
    }
  }
  else if (messageKey.equals(KEY_SWITCH))
  {
    memset(switchStateMsg, 0, sizeof(switchStateMsg));
    strcpy(switchStateMsg, responseMsg);    
    messageValue.trim();
    isSwitchChanged = currentSwitchState != atoi(messageValue.c_str());
    currentSwitchState = atoi(messageValue.c_str());
    if(isSwitchChanged)
    {
      firebase.setInt(KEY_SWITCH, atoi(messageValue.c_str()));    
      sendKeyValueToFirebase(String("ActionHistory"), fullMessage, dateTime);  
    }
  }
  else
  {    
    sendKeyValueToFirebase(String("ActionHistory"), fullMessage, dateTime);
  }
}

void readFromSlave()
{
  // Fogadjuk az uzenetet az Arduinotol
  for (int i = 0; i < MSG_SIZE; i++)
  {
    responseMsg[i] = SPI.transfer(' ');
    delay(1);
  }

  handleMessageTypeFromSlave();
}

void handleGetTemperature()
{
  setRequestMsg("GET_TEMPERATURE=YES");
}

void handleGetHumidity()
{
  setRequestMsg("GET_HUMIDITY=YES");
}

void handleGetSwitchState()
{
  setRequestMsg("GET_SWITCH_STATE=YES");
}

void handleGetLuminosity()
{
  setRequestMsg("GET_LUMINOSITY=YES");
}

void handleIRLedSwitch(bool isSync)
{
  // Ha van parancs modosulas, akkor beallitjuk
  // Ez lesz majd elkuldve az Arduinonak
  if (isLedChanged)
  {
    if (isLedOn)
    {
      setRequestMsg("IR_LED_SWITCH=ON");
    }
    else
    {
      setRequestMsg("IR_LED_SWITCH=OFF");
    }
    
    if(isSync)
    {
      commandSlave();
    }
  }
}

void handleRotateStepper(bool isSync)
{
  // Ha van parancs modosulas, akkor beallitjuk
  // Ez lesz majd elkuldve az Arduinonak
  if (isStepperSwitchChanged)
  {
    if (isStepperOn)
    {
      setRequestMsg("ROTATE_STEPPER=ON");
    }
    else
    {
      setRequestMsg("ROTATE_STEPPER=OFF");
    }
    
    if(isSync)
    {
      commandSlave();
    }
  }
  
  if (isStepperDirectionChanged)
  {
    if (isStepperRight)
    {
      setRequestMsg("ROTATE_STEPPER=RIGHT");
    }
    else
    {
      setRequestMsg("ROTATE_STEPPER=LEFT");
    }
    
    if(isSync)
    {
      commandSlave();
    }
  }
}

void handleRelaySwitch(bool isSync)
{
}

void handleReadActions()
{
  actionCounter = (actionCounter < 3) ? actionCounter + 1 : 0;

  switch (actionCounter)
  {
  case 0:
  {
    handleGetTemperature();
    break;
  }
  case 1:
  {
    handleGetHumidity();
    break;
  }
  case 2:
  {
    handleGetSwitchState();
    break;
  }
  case 3:
  {
    handleGetLuminosity();
    break;
  }
  }
}

void handleAction(bool isSync)
{
  handleIRLedSwitch(isSync);
  handleRotateStepper(isSync);
  handleRelaySwitch(isSync);
}

void talkToSlave()
{
  Serial.println();
  Serial.print("req: ");
  Serial.println(requestMsg);
  
  SPI.beginTransaction(spi_settings);
  writeToSlave();
  delay(50);
  readFromSlave();
//  delay(100);
  SPI.endTransaction();

  Serial.print("resp: ");
  Serial.println(responseMsg);
    
  handleReadActions();
}


void commandSlave()
{
  Serial.println();
  Serial.print("req: ");
  Serial.println(requestMsg);
  
  SPI.beginTransaction(spi_settings);
//  delay(10);  
  writeToSlave();
  delay(50);
  readFromSlave();
//  delay(100);
  SPI.endTransaction();

  Serial.print("resp: ");
  Serial.println(responseMsg);
}

void setup()
{
  isLedOn = false;
  isLedChanged = false;

  Serial.begin(9600);
  SPI.begin();
  Serial.print("Server starting..");
  WiFi.begin(_SSID, _PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1);
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Use this URL to connect: http://");
  Serial.print(WiFi.localIP());

  setRequestMsg("GET_LUMINOSITY=YES");
}

void loop()
{
  talkToSlave();
  fireBaseActions();
  handleAction(true);

//  WiFiClient client = server.available();
//
//  if (client)
//  {
//    Serial.println("new client");
//    bool isClientAvailable = client.available();
//    while (!isClientAvailable)
//    {
//      isClientAvailable = client.available();
//
//      talkToSlave();
//      delay(50);
//    }
//
//    String request = client.readStringUntil('\r');
//    checkButtonPressed(request);
//
//    handleAction(false);
//
//    Serial.print("action: ");
//    Serial.println(requestMsg);
//
//    talkToSlave();
//
//    Serial.print("resp: ");
//    Serial.println(responseMsg);
//
//    // Uzenet megjelenitese a weboldalon
//    client.flush();
//    showUI(client);
//  }

  delay(100);
}

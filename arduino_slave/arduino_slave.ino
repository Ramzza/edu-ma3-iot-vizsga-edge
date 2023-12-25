
//IoT projekt minden részletet tartalmazó kivéve a Node MCUval a kommunikálást SPIn

#include <Wire.h>              //TWI I2C protokollhoz
#include <DS3231.h>            //ez a RTC real time clock egység
#include "TSL2561.h"           //fény szenzor könyvtára
#include <SimpleDHT.h>         //homerseklet es nedvesseg szenzornak
#include <LiquidCrystal_I2C.h> //I2C LCD kijelzo
#include <Stepper.h>

// begin Lab1
#include <SPI.h>
#include <string.h>

#define MSG_SIZE 100

#define KEY_TEMP "Temperature"
#define KEY_HUMID "Humidity"
#define KEY_LUMIN "Luminosity"
#define KEY_SWITCH "Switch_ON"

SPISettings spi_settings(100000, MSBFIRST, SPI_MODE0);

char requestMsg[MSG_SIZE + 1];
char responseMsg[MSG_SIZE + 1];

volatile byte spi_index;
volatile bool fogadunk;
volatile bool kuldunk;
volatile bool isMsgReceived;

DS3231 clock;   //objektum ami kezeli az RTC clockot
RTCDateTime dt; //idő amit kiolvas

TSL2561 tsl(TSL2561_ADDR_FLOAT); //objektum ami kezeli a fényszenzort TSL 2561

int pinDHT11 = A3; //nalunk az A3as pinen van a DHT11
SimpleDHT11 dht11(pinDHT11);

LiquidCrystal_I2C lcd(0x27, 20, 4); //0x27 cimen 20 oszlop 2 soros
                                    //0x3F 20,4 ha nagyobb mas LCD van itt

const int stepsPerRevolution = 200;
Stepper myStepper(stepsPerRevolution, 8, 6, 5, 4); //ezekre a PINekre kotottuk
                                                   //lasd a BOARD/ot amit terveztunk Eagleben

int switch_allapota; //a kapcsolo az A0-ra van kotve es INPUT PULLUP kell neki

uint16_t fenyero;
uint32_t lum;
uint16_t ir, full;

byte temperature;
byte humidity;

bool isStepperOn = false;
bool isStepperDirectionRight = true;

void readTemperatureAndHumidity()
{
  //DHT 11es szenzor
  dht11.read(&temperature, &humidity, NULL);

  lcd.setCursor(7, 1); //oszlop sor
  lcd.print((int)temperature);
  lcd.print("C ");
  lcd.print((int)humidity);
  lcd.print("%H");
}

String getTimeFromClock()
{
  //kiolvassuk az időt - RTC clock
  RTCDateTime currentDateTime = clock.getDateTime();

  lcd.setCursor(0, 0); //oszlop sor
  lcd.print((int)currentDateTime.hour);
  lcd.setCursor(2, 0);
  lcd.print(":");
  lcd.setCursor(3, 0);
  lcd.print((int)currentDateTime.minute);
  lcd.setCursor(5, 0);
  lcd.print(":");
  lcd.setCursor(6, 0);
  lcd.print((int)currentDateTime.second);

  String dateString = String(String(currentDateTime.year) + "-" + String(currentDateTime.month) + "-" + String(currentDateTime.day));
  String timeString = String(String(currentDateTime.hour) + ":" + String(currentDateTime.minute) + ":" + String(currentDateTime.second));
  String dateTimeString = String(dateString + " " + timeString);
  return dateTimeString;
}

void setResponseWithString(String resp)
{
  memset(responseMsg, 0, sizeof(responseMsg));
  strcpy(responseMsg, String(getTimeFromClock() + "->" + resp).c_str());
}

void handleGetTemperature()
{
  readTemperatureAndHumidity();
  setResponseWithString(String(String(KEY_TEMP) + "=" + String(temperature)));
  Serial.print("ResponseWithString: ");
  Serial.println(responseMsg);
}

void handleGetHumidity()
{
  readTemperatureAndHumidity();
  setResponseWithString(String(String(KEY_HUMID) + "=" + String(humidity)));
  Serial.print("ResponseWithString: ");
  Serial.println(responseMsg);
}

void handleGetSwitchState()
{
  //switch olvasasa
  switch_allapota = digitalRead(A0); //switch olvasása

  lcd.setCursor(9, 0); //oszlop sor
  lcd.print("Kapcs=");
  lcd.setCursor(15, 0);
  lcd.print((int)switch_allapota);

  setResponseWithString(String(String(KEY_SWITCH) + "=" + String(switch_allapota)));
  Serial.print("ResponseWithString: ");
  Serial.println(responseMsg);
}

void handleGetLuminosity()
{
  //TSL 2561 szenzor kiolvassuk a fenyerot
  fenyero = tsl.getLuminosity(TSL2561_VISIBLE);
  lum = tsl.getFullLuminosity();
  ir = lum >> 16;
  full = lum & 0xFFFF;
  int resultLux = (int)tsl.calculateLux(full, ir);

  lcd.setCursor(0, 1); //oszlop sor
  lcd.print(resultLux);
  lcd.print("lux  ");

  setResponseWithString(String(String(KEY_LUMIN) + "=" + String(resultLux)));
  Serial.print("ResponseWithString: ");
  Serial.println(responseMsg);
}

void handleIRLedSwitch(String cmd)
{
  //infravoros LEDen kommunikalni
  //7es PINen van
  if (cmd == "ON")
  {
    digitalWrite(7, HIGH);
  }

  if (cmd == "OFF")
  {
    digitalWrite(7, LOW);
  }

  setResponseWithString(String(String("IR Led") + "=" + cmd));
  Serial.print("ResponseWithString: ");
  Serial.println(responseMsg);
}

void handleRotateStepper(String cmd)
{
  //motor vezerles
  if (cmd == "ON")
  {
    isStepperOn = true;
  }
  else if (cmd == "OFF")
  {
    isStepperOn = false;
  }
  else if (cmd == "LEFT")
  {
    isStepperDirectionRight = false;
  }
  else if (cmd == "RIGHT")
  {
    isStepperDirectionRight = true;
  }

  setResponseWithString(String(String("Stepper") + "=" + cmd));
  Serial.print("ResponseWithString: ");
  Serial.println(responseMsg);
}

void handleRelaySwitch(String cmd)
{
  //relé kapcsolása
  digitalWrite(9, switch_allapota);
  Serial.print("ResponseWithString: ");
  Serial.println(responseMsg);
}

void handleCommand()
{
  if (NULL == strchr(requestMsg, '='))
  {
    // A nem "key=value" formaju uzenet nem erdekel
    return;
  }

  String cmd = String(requestMsg);
  cmd.trim();

  String key = cmd.substring(0, cmd.indexOf('='));
  String value = cmd.substring(cmd.indexOf('=') + 1, cmd.length());

  // Elvegezzuk a parancsnak megfelelo akciot
  if (key.equals("GET_TEMPERATURE"))
  {
    handleGetTemperature();
  }
  else if (key.equals("GET_HUMIDITY"))
  {
    handleGetHumidity();
  }
  else if (key.equals("GET_SWITCH_STATE"))
  {
    handleGetSwitchState();
  }
  else if (key.equals("GET_LUMINOSITY"))
  {
    handleGetLuminosity();
  }
  else if (key.equals("IR_LED_SWITCH"))
  {
    Serial.print("cmd: ");
    Serial.println(cmd);
    handleIRLedSwitch(value);
  }
  else if (key.equals("ROTATE_STEPPER"))
  {
    Serial.print("cmd: ");
    Serial.println(cmd);
    handleRotateStepper(value);
  }
  else if (key.equals("RELAY_SWITCH"))
  {
    Serial.print("cmd: ");
    Serial.println(cmd);
    handleRelaySwitch(value);
  }
}

void updateLcd()
{
  getTimeFromClock();
  readTemperatureAndHumidity();

  
  //switch olvasasa
  switch_allapota = digitalRead(A0); //switch olvasása

  lcd.setCursor(9, 0); //oszlop sor
  lcd.print("Kapcs=");
  lcd.setCursor(15, 0);
  lcd.print((int)switch_allapota);
  
  //TSL 2561 szenzor kiolvassuk a fenyerot
  fenyero = tsl.getLuminosity(TSL2561_VISIBLE);
  lum = tsl.getFullLuminosity();
  ir = lum >> 16;
  full = lum & 0xFFFF;
  int resultLux = (int)tsl.calculateLux(full, ir);

  lcd.setCursor(0, 1); //oszlop sor
  lcd.print(resultLux);
  lcd.print("lux  ");
}

void setup()
{
  // begin Lab1
  fogadunk = false;
  kuldunk = true;
  isMsgReceived = false;
  spi_index = 0;

  SPCR |= bit(SPE);
  pinMode(MISO, OUTPUT);

  SPI.attachInterrupt();

  pinMode(A0, INPUT_PULLUP); // ezen olvassuk a switchet
  pinMode(9, OUTPUT);        //ezen kapcsoljuk a relét
                             //(csak ha 12Val van táplálva az Arduino!!!)
  digitalWrite(9, LOW);      //kikapcs relé
  Serial.begin(9600);        //soros port init

  clock.setDateTime(__DATE__, __TIME__); //laptopról beírjuk az időt

  tsl.begin(); //inicializáljuk a fényszenzort
  tsl.setGain(TSL2561_GAIN_16X);
  tsl.setTiming(TSL2561_INTEGRATIONTIME_13MS);

  temperature = 0;
  humidity = 0;
  setResponseWithString("No requests yet...");

  lcd.init();
  lcd.backlight();

  //infravoros led a 7es PINen van
  pinMode(7, OUTPUT);
}

void loop()
{
  if (isMsgReceived)
  {
    // Vegrehajtuk a fogadott parancsot
    isMsgReceived = false;
    handleCommand();
  }

  if (isStepperOn && switch_allapota == 1)
  {
    if (isStepperDirectionRight)
    {
      myStepper.step(1);
    }
    else
    {
      myStepper.step(-1);
    }
  }
  
  updateLcd();
  delay(100);
}

ISR(SPI_STC_vect)
{
  char c = SPDR;

  if (spi_index == 0)
  {
    // Kuldes utan mindig fogadunk es forditva
    kuldunk = !kuldunk;
    fogadunk = !fogadunk;
  }

  if (fogadunk)
  {
    requestMsg[spi_index] = c;
  }

  if (kuldunk && spi_index < strlen(responseMsg))
  {
    SPDR = responseMsg[spi_index];
  }

  spi_index++;

  if (spi_index == MSG_SIZE)
  {
    // Kuldesi/fogadasi ciklus vegen nullazzuk a pointert
    isMsgReceived = fogadunk;
    spi_index = 0;
  }
}

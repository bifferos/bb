// Really dumb temperature/relay/lcd 'server'
//

#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS A1
#define RELAY_HIGH_PIN A2
#define RELAY_LOW_PIN A3

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 13, 9, 4, 5, 6, 7);



#define CMD_RELAY_HIGH '+'
#define CMD_RELAY_LOW  '-'
#define CMD_GET_TEMP   '?'
#define CMD_ROW_1      '<'
#define CMD_ROW_2      '>'



void setupRelayPin(int pin)
{
  digitalWrite(pin, LOW);
  pinMode(pin, OUTPUT);
}


void pulsePinOn(int pin)
{
  digitalWrite(pin, HIGH);
  delay(500);
  digitalWrite(pin, LOW);
}




void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  Serial.begin(9600);
  setupRelayPin(A2);
  setupRelayPin(A3);  
  lcd.write("Booting...");
}


// resets the bus every request.
void send_temperature()
{
  sensors.begin();
  oneWire.reset();
  lcd.setCursor(0, 0);
  sensors.requestTemperatures(); // Send the command to get temperatures
  float temp = sensors.getTempCByIndex(0); 
  //float temp = 19.99;
  Serial.print(temp);
  Serial.print("\n");
}




void loop() {
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  if (!Serial.available()) return;

  int byte = Serial.read();
  switch (byte)
  {
    case CMD_RELAY_HIGH:
      //digitalWrite(relayPin, HIGH);
      pulsePinOn(A2);
      break;
    case CMD_RELAY_LOW:
      //digitalWrite(relayPin, LOW);
      pulsePinOn(A3);
      break;
    case CMD_GET_TEMP:
      send_temperature();
      break;
    case CMD_ROW_1:
      lcd.setCursor(0, 0);
      break;
    case CMD_ROW_2:
      lcd.setCursor(0, 1);
      break;
    default:
      lcd.write(byte);
  }
  
}


#include <SoftwareSerial.h>

#define WIFI_NETWORK "your wifi name"
#define WIFI_PASSWORD "password"
#define HOST_ADDR "hostname.or.address"
#define HOST_PORT 8080 // port number

#define VERBOSE 0
#define WIFI_DISCONNECT 1
#define TCP_DISCONNECT 2
#define EXCHANGE 3

SoftwareSerial esp(7, 8); //ESP8266 PIN
int state = 0;  // ESP8266 state
int rev;        // data from hall effect sensor
int rpm;        // rev per minute
unsigned long prev_time; // previous time
int x = 0;      // PWM param
int dx = 0;     // PWM rise param
#define BUFF_SIZE 256
int MQ7[BUFF_SIZE];
unsigned long MQ7_sum = BUFF_SIZE * 100;
int MQ7_index = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("start");
  attachInterrupt(0, rpm_fun, RISING);
  rev = 0;
  rpm = 0;
  prev_time = 0;
  pinMode(3, OUTPUT);
  digitalWrite(2, HIGH);
  esp.begin(9600);
  state = VERBOSE;
}

void loop() {
  x += dx;
  if (x > 255) {
    x = 255;
    dx = 0;
  }
  else if (x < 0) {
    x = 0;
    dx = 0;
  }
  analogWrite(3, x);

  int MQ7_value = analogRead(A5);
  MQ7_sum -= MQ7[MQ7_index];
  MQ7[MQ7_index++] = MQ7_value;
  MQ7_sum += MQ7_value;
  if (MQ7_index == 256) {
    //Serial.println("update index");
    MQ7_index = 0;
  }
  
  if (state == VERBOSE) {
    Serial.println("Init");
    delay(1000);
    esp.println("AT");
    delay(1000);
  }
  if (esp.available()) {
    String inData = esp.readStringUntil('\r\n');
    Serial.print("ESP8266 [" + inData + "]");
    switch (state / 10) {
      case VERBOSE:
        state = WIFI_DISCONNECT * 10;
      case WIFI_DISCONNECT:
        wifi_conn(inData);
        break;
      case TCP_DISCONNECT:
        send_hello(inData);
        break;
      case EXCHANGE:
        exchange(inData);
        break;
    }
    Serial.println(" ");
  } else {
    if ((state / 10 >= TCP_DISCONNECT) && (millis() - prev_time > 999)) {
      send_msg(MQ7_sum/BUFF_SIZE, x);
      Serial.println(" ");
    }
  }
}

inline int check(String inData) {
  if (inData[0] == 'E') return -1; //Error
  if (inData[0] == 'O') return 1;  //OK
  if (inData[0] == 'b') return 2;  //bussy
  if (inData.startsWith("ALREADY", 0)) return 1;
  return 0;
}

void wifi_conn(String inData) {
  Serial.println("wifi start");
  switch (state % 10) {
    case 0:
      esp.println("AT");
      delay(100);
      state += 1;
      break;

    case 1:
      Serial.println(check(inData));
      esp.println("AT+CWMODE=1");
      delay(100);
      state += 1;

      break;

    case 2:
      Serial.println(check(inData));
      esp.println("AT+CWLAP");
      delay(100);
      state += 1;
      break;

    case 3:
      int res = check(inData);
      if (res == 1) state = TCP_DISCONNECT * 10;
      else if (res != 2) {
        esp.print("AT+CWJAP=\"");
        esp.print(WIFI_NETWORK);
        esp.print("\",\"");
        esp.print(WIFI_PASSWORD);
        esp.println("\"");
      }
  }
}


void send_hello(String inData) {
  switch (state % 10) {
    case 0:
      Serial.println(check(inData));
      esp.println("AT+CIPMUX=0");
      delay(100);
      state += 1;
      break;

    case 1:
      {
        int res = check(inData);
        if (res == 1) state += 1;
        else {
          if (res != 2) {
            esp.print("AT+CIPSTART=\"TCP\",\"");
            esp.print(HOST_ADDR);
            esp.print("\",");
            esp.println(HOST_PORT);
          }
          delay(700);
        }
        break;
      }

    case 2:
      esp.println("AT+CIPSEND=5");
      delay(30);
      esp.println("hello");
      state = EXCHANGE * 10;
  }
}

void calc_rpm(unsigned long diff_time) {
  detachInterrupt(0);
  rpm = 30000.0 / diff_time * rev;
  rev = 0;
  attachInterrupt(0, rpm_fun, RISING);
}

void rpm_fun() {
  rev++;
}

void send_msg(int mq7_, int x_) {
  char msg[128];
  unsigned long now_time = millis();
  unsigned long diff_time = now_time - prev_time;
  prev_time = now_time;
  calc_rpm(diff_time);
  sprintf(msg, "%d\t%d\t%d\t%lu",
          mq7_, x_, rpm, diff_time);
  Serial.println();
  Serial.print(msg);
  esp.print("AT+CIPSEND=");
  esp.println(strlen(msg));
  delay(30);
  esp.println(msg);
}

void exchange(String inData) {
  if (inData.startsWith("link is not", 0)) {
    state = TCP_DISCONNECT * 10;
  }
  //if (inData.startsWith("re
  if (inData.startsWith("+IPD", 0)) {
    int command = 0;
    String value;
    for (int i = 5; i < inData.length(); i++) {
      Serial.print(">"); Serial.print(inData[i]); Serial.println(isControl(inData[i]));
      if (inData[i] == '\t') {
        if (command == 0) {
          command = inData[i + 1] - '0';
        }
        else {
          value = inData.substring(i + 1, inData.length());
          break;
        }
      }
    }
    Serial.println(command);
    Serial.println("value = " + value);
    switch (command) {
      case 1:
        x = value.toInt() / 100.0 * 255;
        Serial.print("set x to = "); Serial.println(x);
        dx = 0;
        break;
      case 2:
        dx = value.toInt();
        Serial.print("set dx to "); Serial.println(dx);
        break;
      case 3:
        x = 0;
        dx = 250;
        break;
    }
  }
}


#define STRIP_PIN 8     // пин ленты
#define NUMLEDS 240      // кол-во светодиодов
#define COLOR_DEBTH 2
#include <microLED.h>   // подключаем библу
microLED<NUMLEDS, STRIP_PIN, MLED_NO_CLOCK, LED_WS2812, ORDER_GRB, CLI_AVER> strip;
uint8_t LDOCR_reg;
uint8_t CLKPR_reg;
uint8_t MCUSR_reg;
uint8_t SREG_reg;
#include <SPI.h>
#include "RF24.h"
#include <PMU.h> //либа питания
int Pstate = 0;

RF24 radio(6, 7); // "создать" модуль на пинах 9 и 10
byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};
uint32_t timer0, timer1, timer2, timer3;   //таймеры
float voltage = 4.2;            //Умолчания
bool ledo = 1;
int PUpin = 2;
int power_mesure = 1750;
static bool moto_en = 0;
static bool spd = 0;
static int ledb = 1;
int ledst = 0;
int leddir = 1;
int led_bri = 1;
static int bri = 0;
int led_color = 0;
uint16_t ledc;
uint16_t recieve_data[1]; // массив принятых данных

void setup() {
  ledc = mWhite;
  strip.setBrightness(0);
  pinMode(PUpin, OUTPUT);
  digitalWrite(PUpin, HIGH);
  pinMode(A5, OUTPUT);
  digitalWrite(A5, moto_en);
  pinMode(A6, OUTPUT);
  digitalWrite(A6, spd);
  pinMode(A0, INPUT); // вход с делителя
  pinMode(A3, OUTPUT);
  digitalWrite(A3, HIGH);
  analogReference(INTERNAL2V56);    //опорное
  //Serial.begin(57600); //открываем порт для связи с ПК
  //Serial.println("start_setup");
  radio.begin();               // активировать модуль
  radio.setAutoAck(1);         // режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);     // (время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    // разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);    // размер пакета, байт
  radio.openWritingPipe(address[0]);
  radio.openReadingPipe(1, address[0]);
  radio.setChannel(0x68);  //выбираем канал (в котором нет шумов!)
  radio.setPALevel (RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_1MBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //Serial.println("start_radio");
  radio.powerUp(); //начать работу
  radio.startListening();  //начинаем слушать эфир, мы приёмный модуль
  detachInterrupt(0);
  delay(200);
}

void loop() {
  byte pipeNo;
  if ( radio.available(&pipeNo)) {
    radio.read( &recieve_data, sizeof(recieve_data) );
    radio.writeAckPayload(pipeNo, &voltage, sizeof(voltage));
    // чиатем входящий сигнал
    if (recieve_data[0] == 0x1) { // затухание или загорание ленты
      //Serial.println(recieve_data[0]);
      recieve_data[0] = 0x0;
      ledo = !ledo;
      ledst = 1;
      timer0 = millis();
      //Serial.println(ledo);
      //Serial.println(ledst);
    }
    if (recieve_data[0] == 0x2) { // мерцание ленты
      //Serial.println(recieve_data[0]);
      recieve_data[0] = 0x0;
      ledst = 2;
      timer0 = millis();
      //Serial.println(ledst);
    }
    if (recieve_data[0] == 0x3) { // яркость ленты - перебор
      //Serial.println(recieve_data[0]);
      recieve_data[0] = 0x0;
      if (led_bri == 4) led_bri = 0;
      led_bri++;
      timer0 = millis();
      //Serial.println(ledb);
    }
    if (recieve_data[0] == 0x4) {       //  перебор цвета ленты
      //Serial.println(recieve_data[0]);
      recieve_data[0] = 0x0;
      led_color++;
      if (led_color == 4) led_color = 1;
      timer0 = millis();
      //Serial.println(ledc);
    }
    if (recieve_data[0] == 0x5) {   // Включить - выключить стрелки
      //Serial.println(recieve_data[0]);
      recieve_data[0] = 0x0;
      moto_en = !moto_en;
      digitalWrite(A5, moto_en);
      timer0 = millis();
      //Serial.println(moto_en);
    }
    if (recieve_data[0] == 0x6) {         //Инверсия движения
      //Serial.println(recieve_data[0]);
      recieve_data[0] = 0x0;
      spd = !spd;
      digitalWrite(A6, spd);
      timer0 = millis();
      //Serial.println(spd);
    }
    if (recieve_data[0] == 0x7) {         //((recieve_data[0] == 0x2) && (last_data[0] != 0x3) && (last_data[0] != 0x4))  { // резерв
      //Serial.println(recieve_data[0]);
      recieve_data[0] = 0x0;
      timer0 = millis();
    }
    if (recieve_data[0] == 0x8) {         //((recieve_data[0] == 0x2) && (last_data[0] != 0x3) && (last_data[0] != 0x4))  { // резерв
      //Serial.println(recieve_data[0]);
      recieve_data[0] = 0x0;
      timer0 = millis();
    }
  }

  switch (led_bri) {
    case 0:
      ledb = 0;
      // //Serial.println(ledb);
      break;
    case 1:
      ledb = 30;
      // //Serial.println(ledb);
      break;
    case 2:
      ledb = 70;
      ////Serial.println(ledb);
      break;
    case 3:
      ledb = 140;
      ////Serial.println(ledb);
      break;
    case 4:
      ledb = 200;
      ////Serial.println(ledb);
      break;
  }
  switch (led_color) {
    //  case 0:
    //   ledc = mBlack;
    //Serial.println(ledc);
    //  break;
    case 1:
      ledc = mBlue;
      //Serial.println(ledc);
      break;
    case 2:
      ledc = mRed;
      //Serial.println(ledc);
      break;
    case 3:
      ledc = mWhite;
      //Serial.println(ledc);
      break;
  }

  if (millis() - timer0 >= 3600000) {   // таймер на 30min выключения питания
    timer0 = millis();
    Pstate = 1;
  }
  if (millis() - timer1 >= 10) { // Таймер на исполнение светодиодной магии
    timer1 = millis();
    if (ledo == 0 && ledst == 1) {  //загорелся
      if (bri <= ledb) {
        strip.setBrightness(bri);
        bri++;
        ////Serial.println(bri);
      }
    }
    if (ledo == 1 && ledst == 1) {  //потух
      if (bri > 0) {
        strip.setBrightness(bri);

        bri--;
        // //Serial.println(bri);
      }
    }
    if (ledst == 2) {
      if (1 >= bri) leddir = 1;   //ТУДА-СЮДА
      if (ledb <= bri) leddir = -1;
      strip.setBrightness(bri);

      bri = bri + leddir;
      ////Serial.println(bri);
    }
    strip.fill(ledc);
    strip.show();
  }
  if (millis() - timer2 >= 30000) {   // таймер на 30 сек - замер напруги
    timer2 = millis();
    int power_mesure = analogRead(A0);
    voltage = power_mesure * 0.00232;
    //Serial.println (power_mesure);
    //Serial.println (voltage);
    if (voltage < 3) {
      Pstate = 1;
    }
  }
  switch (Pstate) {
    case 1:
      Pstate = 0;
      //Serial.println("timer"); //спать
      delay(100);
      LDOCR_reg = LDOCR;
      CLKPR_reg = CLKPR;
      MCUSR_reg = MCUSR;
      SREG_reg = SREG;
      digitalWrite(PUpin, LOW);
      radio.powerDown();
      pinMode(PUpin, INPUT);
      digitalWrite(A3, LOW);
      delay(500);
      pinMode(A3, INPUT);
      delay(500);
      attachInterrupt(0, wkp, HIGH);
      PMU.sleep(PM_POFFS1, SLEEP_FOREVER);
      break;
  }
}

void wkp() {    //проснись
  detachInterrupt(0);
  LDOCR = 0x80;
  LDOCR = LDOCR_reg;
  CLKPR = 0x80;
  CLKPR = CLKPR_reg;
  MCUSR = 0x80;
  MCUSR = MCUSR_reg;
  SREG = SREG_reg;
  //Serial.print("WKPstart");
  pinMode(PUpin, OUTPUT);
  digitalWrite(PUpin, HIGH);
  pinMode(A3, OUTPUT);
  digitalWrite(A3, HIGH);
  radio.powerUp(); //начать работу
  radio.startListening();  //начинаем слушать эфир, мы приёмный модуль
}

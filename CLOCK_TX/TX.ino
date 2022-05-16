// TX
// подключаем библу
#define STRIP_PIN 6     // пин ленты
#define NUMLEDS 1      // кол-во светодиодов
#define COLOR_DEBTH 2
#include <microLED.h>
microLED<NUMLEDS, STRIP_PIN, MLED_NO_CLOCK, LED_WS2812, ORDER_GRB, CLI_AVER> strip;

#define EB_CLICK 600
#include <EncButton.h>
#include <SPI.h>
#include "RF24.h"
#include "nRF24L01.h"
//#include "printf.h"
#include <GyverPower.h>


volatile boolean isrflag = true;
//void(* resetFunc) (void) = 0;
//void isr() {
//resetFunc();
//}
EncButton<EB_TICK, 2> btn0;
EncButton<EB_TICK, 3> btn1;
EncButton<EB_TICK, 4> btn2;
EncButton<EB_TICK, 5> btn3;

RF24 radio(9, 10); // "создать" модуль на пинах 9 и 10 Для Уно
byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; //возможные номера труб
uint16_t transmit_data[1]; // массив, хранящий передаваемые данные
//uint16_t latest_data[1]; // массив, хранящий последние переданные данные
boolean flag; // флажок отправки данных
uint32_t timer0, timer1;
double voltage_r = 3.49;
double voltage_t = 3.49;
int Pstate = 0;
int  pled = 7;

void setup() {
  Serial.begin(56700);
  analogReference(INTERNAL);
  pinMode(A0, INPUT);
  pinMode(pled, OUTPUT);
  digitalWrite(pled, LOW);

  Serial.println("led_power");
  strip.setBrightness(20);

  radio.begin();
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);    //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);     //размер пакета, в байтах
  radio.openWritingPipe(address[0]);   //мы - труба 0, открываем канал для передачи данных
  radio.setChannel(0x68);  //выбираем канал (в котором нет шумов!)

  radio.setPALevel (RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_1MBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!

  radio.powerUp(); //начать работу
  radio.stopListening();  //не слушаем радиоэфир, мы передатчик
 // printf_begin();
 // radio.printDetails();
  power.autoCalibrate(); // автоматическая калибровка ~ 2 секунды , средняя но достаточная точность
  // см раздел константы в GyverPower.h, разделяющий знак " | "
  power.setSleepMode(POWERDOWN_SLEEP); // если нужен другой режим сна, см константы в GyverPower.h (по умолчанию POWERDOWN_SLEEP)
  power.bodInSleep(false); // рекомендуется выключить bod во сне для сохранения энергии (по умолчанию false - выключен!!)

}

void loop() {
  btn0.tick();
  btn1.tick();
  btn2.tick();
  btn3.tick();
  if (isrflag > 0) {
    digitalWrite(pled, HIGH);
    Serial.println("isr1");
    delay(1200);
    int power_mesure = analogRead(A0);
    voltage_t = power_mesure * 0.0048;
    Serial.println(voltage_t);
    if (voltage_t <= 3.3) {
      strip.fill(mRed);
      strip.show();             //послали на ленту
      delay(1000);
      strip.fill(mBlack);
      strip.show();
    } else if (voltage_t < 3.7) {
      strip.fill(mYellow);
      strip.show();             //послали на ленту
      delay(1000);
      strip.fill(mBlack);
      strip.show();
    } else if (voltage_t > 3.7) {
      strip.fill(mGreen);
      strip.show();             //послали на ленту
      delay(1000);
      strip.fill(mBlack);
      strip.show();
    }
    Serial.println("FILL-1-ok");
    Serial.println("isr2");
    radio.powerUp(); // включить передатчик
    transmit_data[0] = 0x9;
    radio.write(&transmit_data, sizeof(transmit_data)); // отправить по радио
    if (!radio.available()) {                                  // если получаем пустой ответ
    } else {
      for (; radio.available();) {                   // если в ответе что-то есть
        radio.read(&voltage_r, sizeof(voltage_r));    // читаем
        // получили забитый данными массив  ответа от приёмника
      }
    }
    // если в ответе что-то есть читаем
    Serial.println(voltage_r);
    delay(100);
    // выключить передатчик
    if (voltage_r <= 3.45) {
      strip.fill(mRed);
      strip.show();             //послали на ленту
      delay(1000);
      strip.fill(mBlack);
      strip.show();
    } else if (voltage_r < 3.8) {
      strip.fill(mYellow);
      strip.show();             //послали на ленту
      delay(1000);
      strip.fill(mBlack);
      strip.show();
    } else if (voltage_r > 3.8) {
      strip.fill(mGreen);       //заливаем зеленым
      strip.show();             //послали на ленту
      delay(1000);
      strip.fill(mBlack);
      strip.show();
    }
    Serial.println("END_OF_isr2");
    delay(100);
    isrflag = false;
    detachInterrupt(0);
    radio.powerDown();
    digitalWrite(pled, LOW);
  }

  if (millis() - timer1 >= 60000) {
    int power_mesure = analogRead(A0);
    voltage_t = power_mesure * 0.0048;  // таймер на 10 сек - замер напруги
    if (voltage_r < 3.4) Pstate = 2;
    if (voltage_t < 3.1) Pstate = 3;
    Serial.println(voltage_t);
    timer1 = millis();
  }

  if (millis() - timer0 >= 1800000) {
    timer0 = millis();
    Pstate = 1;
  }

  switch (Pstate) {
    case 1:
      Pstate = 0;
      Serial.println("timer");
      delay(200);
      digitalWrite(pled, LOW);
      radio.powerDown();
      attachInterrupt(digitalPinToInterrupt(2), wkp, LOW);
      power.hardwareDisable(PWR_ADC | PWR_TIMER1);
      power.sleep(SLEEP_FOREVER);
      break;

    case 2:
      Pstate = 0;
      digitalWrite(pled, HIGH);
      Serial.println("rx low");
      strip.fill(mRed);       //заливаем зеленым
      strip.show();             //послали на ленту
      break;

    case 3:
      Pstate = 0;
      Serial.println("sleep");
      delay(200);
      digitalWrite(pled, LOW);
      radio.powerDown();
      attachInterrupt(digitalPinToInterrupt(2), wkp, LOW);
      power.hardwareDisable(PWR_ADC | PWR_TIMER1);
      power.sleep(SLEEP_FOREVER);
      break;
  }

  if (btn0.click()) {
    transmit_data[0] = 0x1;       // проверка на один клик
    flag = 1;
  }
  if (btn0.held()) {
    transmit_data[0] = 0x2;       // проверк удержания
    flag = 1;
  }
  if (btn1.click()) {
    transmit_data[0] = 0x3;       // проверка на один клик
    flag = 1;
  }
  if (btn1.held()) {
    transmit_data[0] = 0x4;       // проверк удержания
    flag = 1;
  }
  if (btn2.click()) {
    transmit_data[0] = 0x5;       // проверка на один клик
    flag = 1;
  }
  if (btn2.held()) {
    transmit_data[0] = 0x6;
    flag = 1;
  }
  if (btn3.click()) {
    transmit_data[0] = 0x7;       // проверка на один клик
    flag = 1;
  }
  if (btn3.held()) {
    transmit_data[0] = 0x8;       // проверк удержания
    flag = 1;
  }
  if (btn0.hasClicks(5)) wkp();

  /*for (int i = 0; i < 1; i++) { // в цикле от 0 до количества переменных
    if (transmit_data[i] != latest_data[i]) { // если есть изменения в transmit_data
      flag = 1; // поднять флаг отправки по радио
      latest_data[i] = transmit_data[i]; // запомнить последнее изменение
    }
    }*/

  if (flag == 1) {
    radio.powerUp(); // включить передатчик
    delay(1);
    radio.write(&transmit_data, sizeof(transmit_data));       // отправить по радио
    flag = 0; //опустить флаг
        Serial.println(transmit_data[0]);
    transmit_data[0] =0x0;
    if (radio.available() ) {                   // если в ответе что-то есть
      radio.read(&voltage_r, sizeof(voltage_r));    // читаем
      Serial.println(voltage_r);
      Serial.println("FLAG");
      timer0 = millis();
      radio.powerDown(); // выключить передатчик
    }

  }
}
void wkp() {
  power.hardwareEnable(PWR_ADC | PWR_TIMER1);
  digitalWrite(pled, HIGH);
  power.autoCalibrate();
  isrflag = true;
  Serial.print("WKPstart");
}

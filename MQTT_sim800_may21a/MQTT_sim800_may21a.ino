#include <SoftwareSerial.h>
#include "DHT.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display


SoftwareSerial SIM800(2, 3);        // для новых плат начиная с версии RX,TX
// #include <DallasTemperature.h>      // подключаем библиотеку чтения датчиков температуры
// OneWire oneWire(4);                 // и настраиваем  пин 4 как шину подключения датчиков DS18B20
// DallasTemperature sensors(&oneWire);
/*  ----------------------------------------- НАЗНАЧАЕМ ВЫВОДЫ АРДУИНО НА РАЗЛИЧНЫЕ НАПРАВЛЕНИЯ------------------------------   */

#define LED_Pin      13                     // на светодиод (моргалку)
#define DHTPIN 8 // номер пина, к которому подсоединен датчик
#define BUZ_Pin 9 // buzzer 1  


// Инициируем датчик

DHT dht(DHTPIN, DHT22);

/*  ----------------------------------------- НАСТРОЙКИ MQTT брокера---------------------------------------------------------   */
const char MQTT_user[10] = "jafvlskq";      // api.cloudmqtt.com > Details > User  
const char MQTT_pass[15] = "d4oqQHjyAcnH";  // api.cloudmqtt.com > Details > Password
const char MQTT_type[15] = "MQIsdp";        // тип протокола
const char MQTT_CID[15] = "TestSim800";        // уникальное имя устройства в сети MQTT
String MQTT_SERVER = "m23.cloudmqtt.com";   // api.cloudmqtt.com > Details > Server  сервер MQTT брокера
String PORT = "17271";                      // api.cloudmqtt.com > Details > Port    порт MQTT брокера
/*  ----------------------------------------- ИНДИВИДУАЛЬНЫЕ НАСТРОЙКИ !!!---------------------------------------------------------   */
String call_phone= "+79202505389";         // телефон входящего вызова  
String SMS_phone = "+79202505389";         // телефон куда отправляем СМС 
String APN = "internet";             // тчка доступа выхода в интернет вашего сотового оператора
String USER = "";                        // имя выхода в интернет вашего сотового оператора
String PASS = "";                        // пароль доступа выхода в интернет вашего сотового оператора

/*  ----------------------------------------- ДАЛЕЕ НЕ ТРОГАЕМ ---------------------------------------------------------------   */
String pin = "";
unsigned long Time1, Time2 = 0;
float t,h;
int Timer, inDS, count = 0;
int interval = 3;                           // интервал тправки данных на сервер после загрузки ардуино
bool ring = false;                          // флаг момента снятия трубки
bool broker = false;                        // статус подклюлючения к брокеру
bool Security = false;                      // состояние охраны после подачи питания

int ledState1 = LOW; // состояние светодиода
// последний момент времени, когда состояние светодиода изменялось
unsigned long previousMillis1 = 0;
long OnTime1 = 50; // длительность свечения светодиода (в миллисекундах)
long OffTime1 = 950; // светодиод не горит (в миллисекундах)

void setup() {
 // pinMode(RESET_Pin, OUTPUT);             // указываем пин на выход для перезагрузки модема
  pinMode(LED_Pin,     OUTPUT);             // указываем пин на выход (светодиод)
  pinMode(BUZ_Pin,     OUTPUT);
  delay(100); 
  dht.begin();
  Serial.begin(19200);                       //скорость порта
//  Serial.setTimeout(50);
  
  SIM800.begin(19200);                       //скорость связи с модемом
 // SIM800.setTimeout(500);                 // тайм аут ожидания ответа
  
  Serial.println("MQTT | 21/05/2018"); 
  delay (1000);
  SIM800_reset();

  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("MQTT  21/05/2018");
 
              }



void loop() {

if (SIM800.available())  resp_modem();                                    // если что-то пришло от SIM800 в Ардуино отправляем для разбора
if (Serial.available())  resp_serial();                                 // если что-то пришло от Ардуино отправляем в SIM800
if (millis()> Time1 + 10000) Time1 = millis(), detection();               // выполняем функцию detection () каждые 10 сек 
if (Security) blink();

}

void blink()
{
unsigned long currentMillis = millis(); // текущее время в миллисекундах

if((ledState1 == HIGH) && (currentMillis - previousMillis1 >= OnTime1))
 {
   ledState1 = LOW; // выключаем
   previousMillis1 = currentMillis; // запоминаем момент времени
   digitalWrite(LED_Pin, ledState1); // реализуем новое состояние
 }
 else if ((ledState1 == LOW) && (currentMillis - previousMillis1 >= OffTime1))
 {
   ledState1 = HIGH; // выключаем
   previousMillis1 = currentMillis ; // запоминаем момент времени
   digitalWrite(LED_Pin, ledState1); // реализуем новое состояние
 }


}

/*  --------------------------------------------------- Перезагрузка МОДЕМА SIM800L ------------------------------------------------ */ 
void SIM800_reset() {  
    SIM800.println("AT+CFUN=1,1");}   // програмная перезагрузка модема 

void callback(){                                                  // обратный звонок при появлении напряжения на входе IN1
    SIM800.println("ATD"+call_phone+";"),    delay(5000);} 

void detection(){                                                 // условия проверяемые каждые 10 сек  

    Serial.print("Интервал: "), Serial.println(interval);
  h = dht.readHumidity();  
  t = dht.readTemperature();
Serial.println((String)"Влажность: "+h+" %\t"+"Температура: "+t+" *C ");
  lcd.setCursor(0,1);
  lcd.println((String)"Temp:"+t+" Hum:"+h);
    interval--;
    if (interval <1) { interval = 6; 
        if (broker == true) { SIM800.println("AT+CIPSEND"), delay (200);  
                              MQTT_FloatPub ("C5/ds0",      t,2);
                              MQTT_FloatPub ("C5/ds1",      h,2);
                      //      MQTT_FloatPub ("C5/ds2",      TempDS[2],2);
                      //      MQTT_FloatPub ("C5/ds3",      TempDS[3],2);
                      //        MQTT_FloatPub ("C5/vbat",     Vbat,2);
                              MQTT_FloatPub ("C5/timer",    Timer,0);
                              MQTT_PUB      ("C5/security", Security ? "lock1" : "lock0");
                      //        MQTT_PUB      ("C5/engine",   heating ? "start" : "stop");
                      //        MQTT_FloatPub ("C5/engine",   heating,0);
                              MQTT_FloatPub ("C5/uptime",   millis()/3600000,0); 
                              SIM800.write(0x1A); 
                              
    } else  SIM800.println ("AT+SAPBR=3,1, \"Contype\",\"GPRS\""), delay (200);    // подключаемся к GPRS 
                     }  
}  



 
void resp_serial (){     // ---------------- ТРАНСЛИРУЕМ КОМАНДЫ из ПОРТА В МОДЕМ ----------------------------------
     String at = "";   
 //    while (Serial.available()) at = Serial.readString();
  int k = 0;
   while (Serial.available()) k = Serial.read(),at += char(k),delay(1);
     SIM800.println(at), at = "";   }   




void  MQTT_FloatPub (const char topic[15], float val, int x) { // топик, переменная в float, количество знаков после точки
           char st[10];
           dtostrf(val,0, x, st), MQTT_PUB (topic, st);      }

void MQTT_CONNECT () {
  SIM800.println("AT+CIPSEND"), delay (100);
     
  SIM800.write(0x10);                                                              // маркер пакета на установку соединения
  SIM800.write(strlen(MQTT_type)+strlen(MQTT_CID)+strlen(MQTT_user)+strlen(MQTT_pass)+12);
  SIM800.write((byte)0),SIM800.write(strlen(MQTT_type)),SIM800.write(MQTT_type);   // тип протокола
  SIM800.write(0x03), SIM800.write(0xC2),SIM800.write((byte)0),SIM800.write(0x3C); // просто так нужно
  SIM800.write((byte)0), SIM800.write(strlen(MQTT_CID)),  SIM800.write(MQTT_CID);  // MQTT  идентификатор устройства
  SIM800.write((byte)0), SIM800.write(strlen(MQTT_user)), SIM800.write(MQTT_user); // MQTT логин
  SIM800.write((byte)0), SIM800.write(strlen(MQTT_pass)), SIM800.write(MQTT_pass); // MQTT пароль

  MQTT_PUB ("C5/status", "Подключено");                                            // пакет публикации
  MQTT_SUB ("C5/comand");                                                          // пакет подписки на присылаемые команды
  MQTT_SUB ("C5/settimer");                                                        // пакет подписки на присылаемые значения таймера
  SIM800.write(0x1A),  broker = true;    }                                         // маркер завершения пакета

void  MQTT_PUB (const char MQTT_topic[15], const char MQTT_messege[15]) {          // пакет на публикацию

  SIM800.write(0x30), SIM800.write(strlen(MQTT_topic)+strlen(MQTT_messege)+2);
  SIM800.write((byte)0), SIM800.write(strlen(MQTT_topic)), SIM800.write(MQTT_topic); // топик
  SIM800.write(MQTT_messege);   }                                                  // сообщение

void  MQTT_SUB (const char MQTT_topic[15]) {                                       // пакет подписки на топик
  
  SIM800.write(0x82), SIM800.write(strlen(MQTT_topic)+5);                          // сумма пакета 
  SIM800.write((byte)0), SIM800.write(0x01), SIM800.write((byte)0);                // просто так нужно
  SIM800.write(strlen(MQTT_topic)), SIM800.write(MQTT_topic);                      // топик
  SIM800.write((byte)0);  }                          

void resp_modem (){     //------------------ АНЛИЗИРУЕМ БУФЕР ВИРТУАЛЬНОГО ПОРТА МОДЕМА------------------------------
     String at = "";
 //    while (SIM800.available()) at = SIM800.readString();  // набиваем в переменную at
  int k = 0;
   while (SIM800.available()) k = SIM800.read(),at += char(k),delay(1);           
   Serial.println(at);  
 
      if (at.indexOf("+CLIP: \""+call_phone+"\",") > -1) {delay(200), SIM800.println("ATA"), ring = true;
      } else if (at.indexOf("+DTMF: ")  > -1)        {String key = at.substring(at.indexOf("")+9, at.indexOf("")+10);
                                                     pin = pin + key;
                                                     if (pin.indexOf("*") > -1 ) pin= ""; 
      } else if (at.indexOf("SMS Ready") > -1 || at.indexOf("NO CARRIER") > -1 ) {SIM800.println("AT+CLIP=1;+DDET=1"); // Активируем АОН и декодер DTMF
/*  -------------------------------------- проверяем соеденеиние с ИНТЕРНЕТ, конектимся к серверу------------------------------------------------------- */
      } else if  (at.indexOf("AT+SAPBR=3,1, \"Contype\",\"GPRS\"\r\r\nOK") > -1 ) {SIM800.println("AT+SAPBR=3,1, \"APN\",\""+APN+"\""), delay (500); 
      } else if (at.indexOf("AT+SAPBR=3,1, \"APN\",\""+APN+"\"\r\r\nOK") > -1 )   {SIM800.println("AT+SAPBR=1,1"), delay (3000); // устанавливаем соеденение   
      } else if (at.indexOf("AT+SAPBR=1,1\r\r\nOK") > -1 )                        {SIM800.println("AT+SAPBR=2,1"), delay (1000); // проверяем статус соединения  
      } else if (at.indexOf("+SAPBR: 1,1") > -1 )    {delay (200),  SIM800.println("AT+CIPSTART=\"TCP\",\""+MQTT_SERVER+"\",\""+PORT+"\""), delay (1000);
      } else if (at.indexOf("ERROR") > -1 )    {broker = false, delay (50), SIM800.println("AT+CFUN=1,1"), delay (1000), interval = 6 ;

 
   } else if (at.indexOf("C5/comandlock1",4) > -1 )      {Security = 1 ;        // команда постановки на охрану       
                                                          tone (BUZ_Pin, 4000, 100);
   
   } else if (at.indexOf("C5/comandlock0",4) > -1 )      {Security = 0 ;        // команда снятия с хораны

                                                           tone (BUZ_Pin, 4000, 100);
                                                           delay (200);
                                                           tone (BUZ_Pin, 4000, 100);
   } else if (at.indexOf("C5/settimer",4) > -1 )         {Timer = at.substring(at.indexOf("")+15, at.indexOf("")+18).toInt();
//   } else if (at.indexOf("C5/comandstop",4) > -1 )       {heatingstop();     // команда остановки прогрева
//   } else if (at.indexOf("C5/comandstart",4) > -1 )      {enginestart();    // команда запуска прогрева
   } else if (at.indexOf("C5/comandRefresh",4) > -1 )    {// Serial.println ("Команда обнвления");
                                                          SIM800.println("AT+CIPSEND"), delay (200);  
                                                          MQTT_FloatPub ("C5/ds0",      t,2);
                                                          MQTT_FloatPub ("C5/ds1",      h,2);
                                                    //      MQTT_FloatPub ("C5/ds2",      TempDS[2],2);
                                                    //      MQTT_FloatPub ("C5/ds3",      TempDS[3],2);
                                                    //      MQTT_FloatPub ("C5/vbat",     Vbat,2);
                                                          MQTT_FloatPub ("C5/timer",    Timer,0);
                                                          MQTT_PUB      ("C5/security", Security ? "lock1" : "lock0");
                                                    //      MQTT_PUB      ("C5/engine",   heating ? "start" : "stop");
                                                          MQTT_FloatPub ("C5/uptime",   millis()/3600000,0); 
                                                          SIM800.write(0x1A); 
                                                          interval = 6; // швырнуть данные на сервер и ждать 60 сек
            
   } else if (at.indexOf("CONNECT OK\r\n") > -1 ) MQTT_CONNECT (); // после соединения с сервером отправляем пакет авторизации, публикации и пдписки у брокера
   at = "";                                                        // Возвращаем ответ можема в монитор порта , очищаем переменную

//       if (pin.indexOf("123") > -1 ){ pin= "", /* Voice(2),*/ enginestart();  
// } else if (pin.indexOf("789") > -1 ){ pin= "", /* Voice(10),*/ delay(1500), SIM800.println("ATH0"),heatingstop();  
// } else if (pin.indexOf("#")   > -1 ){ pin= "", SIM800.println("ATH0");    }
/*if (ring == true) { ring = false, delay (2000), pin= ""; // обнуляем пин
                    if (heating == false){ Voice(1);
                                    }else Voice(8); } */    
                               
 } 


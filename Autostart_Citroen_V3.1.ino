#include <SoftwareSerial.h>
SoftwareSerial SIM800(7, 6); // RX, TX  // для новых плат
#include <DallasTemperature.h>          // подключаем библиотеку чтения датчиков температуры
#define ONE_WIRE_BUS 2                  // и настраиваем  пин 4 как шину подключения датчиков DS18B20
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);

#define STARTER_Pin 8   // на реле стартера, через транзистор с 11-го пина ардуино
#define ON_Pin 9        // на реле зажигания, через транзистор с 12-го пина ардуино
#define ACTIV_Pin 13    // на светодиод и реле первого положения замка зажигания (для активации возбуждения генератора)
#define BAT_Pin A0      // на батарею, через делитель напряжения 39кОм / 11 кОм
#define Feedback_Pin A1 // на прикуриватель или на ДАДМ, для детектирования момента запуска
#define STOP_Pin A2     // на концевик педали тормоза для отключения режима прогрева
#define PSO_Pin A3      // на прочие датчики через делитель 39 kOhm / 11 kΩ
#define RESET_Pin 5     // пин перезагрузки
#define FIRST_P_Pin 11  // на реле первого положения замка зажигания

/*  ----------------------------------------- ИНДИВИДУАЛЬНЫЕ НАСТРОЙКИ !!!---------------------------------------------------------   */
String call_phone= "+375290000000"; // телефон входящего вызова  
String SMS_phone = "+375290000000"; // телефон куда отправляем СМС  s
String MAC = "80-01-AA-00-00-00";   // МАС-Адрес устройства для индентификации на сервере narodmon.ru (придумать свое 80-01-XX-XX-XX-XX-XX)
String SENS = "VasjaPupkin";        // Название устройства (придумать свое Citroen 566 или др. для отображения в программе и на карте)
String APN = "internet.mts.by";     // тчка доступа выхода в интернет вашего сотового оператора
String USER = "mts";                // имя выхода в интернет вашего сотового оператора
String PASS = "mts";                // пароль доступа выхода в интернет вашего сотового оператора
bool  n_send = true;                // отправка данных на народмон по умолчанию включена (true), отключена (false)
bool  sms_report = true;            // отправка SMS отчета по умолчанию овключена (true), отключена (false)
float Vstart = 13.50;               // поорог распознавания момента запуска по напряжению
/*  ----------------------------------------- ДАЛЕЕ НЕ ТРОГАЕМ --------------------------------------------------------------------   */
String SERVER = "narodmon.ru";     // сервер народмона (на октябрь 2017) 
String PORT = "8283";              // порт народмона   (на октябрь 2017) 
float TempDS0, TempDS1, TempDS2 ;  // переменные хранения температуры
int k = 0;
int count = 0;                     // количество совершенных попыток старnа с последнего запуска
String  pin= "";                   // строковая переменная набираемого пинкода
int interval = 10;                  // интервал отправки данных на народмон 
String at = "";
unsigned long Time1 = 0; 
int Timer = 0;                     // таймер времени прогрева двигателя по умолчанию = 0
int Timer2 = 1080;                 // часовой таймер (60 sec. x 60 min. / 10 = 360 )
int beep =0;                       // количество пропущенных гудков
bool heating = false;              // переменная состояния режим прогрева двигателя
bool SMS_send = false;             // флаг разовой отправки СМС
float Vbat;                        // переменная хранящая напряжение бортовой сети
float m = 66.91;                   // делитель для перевода АЦП в вольты для резистров 39/11kOm

void setup() {
  pinMode(ACTIV_Pin,   OUTPUT);    // указываем пин на выход (светодиод)
  pinMode(ON_Pin,      OUTPUT);    // указываем пин на выход доп реле зажигания
  pinMode(STARTER_Pin, OUTPUT);    // указываем пин на выход доп реле стартера
  pinMode(RESET_Pin,   OUTPUT);    // указываем пин на выход для перезагрузки модуля
  pinMode(FIRST_P_Pin, OUTPUT);    // указываем пин на выход для доп реле первого положения замка зажигания
  Serial.begin(9600);              //скорость порта
  SIM800.begin(9600);              //скорость связи с модемом
  Serial.println("Starting. | V3.2 | SIM800L+narodmon.ru. | MAC:"+MAC+" | NAME:"+SENS+" | APN:"+APN+" | TEL:"+call_phone+" | 09/11/2017"); 
  delay (2000);
  SIM800_reset();
             }

void SIM800_reset() {                                         // Call Ready
/*  --------------------------------------------------- ПРЕДНАСТРОЙКА МОДЕМА SIM800L ------------------------------------------------------------ */   
// digitalWrite(RESET_Pin, LOW),delay(2000);
// digitalWrite(RESET_Pin, HIGH), delay(5000);
// SIM800.println("AT+CFUN=1,1");
  SIM800.println("ATE1"),                    delay(50);        // отключаем режим ЭХА
  SIM800.println("AT+CLIP=1"),               delay (50);       // включаем определитель номера
  SIM800.println("AT+DDET=1"),               delay (50);       // включаем DTMF декдер
  SIM800.println("AT+CMGF=1"),               delay (50);      // Включаем Текстовый режим СМС
  SIM800.println("AT+CSCS=\"gsm\""),         delay (50);      // Выбираем кодировку СМС
  SIM800.println("AT+CNMI=2,1,0,0,0") ,      delay (100);      // Разрешаем прием входящих СМС
  SIM800.println("AT+DDET=1"),               delay (50);       // включаем DTMF декдер
  SIM800.println("AT+VTD=1"),                delay (50);       // Задаем длительность DTMF ответа
  SIM800.println("AT+CMEE=1"),               delay (50);       // Инфо о ошибках 
  SIM800.println("AT+CMGDA=\"DEL ALL\""),    delay (300);      // Удаляем все СМС полученные ранее
                   } 

void loop() {
  if (SIM800.available()) {                                               // если что-то пришло от SIM800 в SoftwareSerial Ардуино
    while (SIM800.available()) k = SIM800.read(), at += char(k),delay(1); // набиваем в переменную at
    Serial.println(at);                                                   // Возвращаем ответ можема в монитор порта
    
    if (at.indexOf("+CLIP: \""+call_phone+"\",") > -1 && at.indexOf("+CMGR:") == -1 ) {
                                                     SIM800.println("ATA"), delay(100);
                                                     SIM800.println("AT+VTS=\"3,5,7\"");
                                                     pin= "";
            
    } else if (at.indexOf("+CLIP: \""+SMS_phone+"\",") > -1 && at.indexOf("+CMGR:") == -1 ) {
                                                     delay(50), SIM800.println("ATH0");
                                                     Timer = 60, enginestart();
            
     } else if (at.indexOf("+DTMF: ")  > -1)        {String key = at.substring(at.indexOf("+DTMF: ")+7, at.indexOf("+DTMF: ")+8);
                                                     pin = pin + key;
                                                     if (pin.indexOf("*") > -1 ) pin= "", SIM800.println ("AT+VTS=\"9,7,9\""); 
                                                 Serial.print (" Starting pin:"), Serial.println(pin);        

                 
      } else if (at.indexOf("+CMTI: \"SM\",") > -1) {int i = at.substring(at.indexOf("\"SM\",")+5, at.indexOf("\"SM\",")+6).toInt();
                                                 //  delay(2000), SIM800.println("AT+CMGR="+i+""); // читаем СМС   
                                                     delay(2000), SIM800.print("AT+CMGR="), SIM800.println(i); // читаем СМС 
//    верменный баг с чтением смс                   // delay(20), SIM800.println("AT+CMGDA=\"DEL ALL\""), delay(20); //  и удаляем их все
      
      } else if (at.indexOf("123start10") > -1 )      {Timer = 60, enginestart();
      } else if (at.indexOf("123start20") > -1 )      {Timer = 120, enginestart();
      } else if (at.indexOf("123stop") > -1 )         {heatingstop();     
   //   } else if (at.indexOf("narodmon=off") > -1 )    {n_send = false;  
   //   } else if (at.indexOf("narodmon=on") > -1 )     {n_send = true;  
   //   } else if (at.indexOf("sms=off") > -1 )         {sms_report = false;  
   //   } else if (at.indexOf("sms=on") > -1 )          {sms_report = true;     
    } else if (at.indexOf("SMS Ready") > -1 )         {SIM800_reset();       

    /*  -------------------------------------- ВХОДИМ В ИНТЕРНЕТ И ОТПРАВЛЯЕМ ПАКЕТ ДАННЫХ НА СЕРВЕР--------------------------------------------------   */
    } else if (at.indexOf("AT+CGATT=1\r\r\nOK\r\n") > -1 )         {SIM800.println("AT+CSTT=\""+APN+"\""),     delay (500);   // установливаем точку доступа  
    } else if (at.indexOf("AT+CSTT=\""+APN+"\"\r\r\nOK\r\n") > -1 ){SIM800.println("AT+CIICR"),                delay (2000);  // устанавливаем соеденение   
    } else if (at.indexOf("AT+CIICR") > -1 )                       {SIM800.println("AT+CIFSR"),                delay (1000);  // возвращаем IP-адрес модуля    
    } else if (at.indexOf("AT+CIFSR") > -1 )                       {SIM800.println("AT+CIPSTART=\"TCP\",\""+SERVER+"\",\""+PORT+"\""), delay (1000);    
    } else if (at.indexOf("CONNECT OK\r\n") > -1 )                 {SIM800.println("AT+CIPSEND"),              delay (1200) ;  // меняем статус модема
    } else if (at.indexOf("AT+CIPSEND\r\r\n>") > -1 )              {  // "набиваем" пакет данными 
                         SIM800.print("#" +MAC+ "#" +SENS);           // заголовок пакета с MAC адресом и именем устройства      
                         if (TempDS0 > -40 && TempDS0 < 54)        SIM800.print("\n#Temp1#"), SIM800.print(TempDS0); // Температура с датчика №1      
                         if (TempDS1 > -40 && TempDS1 < 54)        SIM800.print("\n#Temp2#"), SIM800.print(TempDS1); // Температура с датчика №2      
                         if (TempDS2 > -40 && TempDS2 < 54)        SIM800.print("\n#Temp3#"), SIM800.print(TempDS2); // Температура с датчика №3      
                         SIM800.print("\n#Vbat#"),                 SIM800.print(Vbat);                               // Напряжение аккумулятора
                         SIM800.print("\n#Uptime#"),               SIM800.print(millis()/1000);                      // Время непрерывной работы
                         SIM800.println("\n##");                                                              // закрываем пакет
                         SIM800.println((char)26), delay (100);                                               // отправляем пакет
     } 
at = "";  // очищаем переменную

}
/*  --------------------------------------------------- НЕПРЕРЫВНАЯ ПРОВЕРКА СОБЫТИЙ ------------------------------------------------------------ */   

     if (Serial.available()) {             //если в мониторе порта ввели что-то
    while (Serial.available()) k = Serial.read(), at += char(k), delay(1);
    SIM800.println(at),at = "";  //очищаем
                         }

if (pin.indexOf("123") > -1 ) pin= "", SIM800.println ("AT+VTS=\"1,2,3\""), Serial.println (" Starting 10 min "), enginestart(),Timer = 60 ;  
if (pin.indexOf("456") > -1 ) pin= "", SIM800.println ("AT+VTS=\"4,5,6\""), Serial.println (" Starting 20 min "), enginestart(),Timer = 120;  
if (pin.indexOf("789") > -1 ) pin= "", SIM800.println ("AT+VTS=\"A,B,D\""), Serial.println (" Stop "), heatingstop();  
if (pin.indexOf("#")   > -1 ) pin= "", SIM800.println("ATH0"), SMS_send = true;        
if (millis()> Time1 + 10000) detection(), Time1 = millis();       // выполняем функцию detection () каждые 10 сек 
if (heating == true && digitalRead(STOP_Pin)==1) heatingstop();   //если нажали на педаль тормоза в режиме прогрева

}

void detection(){                           // условия проверяемые каждые 10 сек  
    sensors.requestTemperatures();          // читаем температуру с трех датчиков
    TempDS0 = sensors.getTempCByIndex(0);   // датчик на двигатель
    TempDS1 = sensors.getTempCByIndex(1);   // датчик в салон
    TempDS2 = sensors.getTempCByIndex(2);   // датчик на улицу 
    Vbat = analogRead(BAT_Pin);             // замеряем напряжение на батарее
    Vbat = Vbat / m ;                       // переводим попугаи в вольты
/* Serial.print("Vbat= "),Serial.print(Vbat), Serial.print (" V.");  
    Serial.print(" || Temp1 : "), Serial.print(TempDS0);
    Serial.print(" || Temp2 : "), Serial.print(TempDS1);
    Serial.print(" || Temp3: "),  Serial.print(TempDS2);  
    Serial.print(" || Interval : "), Serial.print(interval);
    Serial.print(" || Timer ="), Serial.println (Timer);
       
 */
  
    if (SMS_send == true && sms_report == true) {              // если фаг SMS_send равен 1 высылаем отчет по СМС
        SIM800.println("AT+CMGS=\""+SMS_phone+"\""), delay(100);
        SIM800.print("Privet "+SENS+".!");
        SIM800.print("\n Temp.Dvig: "),  SIM800.print(TempDS0);
        SIM800.print("\n Temp.Salon: "), SIM800.print(TempDS1);
        SIM800.print("\n Temp.Ulica: "), SIM800.print(TempDS2); 
        SIM800.print("\n Uptime: "), SIM800.print(millis()/3600000), SIM800.print(" H.");
        SIM800.print("\n Vbat: "), SIM800.print(Vbat),SIM800.print("V.");
        if (n_send == true) SIM800.print("\n narodmon.ru ON ");
        if (digitalRead(Feedback_Pin) == HIGH) SIM800.print("\n Zahiganie ON ");
        SIM800.print("\n Popytok:"), SIM800.print(count);
        SIM800.print((char)26), SMS_send = false;
              }
   
    if (heating == true && Timer == 30 ) SMS_send = true; 
    if (Timer > 0 ) Timer--;    // если таймер больше ноля  SMS_send = true;
    if (heating == true && Timer <1) Serial.println("End timer"), heatingstop(); 
    if (heating == true && Vbat < 11.0) Serial.println("Low voltage"), heatingstop(); 
  
     //  Автозапуск при понижении температуры ниже -18 градусов, при -25 смс оповещение каждых 3 часа
    if (Timer2 == 2 && TempDS0 < -18 && TempDS0 != -127) Timer2 = 1080, Timer = 120, enginestart();  
    if (Timer2 == 1 && TempDS0 < -25 && TempDS0 != -127) Timer2 = 1080, SMS_send = true;    
        Timer2--;                                                 // вычитаем еденицу
    if (Timer2 < 0) Timer2 = 1080;                                // продлеваем таймер на 3 часа (60x60x3/10 = 1080)
 
   // if (heating == false) digitalWrite(ACTIV_Pin, HIGH), delay (50), digitalWrite(ACTIV_Pin, LOW);  // моргнем светодиодом
    interval--;
    if (interval <1 && n_send == true) interval = 30, SIM800.println ("AT+CGATT=1"), delay (200);    // подключаемся к GPRS 
    if (interval == 28 && n_send == true ) SIM800.println ("AT+CIPSHUT");    
             
}             

void enginestart() {                                      // программа запуска двигателя
 /*  ----------------------------------------- ПРЕДНАСТРОЙКА ПЕРЕД ЗАПУСКОМ -----------------------------------------------------------------*/
count = 0;                                                // переменная хранящая число попыток запуска
int StarterTime = 1400;                                   // переменная хранения времени работы стартера (1,4 сек. для первой попытки)  
if (TempDS0 < 15 && TempDS0 != -127)  StarterTime = 1200, count = 2;   // при 15 градусах крутим 1.2 сек 2 попытки 
if (TempDS0 < 5  && TempDS0 != -127)  StarterTime = 1800, count = 2;   // при 5  градусах крутим 1.8 сек 2 попытки 
if (TempDS0 < -5 && TempDS0 != -127)  StarterTime = 2200, count = 3;   // при -5 градусах крутим 2.2 сек 3 попытки 
if (TempDS0 <-10 && TempDS0 != -127)  StarterTime = 3300, count = 4;   //при -10 градусах крутим 3.3 сек 4 попытки 
if (TempDS0 <-15 && TempDS0 != -127)  StarterTime = 6000, count = 5;   //при -15 градусах крутим 6.0 сек 5 попыток 
if (TempDS0 <-30 && TempDS0 != -127)  StarterTime = 1,count = 10, SMS_send = true;   //при -20 отправляем СМС 

// если напряжение АКБ больше 10 вольт, зажигание выключено, счетчик числа попыток  меньше 5
 while (Vbat > 10.00 && digitalRead(Feedback_Pin) == LOW && count < 5){ 
   
Serial.print("count="), Serial.print(count),Serial.print(" | StarterTime ="),Serial.print(StarterTime);

    digitalWrite(ON_Pin, LOW),       delay (2000);        // выключаем зажигание на 2 сек. на всякий случай   
    digitalWrite(FIRST_P_Pin, HIGH), delay (500);         // включаем реле первого положения замка зажигания 
    digitalWrite(ON_Pin, HIGH),      delay (4000);        // включаем зажигание  и ждем 4 сек.

// если уже была одна неудачная попытк запуска то прогреваем свечи накаливания 8 сек
 if (count > 2) digitalWrite(ON_Pin, LOW),  delay (2000), digitalWrite(ON_Pin, HIGH), delay (8000);
 
// если уже более чем 3 неудачных попытк запуска то прогреваем свечи накаливания 8 сек дполнительно  
 if (count > 4) digitalWrite(ON_Pin, LOW), delay (10000), digitalWrite(ON_Pin, HIGH), delay (8000);

// если не нажата педаль тормоза то включаем реле стартера на время StarterTime
    if (digitalRead(STOP_Pin) == LOW) digitalWrite(STARTER_Pin, HIGH); // включаем реле стартера
    delay (StarterTime);                                  // выдерживаем время StarterTime
    digitalWrite(STARTER_Pin, LOW),   delay (6000);       // отключаем реле, выжидаем 6 сек.

// замеряем напряжение АКБ 3 раза и высчитываем среднее арифмитическое 3х замеров    
    Vbat =        analogRead(BAT_Pin), delay (300);       // замеряем напряжение АКБ 1 раз
    Vbat = Vbat + analogRead(BAT_Pin), delay (300);       // через 0.3 сек.  2-й раз 
    Vbat = Vbat + analogRead(BAT_Pin), delay (300);       // через 0.3 сек.  3-й раз
    Vbat = Vbat / m / 3 ;                                 // переводим попугаи в вольты и плучаем срееднне 3-х замеров

// пределяем запустил ся ли двигатель     
     // if (digitalRead(DDM_Pin) != LOW)                  // если детектировать по датчику давления масла /-А-/
     // if (digitalRead(DDM_Pin) != HIGH)                 // если детектировать по датчику давления масла /-В-/
     if (Vbat > Vstart) {                                 // если детектировать по напряжению зарядки     /-С-/
        Serial.print (" -> Vbat > Vstart = ");
        Serial.println(Vbat); 
        heating = true, digitalWrite(ACTIV_Pin, HIGH);    // зажжом светодиод
        SIM800.println("ATH0");                           // вешаем трубку
        break;                                            // считаем старт успешным, выхдим из цикла запуска двигателя
                        }
                        
        else { // если статра нет вертимся в цикле пока 
        Serial.print (" - > Vbat < Vstart = "), Serial.println(Vbat); 
        StarterTime = StarterTime + 200;                      // увеличиваем время следующего старта на 0.2 сек.
        count++;                                              // уменьшаем на еденицу число оставшихся потыток запуска
        heatingstop();
             }
      }
  Serial.println (" out >>>>> enginestart()");
 // if (digitalRead(Feedback_Pin) == LOW) SMS_send = true;   // отправляем смс СРАЗУ только в случае незапуска
  
 }


void heatingstop() {  // программа остановки прогрева двигателя
    digitalWrite(ON_Pin, LOW),      delay (200);
    digitalWrite(ACTIV_Pin, LOW),   delay (200);
    digitalWrite(FIRST_P_Pin, LOW), delay (200);
    Serial.println ("Ignition OFF"),
    heating= false,                 delay(3000); 
                   }

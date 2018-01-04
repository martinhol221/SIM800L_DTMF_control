#include <SoftwareSerial.h>
SoftwareSerial SIM800(7, 6);        // для новых плат начиная с версии RX,TX
#include <DallasTemperature.h>      // подключаем библиотеку чтения датчиков температуры
OneWire oneWire(4);                 // и настраиваем  пин 4 как шину подключения датчиков DS18B20
DallasTemperature sensors(&oneWire);

#define FIRST_P_Pin  8              // на реле первого положения замка зажигания
#define SECOND_P     9              // на реле зажигания, через транзистор с 9-го пина ардуино
#define STARTER_Pin  12             // на реле стартера, через транзистор с 12-го пина ардуино
#define WEBASTO_Pin  11             // на реле вебасто или подогрева седушек
#define LED_Pin      13             // на светодиод (моргалку)
#define BAT_Pin      A0             // на батарею, через делитель напряжения 39кОм / 11 кОм
#define Feedback_Pin A1             // на провод от замка зажигания
#define STOP_Pin     A2             // на концевик педали тормоза для отключения режима прогрева
#define PSO_Pin      A3             // на прочие датчики через делитель 39 kOhm / 11 kΩ
#define REL_Pin      10             // на дополнительное реле
#define RESET_Pin    5

/*  ----------------------------------------- ИНДИВИДУАЛЬНЫЕ НАСТРОЙКИ !!!---------------------------------------------------------   */
// String LAT = "";                    // переменная храняжая широту 
// String LNG = "";                    // переменная храняжая долготу 
String call_phone= "+375290000000"; // телефон входящего вызова  
String SMS_phone = "+375290000000"; // телефон куда отправляем СМС 
String MAC = "80-01-AA-00-00-00";   // МАС-Адрес устройства для индентификации на сервере narodmon.ru (придумать свое 80-01-XX-XX-XX-XX-XX)
String SENS = "VasjaPupkin";       // Название устройства (придумать свое Citroen 566 или др. для отображения в программе и на карте)
String APN = "internet.mts.by";    // тчка доступа выхода в интернет вашего сотового оператора
String USER = "mts";               // имя выхода в интернет вашего сотового оператора
String PASS = "mts";               // пароль доступа выхода в интернет вашего сотового оператора
bool  n_send = true;               // отправка данных на народмон по умолчанию включена (true), отключена (false)
bool  sms_report = true;           // отправка SMS отчета по умолчанию овключена (true), отключена (false)
float Vstart = 13.50;              // поорог распознавания момента запуска по напряжению
String SERVER = "narodmon.ru";     // сервер народмона (на октябрь 2017) 
String PORT = "8283";              // порт народмона   (на октябрь 2017) 
String pin = "";                   // строковая переменная набираемого пинкода 
float TempDS[11];                  // массив хранения температуры c рахных датчиков 
float Vbat, VbatStart, V_min ;     // переменная хранящая напряжение бортовой сети
float m = 69.91;                   // делитель для перевода АЦП в вольты для резистров 39/11kOm
unsigned long Time1, StarterTimeON = 0;
int inDS, count = 0;
int interval = 5;                  // интервал отправки данных на народмон сразу после загрузки 
int Timer = 0;                     // таймер времени прогрева двигателя по умолчанию = 0
int Timer2 = 1080;                 // часовой таймер (60 sec. x 60 min. / 10 = 360 )
bool heating = false;              // переменная состояния режим прогрева двигателя
bool SMS_send = false;             // флаг разовой отправки СМС
bool ring = false;                 // флаг момента снятия трубки



void setup() {
  pinMode(LED_Pin,     OUTPUT);    // указываем пин на выход (светодиод)
  pinMode(SECOND_P,    OUTPUT);    // указываем пин на выход доп реле зажигания
  pinMode(STARTER_Pin, OUTPUT);    // указываем пин на выход доп реле стартера
  pinMode(REL_Pin,     OUTPUT);    // указываем пин на выход для доп нужд.
  pinMode(FIRST_P_Pin, OUTPUT);    // указываем пин на выход для доп реле первого положения замка зажигания
  pinMode(WEBASTO_Pin, OUTPUT);    // указываем пин на выход для доп реле вебасто
 // pinMode(RESET_Pin, OUTPUT);      // указываем пин на выход для перезагрузки модема
  pinMode(3, INPUT_PULLUP);        // указываем пин на вход для с внутричипной подтяжкой к +3.3V
  pinMode(2, INPUT_PULLUP);        // указываем пин на вход для с внутричипной подтяжкой к +3.3V
  delay(100); 
  Serial.begin(9600);              //скорость порта
//  Serial.setTimeout(50);
  
  SIM800.begin(9600);              //скорость связи с модемом
 // SIM800.setTimeout(500);          // тайм аут ожидания ответа
  
  Serial.println("Загрузка v.4.8| MAC:"+MAC+" | TEL:"+call_phone+" | 04/01/2018"); 
  SIM800_reset();
 // attachInterrupt(0, callback, FALLING);  // включаем прерывание при переходе 1 -> 0 на D2, или 0 -> 1 на ножке оптопары
 // attachInterrupt(1, callback, FALLING);  // включаем прерывание при переходе 1 -> 0 на D3, или 0 -> 1 на ножке оптопары
             }
/*  --------------------------------------------------- Перезагрузка МОДЕМА SIM800L ------------------------------------------------ */ 
void SIM800_reset() {  
 delay(1000); SIM800.println("AT+CLIP=1;+DDET=1");
//SIM800.println("AT+IPR=9600;E1+CMGF=1;+CSCS=\"gsm\";+CNMI=2,1,0,0,0;+VTD=1;+CMEE=1;&W"); 
//digitalWrite(RESET_Pin, LOW),delay(2000), digitalWrite(RESET_Pin, HIGH), delay(5000);
}
             
void callback(){                                                  // обратный звонок при появлении напряжения на входе IN1
    SIM800.println("ATD"+call_phone+";"),    delay(5000);} 
             
void loop() {

if (SIM800.available())  resp_modem();                            // если что-то пришло от SIM800 в Ардуино отправляем для разбора
if (Serial.available())  resp_serial();                           // если что-то пришло от Ардуино отправляем в SIM800
if (millis()> Time1 + 10000) Time1 = millis(), detection();       // выполняем функцию detection () каждые 10 сек 
if (heating == true && digitalRead(STOP_Pin)==1) heatingstop();   // если нажали на педаль тормоза в режиме прогрева
            }

void detection(){                                                 // условия проверяемые каждые 10 сек  
    
    Vbat = VoltRead();                                            // замеряем напряжение на батарее
    Serial.print("|int: "), Serial.print(interval);
    Serial.print("||Timer:"), Serial.println (Timer);
    
    inDS = 0;
    sensors.requestTemperatures();                                // читаем температуру с трех датчиков
    while (inDS < 10){
          TempDS[inDS] = sensors.getTempCByIndex(inDS);           // читаем температуру
      if (TempDS[inDS] == -127.00){TempDS[inDS]= 80;
                                   break; }                       // пока не доберемся до неподключенного датчика
              inDS++;} 
              
    for (int i=0; i < inDS; i++) Serial.print("Temp"), Serial.print(i), Serial.print("= "), Serial.println(TempDS[i]); 
    
    Serial.println ("");

  
    if (SMS_send == true && sms_report == true) { SMS_send = false;          // если фаг SMS_send равен 1 высылаем отчет по СМС
        SIM800.println("AT+CMGS=\""+SMS_phone+"\""), delay(100);
        SIM800.print("Privet "+SENS+".!");
        for (int i=0; i < inDS; i++)          SIM800.print("\n Temp"), SIM800.print(i), SIM800.print(": "), SIM800.print(TempDS[i]);
        SIM800.print("\n Voltage Now: "), SIM800.print(Vbat),               SIM800.print("V.");
        SIM800.print("\n Voltage Min: "), SIM800.print(V_min),              SIM800.print("V.");
        if (digitalRead(Feedback_Pin) == HIGH)  SIM800.print("\n Voltage for start: "), SIM800.print(VbatStart), SIM800.print("V.");
        if (heating == true)             SIM800.print("\n Timer "),             SIM800.print(Timer/6),   SIM800.print("min.");
        SIM800.print("\n Attempts:"), SIM800.print(count);
        SIM800.print("\n Uptime: "),     SIM800.print(millis()/3600000),        SIM800.print("H.");
      //  SIM800.print("\n https://www.google.com/maps/place/"), SIM800.print(LAT), SIM800.print(","), SIM800.print(LNG);
        SIM800.print((char)26);  }
   
    if (Timer == 12 ) SMS_send = true; 
    if (Timer > 0 ) Timer--;                                // если таймер больше ноля  SMS_send = true;
    if (heating == true && Timer <1)    heatingstop();      // остановка прогрева если закончился отсчет таймера
    if (heating == true && Vbat < 11.0 ) heatingstop();     // остановка прогрева если напряжение просело ниже 11 вольт 
    if (heating == true && TempDS[0] > 86) heatingstop();   // остановка прогрева если температура достигла 70 град 
  
    //  Автозапуск при понижении температуры ниже -18 градусов, при -25 смс оповещение каждых 3 часа
//  if (Timer2 == 2 && TempDS[0] < -18) Timer2 = 1080, Timer = 120, enginestart(3);  
//     Timer2--;                                            // вычитаем еденицу
//  if (Timer2 < 0) Timer2 = 1080;                          // продлеваем таймер на 3 часа (60x60x3/10 = 1080)
    if (heating == false) digitalWrite(LED_Pin, HIGH), delay (50), digitalWrite(LED_Pin, LOW);  // моргнем светодиодом
    if (n_send == true) interval--;
    if (interval <1)    interval = 30, SIM800.println ("AT+SAPBR=3,1, \"Contype\",\"GPRS\""), delay (200);    // подключаемся к GPRS 
    if (interval == 28 && n_send == true ) SIM800.println ("AT+SAPBR=0,1");    
}             


void resp_serial (){     // ---------------- ТРАНСЛИРУЕМ КОМАНДЫ из ПОРТА В МОДЕМ ----------------------------------
     String at = "";   
 //    while (Serial.available()) at = Serial.readString();
  int k = 0;
   while (Serial.available()) k = Serial.read(),at += char(k),delay(1);
     SIM800.println(at), at = "";   }

void resp_modem (){     //------------------ НАЛИЗИРУЕМ БУФЕР ВИРТУАЛЬНОГО ПОРТА МОДЕМА------------------------------
     String at = "";
 //    while (SIM800.available()) at = SIM800.readString();  // набиваем в переменную at
  int k = 0;
   while (SIM800.available()) k = SIM800.read(),at += char(k),delay(1);           
   Serial.println(at);  
 
if (at.indexOf("+CLIP: \""+call_phone+"\",") > -1  && at.indexOf("+CMGR:") == -1 ) {delay(50), SIM800.println("ATA"), ring = true;
                                                    
/*            
    } else if (at.indexOf("+CLIP: \""+SMS_phone+"\",") > -1 && at.indexOf("+CMGR:") == -1 ) {
                                                     delay(50), SIM800.println("ATH0");
                                                     enginestart(3);
 */           
     } else if (at.indexOf("+DTMF: ")  > -1)        {String key = at.substring(at.indexOf("")+9, at.indexOf("")+10);
                                                     pin = pin + key;
                                                     if (pin.indexOf("*") > -1 ) pin= ""; 
 
                 
      } else if (at.indexOf("+CMTI: \"SM\",") > -1) {int i = at.substring(at.indexOf("\"SM\",")+5, at.indexOf("\"SM\",")+6).toInt();
                                                    delay(2000), SIM800.print("AT+CMGR="), SIM800.println(i); // читаем СМС 

     
      } else if (at.indexOf("#123start") > -1   )   {enginestart(5);
      } else if (at.indexOf("#123stop") > -1 )      {Timer=0, heatingstop();       
 //   } else if (at.indexOf("#456start") > -1   )   {Timer = at.substring(at.indexOf()+9, at.indexOf()+11).toInt() *6;     
 //   } else if (at.indexOf("narodmon=off") > -1 )  {n_send = false;  
 //   } else if (at.indexOf("narodmon=on") > -1 )   {n_send = true;  
 //   } else if (at.indexOf("sms=off") > -1 )       {sms_report = false;  
 //   } else if (at.indexOf("sms=on") > -1 )        {sms_report = true;     
      } else if (at.indexOf("SMS Ready") > -1 )     {SIM800.println("AT+CMGDA=\"DEL ALL\";+CLIP=1;+DDET=1"), delay(200); // настройк модема после его загрузки    
    /*  -------------------------------------- проверяем соеденеиние с ИНТЕРНЕТ ------------------------------------------------------------------- */
      } else if (at.indexOf("AT+SAPBR=3,1, \"Contype\",\"GPRS\"\r\r\nOK") > -1 ) {SIM800.println("AT+SAPBR=3,1, \"APN\",\""+APN+"\""),delay (500); 
      } else if (at.indexOf("AT+SAPBR=3,1, \"APN\",\""+APN+"\"\r\r\nOK") > -1 )  {SIM800.println("AT+SAPBR=1,1"),delay (1000); // устанавливаем соеденение   
      } else if (at.indexOf("AT+SAPBR=1,1\r\r\nOK") > -1 )  {SIM800.println("AT+SAPBR=2,1"),        delay (1000);    // проверяем статус соединения    
      } else if (at.indexOf("+SAPBR: 1,1") > -1 )           {/*SIM800.println("AT+CIPGSMLOC=1,1"),    delay (3000);    // запрашиваем геолокацию локацию
      } else if (at.indexOf("+CIPGSMLOC: 0,") > -1   )      {LAT = at.substring(at.indexOf("+CIPGSMLOC: 0,")+24, at.indexOf("+CIPGSMLOC: 0,")+33);
                                                             LNG = at.substring(at.indexOf("+CIPGSMLOC: 0,")+14, at.indexOf("+CIPGSMLOC: 0,")+23); 
                                            delay (200),*/  SIM800.println("AT+CIPSTART=\"TCP\",\""+SERVER+"\",\""+PORT+"\""), delay (1000);
      } else if (at.indexOf("CONNECT OK\r\n") > -1 )      {SIM800.println("AT+CIPSEND"), delay (1200);      
      } else if (at.indexOf("AT+CIPSEND\r\r\n>") > -1 )   {SIM800.print("#" +MAC+ "#" +SENS);                              // заголовок пакета       
                              for (int i=0; i < inDS; i++) SIM800.print("\n#Temp"), SIM800.print(i), SIM800.print("#"), SIM800.print(TempDS[i]);
                                                           SIM800.print("\n#Vbat#"),         SIM800.print(Vbat);          // Напряжение аккумулятора
                                                           SIM800.print("\n#Uptime#"),       SIM800.print(millis()/1000); // Время непрерывной работы
                                                      //   SIM800.print("\n#LAT#"),          SIM800.print(LAT);           // Широта по геолокации
                                                      //   SIM800.print("\n#LNG#"),          SIM800.print(LNG);           // Долгота по геолокации
                                                           SIM800.println("\n##"),           SIM800.println((char)26), delay (100); // закрываем пакет
                                                          } 
at = "";            // Возвращаем ответ можема в монитор порта , очищаем переменную
//Serial.println("Пин "), Serial.println(pin);
       if (pin.indexOf("123") > -1 ){ pin= "", Voice(2), enginestart(3);  
} else if (pin.indexOf("789") > -1 ){ pin= "", Voice(10), delay(1500), SIM800.println("ATH0"),  Timer=0, heatingstop();  
} else if (pin.indexOf("#")   > -1 ){ pin= "", SIM800.println("ATH0"), SMS_send = true;    }
if (ring == true) { ring = false, delay (2000), pin= ""; // обнуляем пин
                    if (heating == false){Voice(1);
                                    }else Voice(8); 
                  }                    
 } 

void enginestart(int Attempts ) {                                      // программа запуска двигателя
 /*  ----------------------------------------- ПРЕДНАСТРОЙКА ПЕРЕД ЗАПУСКОМ ---------------------------------------------------------*/
Serial.println("Предпусковая настройка");
    detachInterrupt(1);                                    // отключаем аппаратное прерывание, что бы не мешало запуску
    VbatStart = VoltRead();

int StTime  = map(TempDS[0], 20, -15, 1000, 5000);         // при -15 крутим стартером 5 сек, при +20 крутим 1 сек
    StTime  = constrain(StTime, 1000, 6000);               // ограничиваем диапазон работы стартера от 1 до 6 сек

int z = map(TempDS[0], 0, -25, 0, 5);                     // задаем количество раз прогрева свечей пропорциолально температуре
    z = constrain(z, 0, 5);                                // огрничиваем попытки от 0 до 5 попыток

    Timer   =  map(TempDS[0], 30, -25, 30, 150);           // при -25 крутим прогреваем 30 мин, при +50 - 5 мин
    Timer   =  constrain(Timer, 30, 180);                  // ограничиваем таймер в зачениях от 5 до 30 минут

    count = 0;                                             // переменная хранящая число совершенных попыток запуска
    V_min = 14;                                            // переменная хранящая минимальные напряжения в ммент старта

// если напряжение АКБ больше 10 вольт, зажигание выключено, температуры выше -25 град, счетчик числа попыток  меньше 5
 while (Vbat > 10.00 && digitalRead(Feedback_Pin) == LOW && TempDS[0] > -30 && count < Attempts){ 
 
 count++, Serial.print ("Попытка №"), Serial.println(count); 
    
 digitalWrite(SECOND_P,     LOW),   delay (2000);        // выключаем зажигание на 2 сек. на всякий случай   
 digitalWrite(FIRST_P_Pin, HIGH),   delay (1000);        // включаем реле первого положения замка зажигания 
 Voice(3);
 digitalWrite(SECOND_P,    HIGH),   delay (4000);        // включаем зажигание, и выжидаем 4 сек.
 
// прогреваем свечи несколько раз пропорционально понижению температуры, греем по 8 сек. с паузой 2 сек.
while (z > 0) Voice(3), digitalWrite(SECOND_P, LOW), delay(2000), digitalWrite(SECOND_P, HIGH), delay(8000);
 
// если не нажата педаль тормоза или КПП в нейтрали то включаем реле стартера на время StTime
 if (digitalRead(STOP_Pin) == LOW) {         // увеличиваем на еденицу число оставшихся потыток запуска
                                   Serial.println("Стартер включил");
                                   Voice(4);
                                   StarterTimeON = millis();
                                   digitalWrite(STARTER_Pin, HIGH);  // включаем реле стартера
                                   } else {
                                   Voice(7); 
                                   heatingstop();
                                   Timer = 0;
                                   count = -1;  
                                   break; 
                                   } 
 delay (100);
//float V_stON = VoltRead();                              // временно так
 while ((millis() < (StarterTimeON + StTime)) /* && ((VoltRead() + V_stOFF) < V_stON)*/)VoltRead(), delay (50);
 digitalWrite(STARTER_Pin, LOW);
 Serial.println("Стартер выключил, ожидаем 6 сек.");
 Voice(12);
 delay (6000);       

// if (digitalRead(DDM_Pin) != LOW) {                // если детектировать по датчику давления масла 

 if (VoltRead() > Vstart) {                          // если детектировать по напряжению зарядки     
                          Serial.println ("Есть запуск!"); 
                          Voice(5);
                          heating = true;
                          digitalWrite(LED_Pin, HIGH);
                          break; }                   // считаем старт успешным, выхдим из цикла запуска двигателя
                          
  Voice(6);              
  StTime = StTime + 200;                             // увеличиваем время следующего старта на 0.2 сек.
  heatingstop();   }                                 // уменьшаем на еденицу число оставшихся потыток запуска
                  
Serial.println ("Выход из запуска");
 if (count != 1) SMS_send = true;                   // отправляем смс СРАЗУ только в случае незапуска c первой попытки
 if (heating == false){ Timer = 0, Voice(10);                  // обнуляем таймер если запуска не произошло
           } else digitalWrite(WEBASTO_Pin, HIGH);  // включаем подогрев седений если все ОК


           
delay(3000), SIM800.println("ATH0");                            // вешаем трубку (для SIM800L) 
attachInterrupt(1, callback, FALLING);          // включаем прерывание на обратный звонок
 }


float VoltRead()    {                               // замеряем напряжение на батарее и переводим значения в вольты
              float ADCC = analogRead(BAT_Pin);
                    ADCC = ADCC / m ;
                    Serial.print("Напряжение: "), Serial.print(ADCC), Serial.println("V");    
                    if (ADCC < V_min) V_min = ADCC;                   
                    return(ADCC); }                  // переводим попугаи в вольты
                    


void heatingstop() {                                // программа остановки прогрева двигателя
    digitalWrite(SECOND_P,    LOW), delay (100);
    digitalWrite(LED_Pin,     LOW), delay (100);
    digitalWrite(FIRST_P_Pin, LOW), delay (100);
    digitalWrite(WEBASTO_Pin, LOW), delay (100);
    heating= false,                 delay(2000); 
    Serial.println ("Выключить все"); }

void Voice(int Track){
    SIM800.print("AT+CREC=4,\"C:\\User\\"), SIM800.print(Track), SIM800.println(".amr\",0,95");}

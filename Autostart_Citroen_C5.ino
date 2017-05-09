// https://www.drive2.ru/l/471004119256006698/
//Автозапуск автомобиля на связке модулей SIM800L+Arduino с управлением по DTMF и отчетами по SMS
#include <SoftwareSerial.h> // подключаем библиотеку виртуального порта
SoftwareSerial gsm(8, 7); // и назначаем пины связи Arduino и SIM800 RX, TX

#include <DallasTemperature.h> // подключаем библиотеку чтения датчиков температуры
#define ONE_WIRE_BUS 9 // и настраиваем  пин 9 как шину подключения датчиков DS18B20
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);

// ******** назначаем выводы Ардуино в зависимости от его подключения  **************
#define RES_Pin 6       // пин перезапуски SIM800L - пока не реализован 
#define DTR_Pin 4       // пин сна SIM800L - пока не реализован
#define START_Pin 11    // на реле стартера, через транзистор с 11-го пина ардуино
#define ACC_Pin 12      // на реле зажигания, через транзистор с 12-го пина ардуино
#define ACTIV_Pin 10    // на светодиод через транзистор, c 10-го пина для индикации активности 
#define BAT_Pin A3      // на батарею, через делитель напряжения 39кОм / 11 кОм
#define Pric_Pin A2     // на прикуриватель или на ДАДМ, для детектирования момента запуска
#define STOP_Pin A1     // на концевик педали тормоза для отключения режима прогрева

String AT_CMD;          // переменная хранения массива данных от модема
int poz = 0;            // позиция в массиве пинкода
int pin[30];             // сам массив набираемого пинкода
int WarmUpTimer = 100;  // переменная времени прогрева двигателя по умолчанию
int count = 0 ;         // счетчик неудачных попыток запуска  
unsigned long Time1 = 0;
unsigned long Time2 = 50; // продолжительность разговора сек.
bool heating = false;     // переменная состояния режим прогрева двигателя
bool alarm_call = false;  // переменная состояния тревоги 
bool alarm_one = false;    // флаг состояния разового срабатывания тревоги
bool alarm_bat = false;   // флаг состояния разового срабатывания по питанию АКБ
bool SMS_send = false;   // флаг разовой отправки СМС
float m = 66.91;         // делитель для перевода АЦП в вольты
float Vbat;              // переменная хранящая напряжение бортовой сети
 
void setup() 
{ 
Serial.begin(9600);
gsm.begin(9600); 
gsm.println("AT+CLIP=1"),delay (20); // включаем определитель номера
gsm.setTimeout(100),delay (20); // задаем задержку для Serial.parseInt() 
Serial.println ("Starting, v.2.0 05/05/2017 ");
pinMode(2, INPUT_PULLUP);    // подтягиваем ногу оптрона (датчика вибрации) к +3.3v 
pinMode(ACTIV_Pin, OUTPUT);  // указываем пин на выход (светодиод)
pinMode(START_Pin, OUTPUT);  // указываем пин на выход (реле стартера)
gsm.println("AT+CMGF=1"), delay(100); // включаем текстовый режим для СМС  
gsm.println("AT+CSCS=\"GSM\""), delay(100); // кодировка текстового режима СМС  

//attachInterrupt(1, ring, FALLING);  // прерывание на входящий звонок
attachInterrupt(0, alarm, FALLING); // прерывание на датчик вибрации c 1 > 0
} 

void loop() { 

if(gsm.find("+375000000000\",145,\"")){ // если нашли номер телефона то
    Serial.println("RING! +375....."); 
    gsm.println("AT+DDET=1"), delay(100); // включаем DTMF-декодер
    gsm.println("ATA"), delay(500); // снимаем трубку
    gsm.println("AT+VTS=\"3,5,7\""), delay(200); // пикнем в трубку 2 раза
    
    unsigned long start_call=millis();
    pin[0]=0, pin[1]=0,pin[2]=0, poz=0; // устанавливаем пинкод 000
    
        while(1){ // крутимся в цикле при снятой трубке пока не разорвется вызов
          
        AT_CMD=ReadAT(); // детектируем нажатие кнопок разбирая команды
        delay(300);     
        if(AT_CMD=="\r\n+DTMF: 1\r\n")       {pin[poz]=1, poz++, Serial.print("1");  
  
        }else if(AT_CMD=="\r\n+DTMF: 2\r\n") {pin[poz]=2, poz++, Serial.print("2") ; 
  
        }else if(AT_CMD=="\r\n+DTMF: 3\r\n") {pin[poz]=3, poz++, Serial.print("3") ; 
  
        }else if(AT_CMD=="\r\n+DTMF: 4\r\n") {pin[poz]=4, poz++, Serial.print("4") ;  
   
        }else if(AT_CMD=="\r\n+DTMF: 5\r\n") {pin[poz]=5, poz++, Serial.print("5") ; 
  
        }else if(AT_CMD=="\r\n+DTMF: 6\r\n") {pin[poz]=6, poz++, Serial.print("6") ; 
  
        }else if(AT_CMD=="\r\n+DTMF: 7\r\n") {pin[poz]=7, poz++, Serial.print("7") ; 
  
        }else if(AT_CMD=="\r\n+DTMF: 8\r\n") {pin[poz]=8, poz++, Serial.print("8") ; 
        
        }else if(AT_CMD=="\r\n+DTMF: 9\r\n") {pin[poz]=9, poz++, Serial.print("9") ;  
        

        }else if(AT_CMD=="\r\n+DTMF: 0\r\n") {pin[0]=0, pin[1]=0, pin[2]=0, poz=0, Serial.println("0 > 000"); 
                
        }else if(AT_CMD=="\r\n+DTMF: #\r\n") {SMS_send=true, alarm_one=true, alarm_bat=false; 
        
        }else if (pin[0]==2 && pin[1]==3 && pin[2]==6){ // если пин 236 обнуляем пинкод > запуск на 10 минут
        delay(200), pin[0]= 0, pin[1]=0, pin[2]=0, poz=0, WarmUpTimer=60, Serial.println("Start 263"), engiestart(); 
        
        }else if (pin[0]==5 && pin[1]==6 && pin[2]==7){ // если пин 567 обнуляем пинкод > запуск на  20 минут
        delay(200), pin[0]= 0, pin[1]= 0, pin[2]=0, poz=0, WarmUpTimer=120, Serial.println("Start 567"), engiestart(); 

        } else if (pin[0]==8 && pin[1]==9 && pin[2]==6){     // если пин 896 
        pin[0]=0, pin[1]=0, pin[2]=0, poz=0, Serial.println("STOP"), heatingstop(); // обнуляем пинкод выключаем зажигание
        }
          
        else if(AT_CMD=="\r\nNO CARRIER\r\n" || millis() > start_call + Time2 *1000){ 
        gsm.println("ATH0");
        break; 
        } 
                } // конец цикла разговора
Serial.println("call-end while");
                  } // конец цикла поиска вызова
                      
if (millis()> Time1 + 10000) detection(), Time1 = millis(); // выполняем функцию detection () каждые 10 сек 
if (heating == true && digitalRead(STOP_Pin)==1) heatingstop() ;//если нажали на педаль тормоза в режиме прогрева
                                     
} // end void loop 

void detection(){ // условия проверяемые каждые 10 сек  
    sensors.requestTemperatures();   // читаем температуру с трех датчиков
    float tempds0 = sensors.getTempCByIndex(0);  
    float tempds1 = sensors.getTempCByIndex(1);
    float tempds2 = sensors.getTempCByIndex(2);
    Vbat = analogRead(BAT_Pin);  // замеряем напряжение на батарее
    Vbat = Vbat / m ; // переводим попугаи в вольты
    Serial.print("Vbat= "),Serial.print(Vbat), Serial.println(" V.");    
    if (heating == true) WarmUpTimer--;    // если двигатель в режиме прогрева - отнимаем от таймера еденицу      
    if (heating == true && WarmUpTimer <1) Serial.println("End timer"), heatingstop() ; 
    if (heating == true && Vbat < 11.3) Serial.println("Low voltage"), heatingstop() ; 
    if (heating == false) digitalWrite(ACTIV_Pin, HIGH), delay (50), digitalWrite(ACTIV_Pin, LOW);  // моргнем светодиодом
    if (alarm_call == true && alarm_one==true) call(), alarm_call=false, alarm_one==false; // звоним на номер по тревоге
    if (alarm_bat == true && Vbat < 7.55) alarm_bat = false, SMS_send = true, Serial.print("Voltage below 7.5 V");
    if (SMS_send == true) {  // если фаг SMS_send равен 1 высылаем отчет по СМС
        Serial.println("SMS send start"), delay(1000);
        gsm.println("AT+CMGS=\"+375000000000\""), delay(500); // номер телефона куда слать СМС
        gsm.println("Status Citrien C5" );
        gsm.print("Batareja: "), gsm.print(Vbat), gsm.println(" V.");
        gsm.print("Status: ");
        if (heating == true) {
           gsm.println(" Progrev ");
           } else { gsm.println(" Ojidanie ");}
        if (digitalRead(STOP_Pin) == LOW) {
           gsm.println(" KPP v neitrali ");
           } else { gsm.println(" Na peredache !!! ");}
        gsm.print("Temp.Dvig: "), gsm.print(tempds0),  gsm.println((char)94); 
        gsm.print("Temp.Salon: "), gsm.print(tempds1), gsm.println((char)94);
        gsm.print("Temp.Ulica: "), gsm.print(tempds2), gsm.println((char)94);
        if (heating == true)  gsm.print("Timer, ostalos : "), gsm.print(WarmUpTimer * 10 / 60), gsm.println(" min.");
           gsm.print("Popytok zapuska: "), gsm.print(count); 
        delay(100), gsm.print((char)26), delay(100), Serial.println("SMS send finish");   
        delay(3000);
        SMS_send = false;
        }
// шлем отчет в серийный порт для отладки        
Serial.print(" || heating ="),Serial.print(heating);
Serial.print(" || WarmUpTimer = "),Serial.print(WarmUpTimer),Serial.print(" || tempds0 =");
Serial.print(tempds0),Serial.print(" || tempds1 = "), Serial.print(tempds1), Serial.print(" || tempds2 =");
Serial.print (tempds2), Serial.print(" || millis ="), Serial.println (millis()/1000);
               }
 
void alarm()   //   функция ревоги, если на оптрон пришли 12В
  {  
alarm_call = true; // меняем флаг и выходим из цикла
  }

void  call()   // функция обратного звонка375000000000
{  
Serial.println ("call +37529.....");
gsm.println("ATD+375000000000;"), delay(100); // звоним по указаному номеру
if (gsm.find("OK")) Serial.println("OK ATD");
else Serial.println("error ATD");
}  

void engiestart() {                                                 // программа запуска двигателя
Serial.println ("Start"), gsm.println("AT+VTS=\"2,6\""), count = 0; // пикнем в трубку 1 раз

if (digitalRead(Pric_Pin) == LOW && digitalRead(STOP_Pin) == LOW) { // если на входе Pric_Pin 0 пробуем заводить, потытка №1
   digitalWrite(ACC_Pin, LOW),    delay (3000);                    // выключаем зажигание на 3 сек.      
   digitalWrite(ACC_Pin, HIGH),   delay (2000);                    // включаем зажигание 
   Vbat = analogRead(BAT_Pin);                                     // замеряем напряжение на батарее через 2 сек.
   Vbat = Vbat / m , Serial.print ("Start №1 V bat: ");            // переводим попугаи в вольты.
   count = 1, Serial.println (BAT_Pin), delay (2000);              // печатаем в сериал вольты.
   digitalWrite(START_Pin, HIGH), delay (1000);                    // включаем реле стартера на 1.0 сек. 
   digitalWrite(START_Pin, LOW),  delay (5000);                    // отключаем реле, ждем 5 сек.
      }
  
if (digitalRead(Pric_Pin) == LOW && digitalRead(STOP_Pin) == LOW) { // если на входе Pric_Pin 0 пробуем заводить, потытка №1
   digitalWrite(ACC_Pin, LOW),    delay (3000);                    // выключаем зажигание на 3 сек.      
   digitalWrite(ACC_Pin, HIGH),   delay (2000);                    // включаем зажигание 
   Vbat = analogRead(BAT_Pin);                                     // замеряем напряжение на батарее через 2 сек.
   Vbat = Vbat / m , Serial.print ("Start №2 V bat: ");            // переводим попугаи в вольты.
   count = 1, Serial.println (BAT_Pin), delay (2000);              // печатаем в сериал вольты.
   digitalWrite(START_Pin, HIGH), delay (1200);                    // включаем реле стартера на 1.2 сек. 
   digitalWrite(START_Pin, LOW),  delay (5000);                    // отключаем реле, ждем 5 сек.
      }
  
if (digitalRead(Pric_Pin) == LOW && digitalRead(STOP_Pin) == LOW) { // если на входе Pric_Pin 0 пробуем заводить, потытка №1
   digitalWrite(ACC_Pin, LOW),    delay (3000);                    // выключаем зажигание на 3 сек.      
   digitalWrite(ACC_Pin, HIGH),   delay (2000);                    // включаем зажигание 
   Vbat = analogRead(BAT_Pin);                                     // замеряем напряжение на батарее через 2 сек.
   Vbat = Vbat / m , Serial.print ("Start №3 V bat: ");            // переводим попугаи в вольты.
   count = 1, Serial.println (BAT_Pin), delay (2000);              // печатаем в сериал вольты.
   digitalWrite(START_Pin, HIGH), delay (1500);                    // включаем реле стартера на 1.5 сек. 
   digitalWrite(START_Pin, LOW),  delay (5000);                    // отключаем реле, ждем 5 сек.
      }
       
if (digitalRead(Pric_Pin) == HIGH){ // если есть старт
   gsm.println("AT+VTS=\"4,9\""), heating = true, digitalWrite(ACTIV_Pin, HIGH); // пикнем в трубку, изменгим флаг
   Serial.println ("Engine started"), gsm.println("ATH0");
                                  }
                                  else 
                                  {
   heatingstop(), delay (500), gsm.println("ATH0"), SMS_send = true; // если нет запуска отправим СМС
                                  }
}

void heatingstop() {  // программа остановки прогрева двигателя
    digitalWrite(ACC_Pin, LOW), digitalWrite(ACTIV_Pin, LOW);
    heating= false, Serial.println ("Warming stopped");
    gsm.println("AT+VTS=\"7,7,7,7,7,7,7,7\""),delay(6000), gsm.println("ATH0"); // пикнем в трубку 7 раз
                   }
 
String ReadAT() 
{ 
int k;
String at; 
while (gsm.available()) k = gsm.read(), at += char(k), delay(10); 
return at; 
}

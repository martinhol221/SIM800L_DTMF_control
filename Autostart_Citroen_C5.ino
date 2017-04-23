// https://www.drive2.ru/l/471004119256006698/
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
int pin[3];             // сам массив набираемого пинкода
int WarmUpTimer = 100;  // переменная времени прогрева двигателя по умолчанию
int count = 0 ;         // счетчик неудачных попыток запуска  
unsigned long Time1 = 0;
unsigned long Time2 = 50; // продолжительность разговора сек.
bool heating = false;     // переменная состояния режим прогрева двигателя
bool alarm_call = false;  // переменная состояния тревоги 
bool alarm_one = false;    // флаг состояния разового срабатывания тревоги
bool alarm_bat = false;   // флаг состояния разового срабатывания по питанию АКБ
bool SMS_send = false;   // флаг разовой отправки СМС

void setup() 
{ 
Serial.begin(9600);
gsm.begin(9600); 
gsm.println("AT+CLIP=1"),delay (20); // включаем определитель номера
gsm.setTimeout(100),delay (20); // задаем задержку для Serial.parseInt() 
Serial.println ("Starting, v.1.8 22/04/2017 ");
pinMode(2, INPUT_PULLUP);    // подтягиваем ногу оптрона (датчика вибрации) к +3.3v 
pinMode(ACTIV_Pin, OUTPUT);  // указываем пин на выход (светодиод)
pinMode(START_Pin, OUTPUT);  // указываем пин на выход (реле стартера)
gsm.println("AT+CMGF=1"), delay(100); // включаем текстовый режим для СМС  
gsm.println("AT+CSCS=\"GSM\""), delay(100); // кодировка текстового режима СМС  

//attachInterrupt(1, ring, FALLING);  // прерывание на входящий звонок
attachInterrupt(0, alarm, FALLING); // прерывание на датчик вибрации c 1 > 0
} 

void loop() { 

if(gsm.find("+375290000000\",145,\"")){ // если нашли номер телефона то
    Serial.println("RING! +375....."); 
    gsm.println("AT+DDET=1"), delay(10); // включаем DTMF-декодер
    gsm.println("ATA"), delay(1000); // снимаем трубку
    gsm.println("AT+VTS=\"3,5,7\""), delay(200); // пикнем в трубку 2 раза
    
    unsigned long start_call=millis();
    pin[0]=0, pin[1]=0,pin[2]=0, poz=0; // устанавливаем пинкод 000
    
        while(1){ // крутимся в цикле при снятой трубке пока не разорвется вызов
          
        AT_CMD=ReadAT(); // детектируем нажатие кнопок разбирая команды
        delay(500);     
        if(AT_CMD=="\r\n+DTMF: 1\r\n")       {pin[poz]=1, poz++ ;  
  
        }else if(AT_CMD=="\r\n+DTMF: 2\r\n") {pin[poz]=2, poz++ ; 
  
        }else if(AT_CMD=="\r\n+DTMF: 3\r\n") {pin[poz]=3, poz++ ; 
  
        }else if(AT_CMD=="\r\n+DTMF: 4\r\n") {pin[poz]=4, poz++ ;  
   
        }else if(AT_CMD=="\r\n+DTMF: 5\r\n") {pin[poz]=5, poz++ ; 
  
        }else if(AT_CMD=="\r\n+DTMF: 6\r\n") {pin[poz]=6, poz++ ; 
  
        }else if(AT_CMD=="\r\n+DTMF: 7\r\n") {pin[poz]=7, poz++ ; 
  
        }else if(AT_CMD=="\r\n+DTMF: 8\r\n") {pin[poz]=8, poz++ ; 
        
        }else if(AT_CMD=="\r\n+DTMF: 9\r\n") {pin[poz]=9, poz++ ;  
        
        }else if(AT_CMD=="\r\n+DTMF: 0\r\n") {pin[poz]=0, poz++ ; 
  
        }else if(AT_CMD=="\r\n+DTMF: *\r\n") {pin[0]=0, pin[1]=0, pin[2]=0, poz=0 ; 
                
        }else if(AT_CMD=="\r\n+DTMF: #\r\n") {SMS_send=true, alarm_one=true, alarm_bat=false; 
        
        }else if (pin[0]==1 && pin[1]==2 && pin[2]==3){ // если пин 123 обнуляем пинкод > запуск на 10 минут
        delay(200), pin[0]= 0, pin[1]=0, pin[2]=0, poz=0, WarmUpTimer=60, engiestart(); 
        
        }else if (pin[0]==4 && pin[1]==5 && pin[2]==6){ // если пин 456 обнуляем пинкод > запуск на  20 минут
        delay(200), pin[0]= 0, pin[1]= 0, pin[2]=0, poz=0, WarmUpTimer=120, engiestart(); 
        
        } else if (pin[0]==7 && pin[1]==8 && pin[2]==9){     // если пин 789 
        pin[0]=0, pin[1]=0, pin[2]=0, poz=0, heatingstop(); // обнуляем пинкод выключаем зажигание
        }
          
        else if(AT_CMD=="\r\nNO CARRIER\r\n" || millis() > start_call + Time2 *1000){ 
        // если пришел отбой или прошло 50 сек. - выходим из цикла
        break; 
        } 
                } // end while 
Serial.println("call-end while");
                
                      } // конец цикла поиска вызова
                      
                      
if (millis()> Time1 + 10000) detection(), Time1 = millis(); // выполняем функцию detection () каждые 10 сек 

if (heating == true && digitalRead(STOP_Pin)==1) { //если нажали на педаль тормоза в режиме прогрева
    digitalWrite(ACTIV_Pin, LOW), digitalWrite(ACC_Pin, LOW), heating=false; // выключаем зажигание, гасим светодиод
                                                  }
} // end void loop 

void detection(){ 
    sensors.requestTemperatures();   // читаем температуру с трех датчиков
    float tempds0 = sensors.getTempCByIndex(0);  
    float tempds1 = sensors.getTempCByIndex(1);
    float tempds2 = sensors.getTempCByIndex(2);
    float Vbat = analogRead(BAT_Pin);  // замеряем напряжение на батарее
    Vbat = Vbat / 66.91;    // переводим попугаи в вольты (835 / 66.91 = 12.5 вольт)
    if (heating==true) { 
      WarmUpTimer--;// если двигатель в прогреве - отнимаем от таймера еденицу
    } else {  
      WarmUpTimer = 100;  // иначе выставляем таймер 
      }  
    // условия проверяемые каждые 10 сек  
    if (heating == true && WarmUpTimer <1) heatingstop(); // если двигатель в прогреве и таймер равен 0  -  глушим двигатель
    if (heating == true && Vbat < 11.3) heatingstop(); // если в прогреве напряжение просело ниже 11.3 в - двигатель заглох
    if (heating == false) digitalWrite(ACTIV_Pin, HIGH), delay (50), digitalWrite(ACTIV_Pin, LOW);  // моргнем светодиодом
    if (alarm_call == true && alarm_one==true) call(), alarm_call=false, alarm_one==false; // звоним на номер по тревоге
    if (alarm_bat == true && Vbat < 7.55) alarm_bat = false, SMS_send = true; // если напряжение на АКБ ниже 6 вольт, шлем СМС
    if (SMS_send == true) {  // если фаг SMS_send равен 1 высылаем отчет по СМС
                  Serial.println("SMS send start"), delay(1000);
                  gsm.println("AT+CMGS=\"+375290000000\""),delay(500); // номер телефона куда слать СМС
                  gsm.println("Status Citrien C5" );
                  gsm.print("Batareja: "), gsm.print(Vbat), gsm.println(" V.");
                  gsm.print("Status: ");
                      if (heating == true) {
                        gsm.println(" Progrev ");
                        } else { gsm.println(" Ojidanie ");}
                  gsm.print("Neitral: ");
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
Serial.print("Vbat= "),Serial.print(Vbat), Serial.print(" || heating ="),Serial.print(heating);
Serial.print(" || WarmUpTimer = "),Serial.print(WarmUpTimer),Serial.print(" || tempds0 =");
Serial.print(tempds0),Serial.print(" || tempds1 = "), Serial.print(tempds1), Serial.print(" || tempds2 =");
Serial.print (tempds2), Serial.print(" || millis ="), Serial.println (millis()/1000);
               }
 
void alarm()   //   функция ревоги, если на оптрон пришли 12В
  {  
alarm_call = true; // меняем флаг и выходим из цикла
  }

void  call()   // функция обратного звонка
{  
Serial.println ("call +37529,,,,,");

 gsm.println("ATD+375290000000;"); // звоним по указаному номеру
      delay(100);
      if (gsm.find("OK")) Serial.println("OK ATD");
      else Serial.println("error ATD");
  }  



void engiestart() {  // программа запуска двигателя

Serial.println ("Engie starting...."), gsm.println("AT+VTS=\"2,6\""); // пикнем в трубку 1 раз
      count = 0 ;
if (digitalRead(Pric_Pin) == LOW && digitalRead(STOP_Pin) == LOW) { // если на входе Pric_Pin 0 пробуем заводить, потытка №1
      Serial.println ("engiestart №1"), count = 1;
      digitalWrite(ACC_Pin, LOW),    delay (5000);   // выдержка выключенного зажигания 5 сек.  
      digitalWrite(ACC_Pin, HIGH),   delay (5000);   // выдержка включенного зажигания 5 сек.
      digitalWrite(START_Pin, HIGH), delay (1000); // включаем реле стартером 1 сек. 
      digitalWrite(START_Pin, LOW),  delay (5000);   // отключаем реле, ждем 5 сек.
      }
  
if (digitalRead(Pric_Pin) == LOW && digitalRead(STOP_Pin) == LOW) { // если на входе Pric_Pin 0 пробуем заводить, потытка №2
      Serial.println ("engiestart №2"), count = 2;
      digitalWrite(ACC_Pin, LOW),    delay (5000);   // выдержка выключенного зажигания 5 сек.  
      digitalWrite(ACC_Pin, HIGH),   delay (5000);   // выдержка включенного зажигания 5 сек.
      digitalWrite(START_Pin, HIGH), delay (1200); // включаем реле стартером 1.2 сек.
      digitalWrite(START_Pin, LOW),  delay (5000);   // отключаем реле, ждем 5 сек.
      }
  
if (digitalRead(Pric_Pin) == LOW && digitalRead(STOP_Pin) == LOW) { // если на входе Pric_Pin 0 пробуем заводить, потытка №3  
      Serial.println ("engiestart №3"), count = 3;
      digitalWrite(ACC_Pin, LOW),    delay (5000);   // выдержка выключенного зажигания 5 сек.  
      digitalWrite(ACC_Pin, HIGH),   delay (5000);   // выдержка включенного зажигания 5 сек.
      digitalWrite(START_Pin, HIGH), delay (1500); // включаем реле стартером 1.5 сек.
      digitalWrite(START_Pin, LOW),  delay (5000);   // отключаем реле, ждем 5 сек.
      }      
  
       
if (digitalRead(Pric_Pin) == HIGH){ // если появилось питание переферии считаем что двигател успешно запущен
            gsm.println("AT+VTS=\"4,9\""), heating = true;
            Serial.println ("heating = true"), digitalWrite(ACTIV_Pin, HIGH);
            }
            else 
            {
            
            digitalWrite(ACC_Pin, LOW), heating=false, Serial.println("heating = false");
            digitalWrite(ACTIV_Pin, LOW), SMS_send = true; // если нет запуска отправим СМС
            }
            
}

void heatingstop() {  // программа остановки двигателя
    digitalWrite(ACC_Pin, LOW), digitalWrite(ACTIV_Pin, LOW), heating= false;
    gsm.println("AT+VTS=\"7,7,7,7,7,7,7,7\""), Serial.println ("heating = false");
    // пикнем в трубку 7 раз
                 }
 
String ReadAT() 
{ 
int k;
String at; 
while (gsm.available()) k = gsm.read(), at += char(k), delay(10); 
return at; 
}

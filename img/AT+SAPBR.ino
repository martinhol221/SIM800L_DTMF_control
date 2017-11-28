AT+IPR=9600;E1+DDET=1;+CMGF=1;+CSCS="gsm";+CNMI=2,1,0,0,0;+VTD=1;+CMEE=1;&W   // первоначальная настройка модема

OK                                                                           

AT+IPR=9600;E1+DDET=1;+CMGF=1;+CSCS="gsm";+CNMI=2,1,0,0,0;+VTD=1;+CMEE=1;&W;+CMGDA="DEL ALL";+CLIP=1 // тоже работает

OK


AT+SAPBR=3,1, "Contype","GPRS";+SAPBR=3,1, "APN","internet.mts.by";+SAPBR=3,1,"USER","mts";+SAPBR=3,1,"PWD","mts";+SAPBR=1,1

OK

AT+SAPBR=2,1

+SAPBR: 1,1,"100.125.2.81"

OK
AT+SAPBR=0,1

OK
  
  
  
AT+SAPBR=3,1, "Contype","GPRS"        // Установка настроек подключения CONTYPE – тип подключения GPRS или CSD

AT+SAPBR=3,1, "APN","internet.mts.by" // Установка настроек подключения APN – точка подключения

AT+SAPBR=3,1,"USER","mts"             // 

AT+SAPBR=3,1,"PWD","mts"              //

AT+SAPBR=5,1                         // сохранение текущих настроек GPRS соединения

AT+SAPBR=1,1                          // Устанавливаем GPRS соединение         

AT+SAPBR=2,1                          // 

+SAPBR: 1,1,"100.72.204.XXX"


AT+CIPGSMLOC=1,1


AT+SAPBR=0,1                         // закрываем GPRS соединение    

AT+SAPBR=4,1                         // чтение текущих настроек подключения

+SAPBR:
CONTYPE: GPRS
APN: internet.mts.by
PHONENUM: 
USER: mts
PWD: mts
RATE: 2

OK


at+ceng=1

at+ceng?




String at = "+CIPGSMLOC: 0,25.006543,55.009465,2017/11/24,09:43:57";
String GPS;
String Date;
int Sec;
int Min;
int Hr;
void setup() {
Serial.begin(9600);


if (at.indexOf("+CIPGSMLOC: 0,") > -1   ) {
                                  GPS = "https://www.google.by/maps/place/";
                                  GPS = GPS + at.substring(at.indexOf("+CIPGSMLOC: 0,")+24, at.indexOf("+CIPGSMLOC: 0,")+33);    // широта
                                  GPS = GPS + ",";
                                  GPS = GPS + at.substring(at.indexOf("+CIPGSMLOC: 0,")+14, at.indexOf("+CIPGSMLOC: 0,")+23);    // долгота
                                  Date = at.substring(at.indexOf("+CIPGSMLOC: 0,")+34, at.indexOf("+CIPGSMLOC: 0,")+44);         // дата
                                  Hr =   at.substring(at.indexOf("+CIPGSMLOC: 0,")+45, at.indexOf("+CIPGSMLOC: 0,")+57).toInt(); // часы 
                                  Min =  at.substring(at.indexOf("+CIPGSMLOC: 0,")+48, at.indexOf("+CIPGSMLOC: 0,")+50).toInt(); // минуты                                 
                                  Sec =  at.substring(at.indexOf("+CIPGSMLOC: 0,")+51, at.indexOf("+CIPGSMLOC: 0,")+53).toInt(); // секунды
                                          }

Serial.println (GPS);
Serial.println (Date);
Serial.println (Hr);
Serial.println (Min);
Serial.println (Sec);
}

void loop() {

}





+CTZV: +12,0

*PSUTTZ: 2017,11,28,15,35,58,"+12",0

DST: 0


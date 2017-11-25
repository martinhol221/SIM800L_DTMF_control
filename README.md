  ## Система удаленного запуска двигателя автомобиля на базе связки SIM800L + Arduino c управлением по DTMF и отчетами по SMS.

Ссылки на предыдущие материалы:

[Анатомия автозапуска (DRIVE2.RU)](https://www.drive2.ru/l/471004119256006698/) смый первый опыт , еще литием на борту

[Анатомия автозапуска 3.0 (DRIVE2.RU)](https://www.drive2.ru/l/474891992371823652/) в пластиколвом корпусе

[Новые платки и новый скетч для автозапуска (DRIVE2.RU)](https://www.drive2.ru/c/485387655492665696/) 


## Так выглядит плата управления версии 1.5
![](https://github.com/martinhol221/SIM800L_DTMF_control/blob/master/img/Sim800L-1.5+.JPG)


# Прошивка, скетч

[Скетч 3.0  для загрузки через Arduino IDE](https://github.com/martinhol221/SIM800L_DTMF_control/blob/master/Autostart_Citroen_V3.1.ino) в Arduino Pro Mini (8Mhz/3.3v), описание по нему ниже

[Скетч 4.0  для (тестовый) ](https://github.com/martinhol221/SIM800L_DTMF_control/blob/master/Autostart_Citroen_V4.0.TEST.ino) отличие от 3.0; 

* время работы стартера линейно привязано к температуре с датчика `TempDS0`, при -25 ... +50 время работы стартера 6.0 ... 1.0 сек.

* таймер обратного отсчета линейно привязан к температуре с датчика `TempDS0`, при -25 ... +50 время работы стартера 5 ... 30 мин.

![](https://github.com/martinhol221/SIM800L_DTMF_control/blob/master/img/loading.JPG)

# Конфигурация скетча :

* номер телефона хозяина для входящих вызовов `call_phone= "+375290000000";` 

* номер телефона куда отправляем СМС отчет `SMS_phone= "+375290000000";`

* адрес устройства на сервере `MAC = "80-01-AA-00-00-00";` - нули заменить на свои придуманные цифры 

* имя устройства на сервере народмон `SENS = "VasjaPupkin";` - аналогично

* точка доступа для выхода в интернет `APN = "internet.mts.by";` вашего сотового оператора

* имя ` USER = "mts"; ` и пароль `PASS = "mts";` для выхода в интернет вашего сотового оператора

* `n_send = true;` если вы хотите, или `n_send = false;` если не хотите отправлять данные на сервер

* `sms_report = true;` - разрешить отправку SMS отчета, или `sms_report = false;` если жалко денег на SMS

* `Vstart = 13.50` - порог детектирования по которому будем считать что авто зарежает АКБ

* `m = 66.91;` - делитель, для точной калебровки напряжения АКБ 

# Подключение:

![](https://github.com/martinhol221/SIM800L_DTMF_control/blob/master/img/Sin800LV1.5.jpg)

* выход на реле иммобилайзера и первого положения замка зажигания `FIRST_P_Pin 8`, на плате `OUT1`

* выход на реле зажигания `ON_Pin 9`, на плате `OUT2` 

* выход на реле стартера `STARTER_Pin 12`, на плате `OUT3` 

* выход на включение обогрева сидений или вебасто `WEBASTO_pin 11`, на плате `OUT4` (опция)

* выход на реле управление сиреной или ЦЗ, на плате `OUT5` (опция)

* выход на сигнальный светодиод `ACTIV_Pin 13` на плате `OUT6`(опция)

* вход `Feedback_Pin A1` - для проверки на момент включенного зажигания с ключа, на плате `FB`

* вход `STOP_Pin A2` - на концевик педали тормоза (АКПП) или на датчик нейтрали в МКПП, на плате `IN2`

* вход `PSO_Pin A3`  - на датчик давления масла, если кому горит (опция), на плате `IN3`

* вход `D2`- для датчиков объема или вибрации (аппаратное прерываение), на плате `IN0` (опция)

* вход `D3` - для подключения к датчику распредвала через оптопару, если кому горит (опция)

* линия `L` - на пин 15 K-line шины в OBDII разъёме, если такова имеется (опция)

* линия `K` - на пин 7 K-line шины в OBDII разъёме, если такова имеется (опция)

* масса `GND`  - она же минус, для шины датчиков температуры DS18B20

* провод `DS18` - на линию опроса вышеупомянутых датчиков, приходит на 4й пин ардуино с подтяжкой к 3.3V

* клемма `3.3V` - напряжение питания датчиков температуры

* клемма `12V` - питание платы через предохранитель на 2А от "постоянного плюса"

* клеммы `REL`, `NO` и ` NC` - входы и выходы  реле для коммутации антенны обходчика иммбилайзера


# Какие функции поддерживает

## 1. Входящий звонок.
  
При входящем звонке с номера `call_phone` "снимает трубку" и проигрывает DTMF-гудок, ожидая ввода команды с клавиатуры телефона;

* ввод `123` включает запуск двигателя с 5-ю попытками и на время прогрева в 10 минут
* ввод `456` включает запуск двигателя с 5-ю попытками и на время прогрева в 20 минут
* ввод `789` останавливает таймер и обнуляет его.
* ввод `*`   затирает ошибочно введенные цифры
* ввод `#`   разрывает соединение и отправляет смс на номер `SMS_phone`

## 2. Исходящее СМС сообщение содержит информацию;

`Privet Vasja Pupkin` - имея сенсора задаваемого в шапке скетча

`Temp.Dvig:  42.05`   - температура датчика DS18B20 расположенного на трубках отопителя салона

`Temp.Salon: 24.01`   - температура датчика DS18B20 расположенного в ногах водителя

`Temp.Ulica: 15.03`   - температура датчика DS18B20 расположенного снаружи автомобиля

`Uptime: 354H`        - время непрерывной работы ардуино в часах

`Vbat: 13.01V`        - напряжение АКБ автомобиля

`Progrev, Timer 8`    - состояние таймера обратного отсчета

`narodmon.ru ON`      - Признак включенной функции отправки данных на [narodmon.ru](narodmon.ru)

`Zahiganie ON`        - Признак включенного зажигания определенное по проводу `Feedback_Pin`

`Popytok: 1`          - Число включения стартера с последнего удачного или неудачного запуска


## 3. Входящие SMS команды

* Присланное СМС с текстом `123start14` запустит двигатель на 14 минут. `123` можно заменить на свой секретный трёхзначные пароль в скетче, двузначное число после `start` устанавливает время прогрева в минутах. 
* СМС со словом `stop` остановит прогрев


## 4. Отправка показаний датчиков на сервер narodmon.ru

Каждые 5 минут открывает GPRS соединение с сервером `narodmon.ru:8283` и отправляет пакет вида:
 
`#80-00-00-XX-XX-XX#SensorName`  - из шапки скетча         

`#Temp1#42.05` - температрура с датчика №1, DS18B20 подключенного на 4 й пин ардуино  

`#Temp2#24.01` - температрура с датчика №2, номер присваивается случайно исходя из серийного номера датчика

`#Temp2#15.03` - температрура с датчика №3, номер присваивается случайно исходя из серийного номера датчика

`#Vbat#13.01`  - Напряжение АКБ, пересчитанное через делитель `m = 66.91;`, 876 значение АЦП 66.91

`#Uptime#7996` - Время непрерывной работы ардуино без перезагрузок, для статистики бесперебойной работы. 

`##`           - Окончание пакета данных.            

Расход трафика до 10 Мб в месяц.

![](https://github.com/martinhol221/SIM800L_DTMF_control/blob/master/img/GPRSTrafic.jpg)


## 5. Прием команд из приложения Народмон 2017

Команды такие же как и при входящем СМС, отличие в том что команда доходит только в момент связи с сервером от 0 до 5 минут, как повезет.

![](https://github.com/martinhol221/SIM800L_DTMF_control/blob/master/img/narodmon2017apk.jpg)

## 6. Автопрогрев

Каждых 3 часа происходит проверка на низкую температуру:

Если температура упала ниже -18 градусов выполняем запуск двигателя на 20 минут тремя попытками.

Если температура упала ниже -25 градусов запуск не выполняем, просто высылаем СМС с температурой.


# Алгоритм запуска: 
После получения команды на запуск, ардуино;

1 Обнуляет счётчик попыток запуска.

2 Задаёт время работы стартера в 1.1 сек для первой попытки.

3 Каллебрует время работы стартера если температура двигателя меньше 15, 5, -5, -10, -15, -30 на значения 1.2, 1.8, 2.2 3.3, 6.0 и 0 сек соответственно своей температуре.

4 Проверяем что бы напряжение АКБ было больше 10 вольт, зажигание с ключа не включено (гарантия что двигатель не работает), и число попыток запуска не достигло максимальных (5-ти попыток).

5 Если предыдущие условие выполненной то включаем реле первого положения замка зажигания , ожидает 0.5 сек.

6 Включаем реле зажигания, ожидаем 4 сек., проверяем не было ли предыдущих неудачных попыток запуска
  
  6.1 Eсли их было 2 и более то дополнительно выключаем/включаем зажигание на 2/8сек

  6.2 Если предыдущих неудачных попыток запуска было 4 и более то дополнительно выключаем/включаем зажигание на 10/8сек

7 Включаем реле стартера установленное время ` StTime ` и выключаем его.

8 Выжидаем 6 сек. на набор аккумулятором напряжения заряда от генератора.

9 Заменяем напряжение АКБ, и если измеренное напряжение выше установленного порога в 13.5 то считаем старт успешным, иначе возвращаемся к пункту 4, и так оставшихся 4 раза.


# Ближайшие планы по прошивке (скетчу):

* Сделать автоподъем трубки на 4-й гудок, если гудков было меньше 4х и звонящий абонент положил трубку раньше то запускаем двигатель.

* Сделать управление запуском и обмен данными с машиной из приложения на андроид, по gprs каналу используя протоколу MQTT.

* Допилить чтение температуры и оборотов двигателя по протоколу K-Line шины для авто поддерживающих ISO 14230-4 KWP 10.4 Kbaud.

* Написать функцию обратного дозвона при срабатывании датчика объем для реализации примитивной сигнализации.

* Добавить возможность подключать дешевый сканер отпечатков пальцев для постановки или снятия с охраны

* Добавить возможность геолокации по GPS данным от базовых станций сотовой связи, с отправкой данных по СМС.

* Добавить возможность чтение даты и времени по информации базовой станции.

* Перевести управление с ArduinoProMini на ESP8266 с заменой прошивки и возможностью обновления через интернет, или с веб страницы и прочими плюшками. 



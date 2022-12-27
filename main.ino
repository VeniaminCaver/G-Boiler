// GoodElectronics
// G-Boiler TurboSleep Monitoring HQ LP
// Предназначен для надёжного наблюдения ключевых параметров котельной 3х3 МВт

// На борту:

// 2 датчика давления 4-20мА
// 1 термопара MAX6675
// до 128 температурных датчиков DS18B20(В данный момент (1)), 
// реле электромеханическое (замкнуто при подаче переменного тока)
// оповещение по SMS через GSM модуль Siemens tc35 (Иногда теряет связь, заменить на SIM900, SIM800L, A6, A7. Цена - 1550 рублей, доставка с АлиЭкспресс -  https://aliexpress.ru/item/1908631967.html?aff_fcid=e62b10d447f54ffdb1fa6b1db90ca11f-1671569805647-08543-cqhFUGv2&aff_fsk=cqhFUGv2&aff_platform=product&sk=cqhFUGv2&aff_trace_key=e62b10d447f54ffdb1fa6b1db90ca11f-1671569805647-08543-cqhFUGv2&terminal_id=ebb90bd7b6ef4e2692fb52d72171dc27&gatewayAdapt=glo2rus&sku_id=59170408699)
// LLC Bastion +79064713801 bastion26.ru



#include <EEPROM.h>
#include <max6675.h>
#include <microDS18B20.h>
#include <LiquidCrystal_I2C.h> 
#include <SoftwareSerial.h>  // Бибилиотеки /librarys

#define PIN_BUTTON_SUMMER 9
#define PIN_BUTTON_WINTER 8 // Две кнопки (ЗИМА/ЛЕТО)

#define PIN_RELAY_INPUT 11 //Мониторим реле, в штатном режиме - не замкнуто, при этом на PIN11 5v, при сработке на пин прийдёт 0

#define PIN_RED_LED 12
#define PIN_BLUE_LED 13 //Красный и синий контакты на RGB

#define T_PERIOD 2000 //Период опроса датчиков 2 секунды
#define BUTTON_PERIOD 10  //Период опроса кнопки 10 миллисекунд

#define STEP_1_PERIOD 1000  //Период опроса кнопки 10 миллисекунд
#define STEP_2_PERIOD 3000  //Период опроса кнопки 10 миллисекунд
#define STEP_3_PERIOD 5000  //Период опроса кнопки 10 миллисекунд

uint32_t buttonTimer = 0; //Таймер для кнопки
uint32_t step1Timer = 0; //Таймер первого шага проверки 
uint32_t step2Timer = 0; //Таймер второго шага проверки
uint32_t step3Timer = 0; //Таймер третьего шага проверки на сработку       

MicroDS18B20<4> sensor; //На 4 пине шина данных DS18B20
SoftwareSerial gsmSerial(10, 11); //RX TX GSM модема

LiquidCrystal_I2C lcd(0x27,16,2); //Экран две строки по 16 символов

const uint8_t     thermoDO  = 5;     
const uint8_t     thermoCS  = 6;                                 
const uint8_t     thermoCLK = 7;    //Термопара на мах6675          
                   
uint8_t           degree[8] = {140,146,146,140,128,128,128,128}; //  Массив хрянящий биты символа градуса

MAX6675           thermo(thermoCLK, thermoCS, thermoDO); //Подрубаем термопару на пины

int counter = 0;      // счётчик
uint32_t timer = 0;   // таймер для ds18b20 (отказаться в пользу более точных на термопаре max6675)

int alarmCode, season, currentValueSummer, currentValueRelay, currentValueWinter, prevValueSummer, prevValueRelay, prevValueWinter, state, temp, temp2, temp3, p2, p3; //Основные переменные

enum state
{
    START,
    BUTTONS,
    SETSEASON,
    WINTER,
    SUMMER,
    SENDSTATUSSMS,
    RESEARCHPT2,
    RESEARCHTT2,
    STEP2,
    STEP3,
    RESEARCHRELAY,
    PREPAIRDATAPT2,
    PREPAIRDATATT2,
    PREPAIRDATATT3,
    PREPAIRDATARELAY,
    SMSSENDER,
    DELAY,
    MAXSTATES           //Состояния программы - основа логики
};

enum aCodes        //aCodes - коды сообщений об ошибке
{
    TT1,       //по температуре подачи на новый корпус
    TT2,       //по температуре обратки со старого корпуса
    TT3,       //по температуре подачи горячей воды потребителям
    PT1,       //по давлению котловой воды
    PT2,       //по давлению в обратке со старого корпуса
    PT3        //по давлению горячей воды сразу после насосов 4.1/4.2  Возможно добавление проверки давления холодной воды и прочих важных контролируемых параметров
};
void setup() { 
  
        state = START; // Определяем начальное состояние, когда начнёт выполняться switch - начнём со START

        Serial.begin(9600); // Через пины 0 и 1 болтаем с соседней платой по сети
        gsmSerial.begin(9600); // порт GSM-модема

        pinMode(PIN_BUTTON_WINTER, INPUT_PULLUP);
        pinMode(PIN_BUTTON_SUMMER, INPUT_PULLUP);
        pinMode(PIN_RELAY_INPUT, INPUT_PULLUP); //слушаем пины кнопок и реле - порты  на ввод с подтяжкой к +

        pinMode(PIN_RED_LED, OUTPUT);
        pinMode(PIN_BLUE_LED, OUTPUT); //светодиоды на вывод
        
        digitalWrite(PIN_RED_LED, 1);
        digitalWrite(PIN_BLUE_LED, 1); // на оба подаём 5v - это занчит выключены, включаются землёй (нулём)

        lcd.init();                     
        lcd.backlight();// Включаем подсветку дисплея 
      //всё это выполнится один раз при включении
}


void loop() {switch (state) {  //всё что ниже будет зациклено с этого места

// Основной цикл реализован на конечном автомате, в каждый момент времени может быть только одно состояние, но в нём можно считывать данные, изменение которых будет переводить систему в другое состояние
// Реализация в коде - через switch. На данный момент программа имеет MAXSTATES состояний, где MAXSTATES - служебное состояние для тестирования внедряемых новых состояний.
// Позже здесь будет добавлена ещё информация по логике работы программы

  

  case START: //состояние START
  
  season = EEPROM.read(10); //читаем данные в десятой ячейке EEPROM, это для определения основного режима работы: зима или лето
  state = BUTTONS; // задаём состояние BUTTONS
  break; 
  
    case BUTTONS: //состояние BUTTONS

    
  
    currentValueSummer = digitalRead(PIN_BUTTON_SUMMER); //тут алгоритм работы кнопок с антидребезгом,
    
     if (currentValueSummer != prevValueSummer) 
     
     {
      
     if (millis() - buttonTimer >= BUTTON_PERIOD) { 
      
          buttonTimer = millis();                   
          currentValueSummer = digitalRead(PIN_BUTTON_SUMMER); 
  }
    
     }
     
    prevValueSummer = currentValueSummer;
    
    
    currentValueWinter = digitalRead(PIN_BUTTON_WINTER);
    
     if (currentValueWinter != prevValueWinter) 
     
     {
     if (millis() - buttonTimer >= BUTTON_PERIOD) { 
      
          buttonTimer = millis();  
     currentValueWinter = digitalRead(PIN_BUTTON_WINTER);
     }
     }
     
    prevValueWinter = currentValueWinter; //тут он закончился, это могло быть время порядка 10мс, в течении которых исполнялся кейс BUTTONS, при этом каждый цикл программа выходила из switch и заходила вновь в кейс BUTTONS

    if (currentValueSummer == 0)      //и вот если алгоритм обнаружил что была нажата кнока ЛЕТО то
    
    {
    Serial.println("Режим лето"); //известили соседнюю плату по UART об этом
    
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("LETO");   
    lcd.setCursor(5, 0); lcd.print( "MODE" );  //написали на экранчике, мол ЛЕТО МОДЕ

    digitalWrite(13, 1); //погасили синий
    digitalWrite(12, 0); //Включили красный диод
    EEPROM.put(10, 1); //записали в ячейку 10 EEPROM число 1.  1 это лето 2 это зима, другие данные - значит ошибка

    state = SUMMER; //идём в SUMMER
    }

    if (currentValueWinter == 0){ //ну а если алгоритм обнаружил что была нажата кнока ЗИМА то
 
    Serial.println("Режим Зима"); //известили соседнюю плату по UART об этом
    
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("ZIMA");   
    lcd.setCursor(5, 0); lcd.print( "MODE" ); //написали на экранчике мол ЗИМА МОДЕ

    digitalWrite(12, 1); //погасили красный
    digitalWrite(13, 0); //включили синий
    EEPROM.put(10, 2); // //записали в ячейку 10 EEPROM число 2.  1 это лето 2 это зима, другие данные - значит ошибка

    state = WINTER; //идём в WINTER
    }

    if (season == 1) 
    {
      state = SUMMER;
      
      }
      if (season == 2)
      {
       state = WINTER; 
        }

       if (season != 1 && season != 2)
       {
        
        state = SETSEASON; // если вдруг окажется, что в ячейке чёто другое лежит, то идём SETSEASON чтобы выбрать режим -  зима или лето
        
       
        }
    break;
    

    case SETSEASON:
    
     if (millis() - timer >= T_PERIOD) { //сюда заходим каждые 2 секунды, чтоб мониторчик не моросил
    timer = millis(); //сбрасываем таймер
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("SELECT MODE:");   //ну и пишем типа Селект Моде, все дела
     
    
    if (season != 1 && season != 2) //если не задано
       {

    lcd.setCursor(0, 1); lcd.print("MODE NO SET!");   //пишем мол НОУ СЕТ
    lcd.setCursor(14, 1); lcd.print(EEPROM.read(10)); //на экранчик то что в ячейке лежит выводим
       }
   
     counter++;  
    if (counter > 30) counter = 0; //что то типа счётчик зацикливает как то по хитрому
    
    
  }
    
    state = START; //идём опять в старт, чтоб автомат перезапустился, и можно было получить инфу с кнопок, если уже установлена, то прога пойдёт по нужному пути

    break;

    case SUMMER: //Основной режим летом, мониторим температуру в контрольных точках, мониторим давление в двух контурах(мне 3 контура нужно мониторить, а в  идеале так 4
    
    digitalWrite(13, 1); //
    digitalWrite(12, 0); //Проверяем чтоб светился красный  диод, а синий был выключен
    
    sensor.requestTemp(); //Запрашиваем у DS18B20 температуру
     if (millis() - timer >= T_PERIOD) //если 2 сек прошло то  
    {
      
    Serial.println("ЛЕТО"); //оповещаем соседа по UART
    Serial.print("Температура теплоносителя Т2 ");
    Serial.println(thermo.readCelsius()); // считываем температуру с термопары и сразу отправляем по UART соседу
    if (sensor.readTemp()) //когда датчик DS18B20 готов отдать температуру
    {
    temp = sensor.getTemp(); //записываем в переменную temp значение температуры на датчике DS18B20
    temp2 = thermo.readCelsius(); //записываем в переменную temp2 значение температуры на термопаре
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("MODE");   
    lcd.setCursor(5, 0); lcd.print( "T2" );   
    lcd.setCursor(8, 0); lcd.print("T3");  
    lcd.setCursor(0, 1); lcd.print("LETO");  
    lcd.setCursor(5, 1); lcd.print( temp2 );  
    lcd.setCursor(8, 1); lcd.print(temp);   //описание команд в /lcd                   
    }
    else Serial.println("error"); //если ds18b20 не подготовил температуру, то отправляем соседу error, здесь же при запуске нужно организовать оповещение, так как это может быть физическое отключение датчика
   
    if (temp2 >= 50)
        {
          state = RESEARCHTT2;

        }
    timer = millis(); // сброс таймера
    
    
    
    counter++;  // прибавляем счётчик
    if (counter > 30) counter = 0;  // закольцовываем изменение
    }
    
       

    state = START;

    break;

    case WINTER:
    digitalWrite(12, 1); 
    digitalWrite(13, 0);
    sensor.requestTemp(); //Запрашиваем у DS18B20 температуру
     if (millis() - timer >= T_PERIOD) //если 2 сек прошло то  
    {
      
    Serial.println("ЗИМА"); //оповещаем соседа по UART
    Serial.print("Температура теплоносителя Т2 ");
    Serial.println(thermo.readCelsius()); // считываем температуру с термопары и сразу отправляем по UART соседу
    if (sensor.readTemp()) //когда датчик DS18B20 готов отдать температуру
    {
    temp = sensor.getTemp(); //записываем в переменную temp значение температуры на датчике DS18B20
    temp2 = thermo.readCelsius(); //записываем в переменную temp2 значение температуры на термопаре
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("MODE");   
    lcd.setCursor(5, 0); lcd.print( "T2" );   
    lcd.setCursor(8, 0); lcd.print("T3");  
    lcd.setCursor(0, 1); lcd.print("ZIMA");  
    lcd.setCursor(5, 1); lcd.print( temp2 );  
    lcd.setCursor(8, 1); lcd.print(temp);   //описание команд в /lcd                   
    }
    else Serial.println("error"); //если ds18b20 не подготовил температуру, то отправляем соседу error, здесь же при запуске нужно организовать оповещение, так как это может быть физическое отключение датчика
 
 if (temp2 <= 50)
        {
          state = RESEARCHTT2;

        } 

    timer = millis(); // сброс таймера
    
    
    
    counter++;  // прибавляем счётчик
    if (counter > 30) counter = 0;  // закольцовываем изменение
    }
    
       
    if (temp2 < 48 || temp2 > 77)
    {
      state = PREPAIRDATATT2;
    }

    if (temp < 60 || temp > 69)
    {
      state = PREPAIRDATATT3;
    }
    state = START;

    break;

    case RESEARCHTT2:

     if (millis() - step1Timer >= STEP_1_PERIOD) 
     { 
          if (temp2 <= 50)
          {
            state = STEP2;
          }
          else
          state = START;

          step1Timer = millis();                   
          
      }


    break;

    case STEP2:

     if (millis() - step2Timer >= STEP_2_PERIOD) 
     { 
          if (temp2 <= 50)
          {
            state = STEP3;
          }
          else
          state = START;

          step2Timer = millis();                   
          
      }


    break;

    case STEP3:

     if (millis() - step3Timer >= STEP_3_PERIOD) 
     { 
          if (temp2 <= 50)
          {
            state = PREPAIRDATATT2;
          }
          else
          state = START;

          step3Timer = millis();                   
          
      }


    break;

    case PREPAIRDATATT2:

     temp2 = thermo.readCelsius();
     aCodes = TT2; //!!! почему то не хочет присваивать aCodes TT2 (должен присвоить число 1 из енумса эйкодес
     state = SMSSENDER;



    break;


  case SMSSENDER:

     switch (aCodes) 
     {

            case TT1:
            break;

            case TT2:

             gsmSerial.print("AT+CMGF=1\r");
             delay(100);
              gsmSerial.println("AT+CMGS=\"+71234567890\"");
              delay(100);
              gsmSerial.print("Temp: ");
              gsmSerial.print(temp2);
              gsmSerial.println(" C");
              delay(100);
              gsmSerial.println((char)26);

              state = DELAY;
            
            break;

            case TT3:
            break;

            case PT1:
            break;

            case PT2:
            break;

            case PT3;
            break;
     }



    break;

    case DELAY:

    

    break;
 }

 
}

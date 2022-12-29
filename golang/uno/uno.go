package main

//импортируем библиотеки
import (
	"machine"
	"time"
)

//run main programm
func main() {

	//конфигурируем пины ардуины - берем светодиод 13 выход вроде бы 
	Led := machine.LED
	//конфигурируем светодиод на выход (чтобы подавать на него сигнал)
	Led.Configure(machine.PinConfig{Mode: machine.PinOutput})

	//задаем переменную выбора заданий (по умолчанию допустим LOW)
  var currentBoilerTask string = "LOW"

	//запускаем бесконечный цикл
	for {
		//переключаемся в зависимости от значения переменной currentBoilerTask 
		switch taskName := currentBoilerTask; taskName {
		case "HIGH":
			//если задание называется зажечь светодиод - "HIGH"
			println("received ", taskName)
			//зажигаем светодиод
			Led.High()
			//спим 1 секунду
			time.Sleep(1 * time.Second)
			//прыгаем в задание с названием LOW
			currentBoilerTask = "LOW"
		case "LOW":
			//если задание называется потушить светодиод "LOW"
			println("received ", taskName)
			//тушим светодиод
			Led.Low()
			//спим 1 секунду
			time.Sleep(1 * time.Second)
			//прыгаем в задание с названием HIGH
			currentBoilerTask = "HIGH"
			//
		}
	}
}

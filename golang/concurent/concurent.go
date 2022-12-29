package main

import (
	"machine"
	"time"
)

// максимальное количество параллельных выполнений:
var boilerWorkersMaxCount int = 2

// канал блокировщик параллельных выполнений
var respawnLock chan int

// канал с заданиями
var BoilerTask chan string


//run main programm
func main() {

	//initialise channel with tasks:
	BoilerTask = make(chan string)

	//initialize unblocking channel to guard respawn tasks
	respawnLock = make(chan int, boilerWorkersMaxCount)

	go func() {
		for {
			// will block if there is boilerWorkersMaxCount ints in respawnLock
			respawnLock <- 1
			//sleep 1 second
			time.Sleep(1 * time.Second)
			//запускаем обработчик заданий:
			go boilerWorkerRun(len(respawnLock))
		}
	}()

	for {
		BoilerTask <- "HIGH"
		time.Sleep(1 * time.Second)
		BoilerTask <- "LOW"
		time.Sleep(1 * time.Second)
	}
}

func boilerWorkerRun(workerId int) {
	Led := machine.LED
	Led.Configure(machine.PinConfig{Mode: machine.PinOutput})
	println("Started boiler worker #", workerId)
	for {
		select {
			//в случае если есть задание в канале BoilerTask
		case currentBoilerTask := <-BoilerTask:
			switch taskName := currentBoilerTask; taskName {
			case "HIGH":
				//если задание называется зажечь светодиод - "HIGH"
				println("received ", taskName, "and processed by worker #", workerId)
				Led.High()
				//
			case "LOW":
				//если задание называется потушить светодиод "LOW"
				println("received ", taskName, "and processed by worker #", workerId)
				Led.Low()
				//
			}
		}
	}
}

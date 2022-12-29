https://tinygo.org/docs/reference/microcontrollers/

## build/flash for arduino-mega2560
```
sudo tinygo build -target arduino-mega2560 -scheduler tasks concurent.go
sudo tinygo flash -target arduino-mega2560 -scheduler tasks concurent.go
```

## build/flash for esp8266 nodemcu
```
sudo tinygo build -target nodemcu concurent.go
sudo tinygo flash -target nodemcu concurent.go
```

https://tinygo.org/docs/reference/microcontrollers/

## flash for arduino arduino-mega2560
```
sudo tinygo build -target arduino uno.go
sudo tinygo flash -target arduino uno.go

sudo tinygo build -target arduino-mega2560 uno.go
sudo tinygo flash -target arduino-mega2560 uno.go
```

## build/flash for esp8266 nodemcu
```
sudo tinygo build -target nodemcu uno.go
sudo tinygo flash -target nodemcu uno.go
```

TARGET=lonet

CC = g++
LD = ld

INCLUDE=inc/
RPI="pi@192.168.0.145"
PATH_ON_RPI=/home/pi/gsm

CFLAGS = -g -O0 -I$(INCLUDE) -std=c++11
LDFLAGS = -lpthread

HEADERS = 
OBJECTS = src/rpi_gsm.o src/Gpio.o src/LinuxGpio.o src/Sms.o src/Serial.o

RPI_IP = 192.168.0.145
RPI_USER=pi
RPI_DIR=/home/pi/gsm

default: $(TARGET)

%.o: %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

sync:
	scp -r Makefile inc src $(RPI_USER)@$(RPI_IP):$(RPI_DIR)
	ssh  "$(RPI)" -t "make -C $(PATH_ON_RPI)"
	

clean:
	-rm -f $(OBJECTS)
	-rm -f $(TARGET)
#ifndef ARDUINO
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/ioctl.h>

typedef unsigned char byte;

class TwoWire {
    public:
    TwoWire();
    ~TwoWire();
    void beginTransmission(byte address);
    uint8_t endTransmission();
    uint8_t endTransmission(int sendStop);
    bool begin(int sda, int scl, uint32_t speed);
    bool setClock(uint32_t speed);
    uint8_t requestFrom(uint8_t address, uint8_t size, uint8_t sendStop);
    size_t write(uint8_t);
    int available();
    int read();
};

class DummySerial {
    public:
        DummySerial();
        ~DummySerial();
        int begin(uint32_t baud);
        int printf(const char *fmt, ...);
        int print(const char *s);
        int println(const char *s);
        int available();
        uint8_t read();
        size_t read(char *buf, int max_bytes);
};


TwoWire::TwoWire() {

}

TwoWire::~TwoWire() {

}
DummySerial::DummySerial() {

}
DummySerial::~DummySerial() {
    
}

int DummySerial::begin(uint32_t baud) {
    return(0);
}

int DummySerial::printf(const char *fmt, ...) {
  char buf[512];
  memset((void *) buf, 0, sizeof(buf));
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf) - 1, fmt, args);
  va_end(args);
  ::printf("%s", buf);
  return(0);
}

int DummySerial::print(const char *s) {
  ::printf("%s", s);
  return(0);
}

int DummySerial::println(const char *s) {
  ::printf("%s\n", s);
  return(0);
}

int DummySerial::available() {
    int num_bytes;
    ioctl(0, FIONREAD, &num_bytes);
    return(num_bytes);
}

uint8_t DummySerial::read() {
    static char b;
    ssize_t bytes_read;
    bytes_read = ::read(0, &b, 1);
    if (bytes_read) return((uint8_t) b);
    else return(0);
}
size_t DummySerial::read(char *buffer, int max_bytes) {
    ssize_t bytes_read;
    
    bytes_read = ::read(0, buffer, max_bytes);
    if (bytes_read < 0) {
        printf("::SERIALWRAPPER ERROR READING!!!!!!\n");
    }
    return(bytes_read);
}

void TwoWire::beginTransmission(byte address) {
    return;
}

uint8_t TwoWire::endTransmission() {
    return(0);
}

uint8_t TwoWire::endTransmission(int sendStop) {
    return(0);
}

bool TwoWire::begin(int sda, int scl, uint32_t speed) {
    return(1);
}

bool TwoWire::setClock(uint32_t speed) {
    return(1);
}
  
uint8_t TwoWire::requestFrom(uint8_t address, uint8_t size, uint8_t sendStop) {
    return(0);
}

size_t TwoWire::write(uint8_t) {
    return(0);
}

int TwoWire::available() {
    return(0);
}

int TwoWire::read() {
    return(0);
}

int delay(uint32_t useconds) {
    usleep(useconds);
}
extern void setup();
extern void loop();
int main(int argc, char *argv[]) {
    setup();
    while (1) { loop(); }
}
TwoWire Wire;
DummySerial Serial;
#endif
#ifndef ARDUINO
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
extern TwoWire Wire;
extern DummySerial Serial;
extern int delay(uint32_t useconds);
#endif
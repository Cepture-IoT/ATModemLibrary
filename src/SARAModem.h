
#ifndef _SARAMODEM_H_INCLUDED
#define _SARAMODEM_H_INCLUDED

#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>

enum ReadResponseResultEnum {
    READ_TIMEOUT,
    READ_OK,
    READ_ERROR,
    READ_NO_CARRIER,
    READ_CME_ERROR
};
enum BeginResultEnum{
    BEGIN_SUCCESS
};
class SARAModem{
    public:
        SARAModem(HardwareSerial &sara_serial, int baudrate, int power_pin, int reset_pin, bool echo);
        void on();
        BeginResultEnum begin();
        //reads the response from the sara module, if echo detected skips echo, stores everything after echo other than the OK/ERROR/whatever at the end
        ReadResponseResultEnum readResponse(String &response_buffer, unsigned long time_out, bool wait_for_response = false, unsigned long lag_timeout = 50);
        bool echo_enabled;
        
        size_t write(uint8_t c);
        size_t write(const uint8_t*, size_t);

        void send(const char* command);
        void send(const String& command) { send(command.c_str()); }
        void sendf(const char *fmt, ...);

        bool available();
        char read();
    private:
        HardwareSerial* sara_serial;
        int baudrate;
        int power_pin;
        int reset_pin;

        String read_buffer;
};
#endif
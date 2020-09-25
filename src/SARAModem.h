
#ifndef _SARAMODEM_H_INCLUDED
#define _SARAMODEM_H_INCLUDED

#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>

#define MODEM_BUFFER_SIZE 1024
#define USE_POWER_CTRL_PIN true
#define POWER_CTRL_OFF_IS_LOW true
#define POWER_PIN_HIGH_NORMAL true
#define USE_V_INT_PIN true
enum ReadResponseResultEnum {
    READ_TIMEOUT,
    READ_OK,
    READ_ERROR,
    READ_NO_CARRIER,
    READ_CME_ERROR,
    READ_BUFFER_TOO_SMALL
};
enum BeginResultEnum{
    BEGIN_SUCCESS
};
int lastStrStr(char* base_str, char* in_str);
int lastStrStr(char* base_str, size_t base_len, char* in_str, size_t in_len);
class SARAModem{
    public:
        SARAModem(HardwareSerial &sara_serial, int baudrate, int power_pin, int reset_pin, int power_control_pin, int v_int_pin, bool echo);
        SARAModem(HardwareSerial &sara_serial, int baudrate, int power_pin, int reset_pin, bool echo);
        void on();
        void off();
        BeginResultEnum begin();
        //reads the response from the sara module, if echo detected skips echo, stores everything after echo other than the OK/ERROR/whatever at the end
        ReadResponseResultEnum readResponse(char* response_buffer, size_t size, bool wait_for_response = false, unsigned long time_out = 1000);
        bool echo_enabled;
        
        size_t write(uint8_t c);
        size_t write(const uint8_t*, size_t);

        void send(const char* command);
        void send(const String& command) { send(command.c_str()); }
        void sendf(const char *fmt, ...);

        bool available();
        char read();
        int echo(bool on);
    private:
        HardwareSerial* sara_serial;
        int baudrate;
        int power_pin;
        int reset_pin;
        #if USE_POWER_CTRL_PIN
            int pwr_ctrl_pin;
        #endif
        #if USE_V_INT_PIN
            int v_int_pin;
        #endif

        //String read_buffer;
        char read_buffer[MODEM_BUFFER_SIZE];
};
#endif
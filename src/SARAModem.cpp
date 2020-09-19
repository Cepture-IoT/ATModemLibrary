#include "SARAModem.h"

SARAModem::SARAModem(HardwareSerial &sara_serial, int baudrate, int power_pin, int reset_pin, bool echo):
    sara_serial(&sara_serial),
    baudrate(baudrate),
    power_pin(power_pin),
    reset_pin(reset_pin),
    echo_enabled(echo)
    {
        read_buffer.reserve(256);
}
BeginResultEnum SARAModem::begin(){
    on();
    sara_serial->begin(baudrate);
    return BEGIN_SUCCESS;
}
void SARAModem::on(){
    // enable the POW_ON pin
    pinMode(power_pin, OUTPUT);
    digitalWrite(power_pin, HIGH);
    //reset pin shouldnt be used using this
    digitalWrite(reset_pin, LOW);
    // // reset the ublox module
    // pinMode(reset_pin, OUTPUT);
    // digitalWrite(reset_pin, HIGH);
    // delay(100);
    // digitalWrite(reset_pin, LOW);
}
void SARAModem::off(){
    sara_serial->end();
    // digitalWrite(reset_pin, HIGH);

    // power off module
    digitalWrite(power_pin, LOW);
}
ReadResponseResultEnum SARAModem::readResponse(String &buffer, unsigned long timeout, bool wait_for_response, unsigned long lag_timeout){
    int responseResultIndex = -1;
    unsigned long start_time = millis();
    // Serial.println("############################");
    //wait for response if told too
    while(wait_for_response && !sara_serial->available()){
        if(millis()-start_time >= timeout){
            // Serial.println("TIMEOUT");
            return READ_TIMEOUT;
        }
    };
    read_buffer = "";
    char c = '0';
    ReadResponseResultEnum result = READ_ERROR;
    while(sara_serial->available()){
        char c = sara_serial->read();
        read_buffer += c;

        //if newline or carriage return then a message has ended
        if(c == '\n'){
            // Serial.print(result);
            // Serial.print(" ");
            // Serial.println(read_buffer);
            //check if echo
            responseResultIndex = read_buffer.lastIndexOf("AT");
            if(responseResultIndex != -1){
                //echo has occured so empty buffer
                read_buffer = "";
                continue;
            }
            //check for special response
            //is it OK
            responseResultIndex = read_buffer.lastIndexOf("OK");
            if(responseResultIndex != -1){
                result = READ_OK;
                read_buffer = "";
                continue;
            }
            //is it CME error
            responseResultIndex = read_buffer.lastIndexOf("CME ERROR");
            if(responseResultIndex != -1){
                result = READ_CME_ERROR;
                read_buffer = "";
                continue;
            }
            //is it an error
            responseResultIndex = read_buffer.lastIndexOf("ERROR");
            if(responseResultIndex != -1){
                result = READ_ERROR;
                read_buffer = "";
                continue;
            }
            //is it no carrier
            responseResultIndex = read_buffer.lastIndexOf("NO CARRIER");
            if(responseResultIndex != -1){
                result = READ_NO_CARRIER;
                read_buffer = "";
                continue;
            }

            //add to buffer with new lines and stuff incase a second thing comes in
            // Serial.println("END");
            buffer += read_buffer;
            read_buffer = "";
        }
        //the board can read too fast for serial data to show up so wait for a period of time if no data is available incase its just lag
        start_time = millis();
        while(!sara_serial->available() && millis() - start_time <= lag_timeout){
            delay(10);
        }
    }
    return result;
}

size_t SARAModem::write(uint8_t c)
{
  return sara_serial->write(c);
}

size_t SARAModem::write(const uint8_t* buf, size_t size)
{
  size_t result = sara_serial->write(buf, size);

  // the R410m echos the binary data, when we don't what it to so
  if(echo_enabled){
    size_t ignoreCount = 0;

    while (ignoreCount < result) {
        if (sara_serial->available()) {
        sara_serial->read();

        ignoreCount++;
        }
    }
  }

  return result;
}

void SARAModem::send(const char* command)
{
//   // compare the time of the last response or URC and ensure
//   // at least 20ms have passed before sending a new command
//   unsigned long delta = millis() - _lastResponseOrUrcMillis;
//   if(delta < MODEM_MIN_RESPONSE_OR_URC_WAIT_TIME_MS) {
//     delay(MODEM_MIN_RESPONSE_OR_URC_WAIT_TIME_MS - delta);
//   }

  sara_serial->println(command);
  sara_serial->flush();
}

void SARAModem::sendf(const char *fmt, ...)
{
  char buf[BUFSIZ];

  va_list ap;
  va_start((ap), (fmt));
  vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
  va_end(ap);

  send(buf);
}
bool SARAModem::available(){
    return sara_serial->available();
}
char SARAModem::read(){
    return sara_serial->read();
}
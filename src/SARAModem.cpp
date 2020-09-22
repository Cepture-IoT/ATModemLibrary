#include "SARAModem.h"

size_t lastStrStr(char* base_str, char* in_str){
    return lastStrStr(base_str,strlen(base_str),in_str,strlen(in_str));
}
for (char *p = buffer; p <= buffer + fromIndex; p++) {
    p = strstr(p, s2.buffer);
    if (!p) break;
    if ((unsigned int)(p - buffer) <= fromIndex) found = p - buffer;
}
size_t lastStrStr(char* base_str, size_t base_len, char* in_str, size_t in_len){
    int cur_loc = base_len - in_len;
    if(cur_loc < 0) return -1;
    for(int loc = cur_loc; loc >=0 ; loc--){
        char* tmp_c = base_str+loc;
        char* tmp_r = strstr(tmp_c,in_str);
        if(tmp_r == tmp_c){
            return loc;
        }

    }
    return -1;
}
SARAModem::SARAModem(HardwareSerial &sara_serial, int baudrate, int power_pin, int reset_pin, bool echo):
    sara_serial(&sara_serial),
    baudrate(baudrate),
    power_pin(power_pin),
    reset_pin(reset_pin),
    echo_enabled(echo){

    }
//     {
//         read_buffer.reserve(256);
// }
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
    // digitalWrite(reset_pin, LOW);
    // reset the ublox module
    pinMode(reset_pin, OUTPUT);
    digitalWrite(reset_pin, HIGH);
    delay(100);
    digitalWrite(reset_pin, LOW);
}
void SARAModem::off(){
    sara_serial->end();
    // digitalWrite(reset_pin, HIGH);

    // power off module
    digitalWrite(power_pin, LOW);
}
ReadResponseResultEnum SARAModem::readResponse(char &buffer,size_t buf_length, unsigned long timeout, bool wait_for_response, unsigned long lag_timeout){
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
    read_buffer[0] = '\0';
    char c = '0';
    ReadResponseResultEnum result = READ_ERROR;
    size_t read_buffer_ind = 0;
    bool exceeded = false;
    while(sara_serial->available()){
        char c = sara_serial->read();
        // read_buffer += c;
        read_buffer[read_buffer_ind] = c;
        //dont let the buffer index grow larger than the buffer
        read_buffer_ind++;
        if(read_buffer_ind >= buf_length-1){
            exceeded = true;
            read_buffer_ind -= 1;
        }
        
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
    read_buffer[read_buffer_ind+1] = '\0';
    if(exceeded){
        return READ_BUFFER_TOO_SMALL;
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
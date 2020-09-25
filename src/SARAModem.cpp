#include "SARAModem.h"

int lastStrStr(char* base_str, char* in_str){
    return lastStrStr(base_str,strlen(base_str),in_str,strlen(in_str));
}

int lastStrStr(char* base_str, size_t base_len, char* in_str, size_t in_len){
    int cur_loc = base_len - in_len;
    if(cur_loc < 0) return -1;
    for(char* c = base_str+cur_loc; c >= base_str ; c--){
        if(c == strstr(c,in_str)){
            return c-base_str;
        }
    }
    return -1;
}
SARAModem::SARAModem(HardwareSerial &sara_serial, int baudrate, int power_pin, int reset_pin, bool echo){
    SARAModem(sara_serial, baudrate, power_pin, reset_pin, -1, -1, echo);
}
SARAModem::SARAModem(HardwareSerial &sara_serial, int baudrate, int power_pin, int reset_pin, int power_control_pin, int v_int_pin, bool echo):
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
    #if POWER_PIN_HIGH_NORMAL
        digitalWrite(power_pin, LOW);
        delay(200);
        digitalWrite(power_pin,HIGH);
    #else
        digitalWrite(power_pin, HIGH);
        delay(200);
        digitalWrite(power_pin, LOW);
    #endif
    //reset pin shouldnt be used using this
    // digitalWrite(reset_pin, LOW);
    // reset the ublox module
    // pinMode(reset_pin, OUTPUT);
    // digitalWrite(reset_pin, HIGH);
    // delay(100);
    // digitalWrite(reset_pin, LOW);
    #if USE_V_INT_PIN
        while(true){
            if(digitalRead(v_int_pin) == HIGH){
                break;
            }
            //chose 250ms so it can deep sleep a little extra
            delay(250);
        }
    #else
        //as we arent checking for V_INT being low we need to have a delay so we dont damage the module
        delay(5000);
    #endif
}
void SARAModem::off(){
    sara_serial->end();
    // digitalWrite(reset_pin, HIGH);

    // power off module
    digitalWrite(power_pin, HIGH);
    

    //power enable off
    #if USE_POWER_CTRL_PIN
        #if USE_V_INT_PIN
            while(true){
                if(digitalRead(v_int_pin) == LOW){
                    break;
                }
                //chose 250ms so it can deep sleep a little extra
                delay(250);
            }
        #else
            //as we arent checking for V_INT being low we need to have a delay so we dont damage the module
            delay(5000);
        #endif
        if(pwr_ctrl_pin != -1){
            #if POWER_CTRL_OFF_IS_LOW
                digitalWrite(pwr_ctrl_pin,LOW);
            #else
                digitalWrite(pwr_ctrl_pin,HIGH);
            #endif
        }
    #endif
}
ReadResponseResultEnum SARAModem::readResponse(char* buffer, size_t _size, bool wait_for_response, unsigned long timeout){
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
    bool echod = false;
    while(sara_serial->available()){
        char c = sara_serial->read();
        // read_buffer += c;
        read_buffer[read_buffer_ind] = c;
        //dont let the buffer index grow larger than the buffer
        read_buffer_ind++;
        if(read_buffer_ind >= MODEM_BUFFER_SIZE-1){
            exceeded = true;
            read_buffer_ind -= 1;
        }
        
        //if newline or carriage return then a message has ended
        if(c == '\n'){
            read_buffer[read_buffer_ind+1] = '\0';
            // Serial.println("###########");
            // Serial.print(result);
            // Serial.print(" ");
            // Serial.println(read_buffer);
            // Serial.println(read_buffer_ind);
            
            //check if echo
            //responseResultIndex = read_buffer.lastIndexOf("AT");
            responseResultIndex = lastStrStr(read_buffer,"AT");
            if(responseResultIndex != -1){
                //echo has occured so empty buffer
                read_buffer[0] = '\0';
                read_buffer_ind = 0;
                echod = true;
                continue;
            }
            //check for special response
            //is it OK
            //responseResultIndex = read_buffer.lastIndexOf("OK");
            responseResultIndex = lastStrStr(read_buffer,"OK");
            if(responseResultIndex != -1){
                result = READ_OK;
                read_buffer[0] = '\0';
                read_buffer_ind = 0;
                continue;
            }
            //is it CME error
            //responseResultIndex = read_buffer.lastIndexOf("CME ERROR");
            responseResultIndex = lastStrStr(read_buffer,"CME ERROR");
            if(responseResultIndex != -1){
                result = READ_CME_ERROR;
                read_buffer[0] = '\0';
                read_buffer_ind = 0;
                continue;
            }
            //is it an error
            //responseResultIndex = read_buffer.lastIndexOf("ERROR");
            responseResultIndex = lastStrStr(read_buffer,"ERROR");
            if(responseResultIndex != -1){
                result = READ_ERROR;
                read_buffer[0] = '\0';
                read_buffer_ind = 0;
                continue;
            }
            //is it no carrier
            //responseResultIndex = read_buffer.lastIndexOf("NO CARRIER");
            responseResultIndex = lastStrStr(read_buffer,"NO CARRIER");
            if(responseResultIndex != -1){
                result = READ_NO_CARRIER;
                read_buffer[0] = '\0';
                read_buffer_ind = 0;
                continue;
            }

            //add to buffer with new lines and stuff incase a second thing comes in
            //TODO: Maybe fixed overflow writing
            strncat(buffer,read_buffer,size-strlen(buffer));
            read_buffer[0] = '\0';
            read_buffer_ind = 0;
        }
        //if the system does echo then we want to wait still as the OK may take a while to come, if
        //echo isnt enabled then the wait_for_response thing at the start will only start when the 
        //reply from the command returns. Eg, OK.
        if(echo_enabled && echod){
            if(millis()-start_time >= timeout){
                // Serial.println("TIMEOUT");
                return READ_TIMEOUT;
            }
        }
        //the board can read too fast for serial data to show up so wait for a period of time if no data is available incase its just lag
        unsigned long lag_start = millis();
        while(!sara_serial->available() && millis() - lag_start <= READ_LAG_TIMEOUT){
            delay(10);
        }
    }
    
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
    // Serial.println(command);
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
int SARAModem::echo(bool on){
    char* echo_command = "ATE1";
    if(!on){
        echo_command[3] = '0';
    }
    send(echo_command);
    return readResponse(NULL, true, 1000);
}

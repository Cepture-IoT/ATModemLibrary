#include "ModemClient.h"
#define __XSI_VISIBLE 1
#include <time.h>
ModemClient::ModemClient(SARAModem &modem, int buffer_size){
    this->modem = &modem;
    _buffer.reserve(buffer_size);
}
bool ModemClient::setSocket(int socket){
    char param_1[10];
    int param_2;
    _buffer = "";
    int val = socketQuery(socket, SQUERY_TYPE,param_1, param_2);
    if(val == -1){
        return false;
    }
    _current_socket = socket;
    return true;
}
size_t ModemClient::write(uint8_t c){
    char tmp[] = "0";
    tmp[0] = (char)c;
    int ret_val = socketWriteTCP(_current_socket,(char*)tmp,(size_t)1);
    if(ret_val < 0){
        return -ret_val;
    }else{
        return ret_val;
    }
}
size_t ModemClient::write(const uint8_t* buf, size_t size){

    int ret_val = socketWriteTCP(_current_socket,(char*) buf,(size_t)size);
    if(ret_val < 0){
        return -ret_val;
    }else{
        return ret_val;
    }
}

int ModemClient::available(){
    char tmp[10];
    int val = socketReadTCP(_current_socket, tmp, 0);
    if(val == -1){
        return 0;
    }
    return val;
}
int ModemClient::read(){
    uint8_t buffer[1];
    int val = read(buffer,1);
    if(val == -1){
        return -1;
    }else
    {
        return (int)buffer[0];
    }
    
}
int ModemClient::read(uint8_t *buf, size_t size){
    int val = socketReadTCP(_current_socket, (char*)buf, size);
    return val;
}
int ModemClient::peek(){
    return -1;
}
void ModemClient::flush(){
    int read_num = -1;
    char buf[128];
    while(read_num > 0){
        read_num = socketReadTCP(_current_socket,buf,128);
    }

}
int ModemClient::connect(IPAddress ip, uint16_t port){
    char host[17] = "255.255.255.255\0";
    snprintf(host,17,"%d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);
    return connect((const char*)host,port);
}
int ModemClient::connect(const char *host, uint16_t port){
    int val = socketConnect(_current_socket, host, port);
    if(val < 0){
        return 0;
    }
    return val;
}
void ModemClient::stop(){
    int val = socketClose(_current_socket, true);
    _current_socket = -1;
}
uint8_t ModemClient::connected(){
    char tmp[10];
    int tmp2;
    int val = socketQuery(_current_socket,SQUERY_TCP_STATUS, tmp, tmp2);
    //reuse tmp2 
    int status;
    tmp2 = sscanf(tmp,"%d",&status);
    if(tmp2 != 1){
        return false;
    }
    if(status != TCPSTATUS_ESTABLISHED){
        return false;
    }
    return true;
}
ModemClient::operator bool(){

}

int ModemClient::commandSmartSend(char *command, String &buffer,int attempts, int timeout, bool wait, unsigned long lag_timeout){
    int attempt = 0;
   
    while(attempt < attempts){
        modem->send(command);
        ReadResponseResultEnum read_result = modem->readResponse(buffer,timeout,wait,lag_timeout);
        if(read_result == READ_OK){
            break;
        }else if(read_result != READ_TIMEOUT){
            //return as some error occured in sending
            switch(read_result){
                case READ_ERROR:
                    last_error = CE_ERROR;
                    break;
                case READ_NO_CARRIER:
                    last_error = CE_NO_CARRIER;
                    break;
                case READ_CME_ERROR:
                    last_error = CE_CME_ERROR;
                    break;
            }
            // Serial.println("OTHER ERROR");
            // Serial.println(last_error);
            return -1;
        }else{
            attempt++;
        }
    }
    if(attempt >= attempts){
        last_error = CE_TIMEOUT;
        // Serial.println("TIMEOUT");
        return -1;
    }
    // Serial.print("BUF: ");
    // Serial.println(buffer);
    return 0;
}
int ModemClient::commandSmartRead(String &buffer,int attempts, int timeout, bool wait){
    int attempt = 0;
   
    while(attempt < attempts){
        ReadResponseResultEnum read_result = modem->readResponse(buffer,timeout,wait);
        if(read_result == READ_OK){
            break;
        }else if(read_result != READ_TIMEOUT){
            //return as some error occured in sending
            switch(read_result){
                case READ_ERROR:
                    last_error = CE_ERROR;
                    break;
                case READ_NO_CARRIER:
                    last_error = CE_NO_CARRIER;
                    break;
                case READ_CME_ERROR:
                    last_error = CE_CME_ERROR;
                    break;
            }
            // Serial.println("OTHER ERROR");
            // Serial.println(last_error);
            return -1;
        }else{
            attempt++;
        }
    }
    if(attempt >= attempts){
        last_error = CE_TIMEOUT;
        // Serial.println("TIMEOUT");
        return -1;
    }
    // Serial.print("BUF: ");
    // Serial.println(buffer);
    return 0;
}
bool ModemClient::waitForBytePrompt(unsigned long timeout){
    unsigned long start_time = millis();
    while(!modem->available()){
        if(millis()-start_time >= timeout){
            last_error = CE_BYTE_PROMPT_TIMEOUT;
            return false;
        }
    };
    while(modem->available()){
        char c = modem->read();
        if(c == '@'){
            return true;
        }
        if(!modem->available()){
            delay(10);
        }
    }
    last_error = CE_BYTE_PROMPT_NOT_FOUND;
    return false;

    
}
int ModemClient::socketCreate(bool use_tcp, int port){
    char command[50];
    _buffer = "";
    int protocol = use_tcp ? 6 : 17;
    snprintf(command,50, "AT+USOCR=%d,%d",protocol, port);

    int val = commandSmartSend(command,_buffer,10,COMMAND_TIMEOUT ,true);
    if(val == -1){
        return -1;
    }

    int URC_start = _buffer.lastIndexOf("+USOCR:");
    // Serial.println("");
    // Serial.println(_buffer);
    // Serial.println(URC_start);
    // Serial.println("HERE2");
    //check if expected reply exists
    if(URC_start == -1){
        //reply not found
        last_error = CE_REPLY_NOT_FOUND;
        return -1;
    }
    String reply_section = _buffer.substring(URC_start);
    int socket = -1;
//    return 0;
    int sscanf_result = sscanf(reply_section.c_str(),"+USOCR: %d",&socket);
    //  Serial.print("SCAN ");
    //  Serial.println(sscanf_result);

    //if sscanf finds nothing or socket is out of range
    if(sscanf_result != 1 || socket < SOCKET_MIN || socket > SOCKET_MAX){
        //expected reply value doesnt exists
        last_error = CE_REPLY_VALUE_INVALID;
        return -1;
    }
    return socket;
}
int ModemClient::socketQuery(int socket, int query_type,char* param_1, int &param_2){
    char command[50];
    _buffer = "";
    snprintf(command,50, "AT+USOCTL=%d,%d",socket, query_type);

    int val = commandSmartSend(command,_buffer,10,COMMAND_TIMEOUT ,true);
    if(val == -1){
        return -1;
    }
    int URC_start = _buffer.lastIndexOf("+USOCTL:");
    String reply_section = _buffer.substring(URC_start);

    int sscanf_result = sscanf(reply_section.c_str(),"+USOCTL: %*d, %*d, %s, %d",param_1,&param_2);
    // Serial.print("SCAN ");
    // Serial.println(sscanf_result);

    //if sscanf finds less than 1 value
    if(sscanf_result < 1){
        //expected reply value doesnt exists
        last_error = CE_REPLY_VALUE_INVALID;
        return -1;
    }
    return sscanf_result;
}

int ModemClient::socketClose(int socket, bool async){
    char command[50];
    _buffer = "";
    snprintf(command,50, "AT+USOCL=%d,%d",socket,async);

    int val = commandSmartSend(command,_buffer,10,COMMAND_TIMEOUT ,true, 10000);
    if(val == -1){
        return -1;
    }
    return 1;
    // int URC_start = _buffer.lastIndexOf("+USOCTL:");
    // String reply_section = _buffer.substring(URC_start);

    // int sscanf_result = sscanf(reply_section.c_str(),"+USOCTL: %*d, %*d, %s, %d",param_1,&param_2);
    // Serial.print("SCAN ");
    // Serial.println(sscanf_result);

    // //if sscanf finds less than 1 value
    // if(sscanf_result < 1){
    //     //expected reply value doesnt exists
    //     last_error = CE_REPLY_VALUE_INVALID;
    //     return -1;
    // }
    // return sscanf_result;
}
int ModemClient::socketConnect(int socket, const char* address, int port){
    char command[50];
    _buffer = "";
    snprintf(command,50, "AT+USOCO=%d,\"%s\",%d",socket, address, port);

    int val = commandSmartSend(command,_buffer,10,10000 ,true,10000);
    if(val == -1){
        return -1;
    }
    return 1;
}

int ModemClient::socketWriteTCP(int socket, const char* buffer, size_t buffer_length){
    //char command[16+SOCKET_WRITE_MAX_SIZE];
    String command;
    command.reserve(19+(buffer_length < SOCKET_WRITE_MAX_SIZE ? buffer_length : SOCKET_WRITE_MAX_SIZE)*2);

    int written = 0;
    //split buffer into chunks, mostly stolen from NBMKR
    while(buffer_length){
        size_t chunkSize = buffer_length;

        if (chunkSize > SOCKET_WRITE_MAX_SIZE) {
            chunkSize = SOCKET_WRITE_MAX_SIZE;
        }

        command = "AT+USOWR=";
        command += socket;
        command += ",";
        command += (uint16_t)chunkSize;
        command += ",\"";

        for (size_t i = 0; i < chunkSize; i++) {
        byte b = buffer[i + written];

        byte n1 = (b >> 4) & 0x0f;
        byte n2 = (b & 0x0f);

        command += (char)(n1 > 9 ? 'A' + n1 - 10 : '0' + n1);
        command += (char)(n2 > 9 ? 'A' + n2 - 10 : '0' + n2);
        }

        command += "\"";

        int val = commandSmartSend((char*)command.c_str(),_buffer,10,COMMAND_TIMEOUT,true);
        if(val == -1){
            return -written;
        }
        written += chunkSize;
        buffer_length -= chunkSize;
    }
    return written;
}
int ModemClient::socketWriteTCP(int socket, String &buffer){
    return socketWriteTCP(socket, (char*)buffer.c_str(),buffer.length());
}

int ModemClient::socketReadTCP(int socket, char* return_buffer, size_t size){
    //make it very big to be reused for the read, double the si
    char command[50];
    _buffer = "";
    //limit the max number of bytes to the max read size
    snprintf(command,SOCKET_READ_MAX_SIZE, "AT+USORD=%d,%d",socket, size > SOCKET_READ_MAX_SIZE ? SOCKET_READ_MAX_SIZE : size);


    int val = commandSmartSend(command,_buffer,10,COMMAND_TIMEOUT ,true);
    if(val == -1){
        return -1;
    }
    
    int URC_start = _buffer.lastIndexOf("+USORD:");
    String reply_section = _buffer.substring(URC_start);
    int bytes = 0;
    //get the number of bytes that were sent
    int sscanf_result = sscanf(reply_section.c_str(),"+USORD: %*d, %d",&bytes);

    //if sscanf finds less than 2 value
    if(sscanf_result < 1){
        //expected reply value doesnt exists
        last_error = CE_REPLY_VALUE_INVALID;
        return -1;
    }
    //find where the bytes start
    int bytes_start = reply_section.indexOf("\"")+1;
    if(bytes_start != 0){
        const char* c_str_pointer = reply_section.c_str();
        //copy the bytes from the reply section string to the return buffer up to the number of bytes sent
        for(int byte_ind = 0; byte_ind < bytes; byte_ind++){
            // return_buffer[byte_ind] = c_str_pointer[bytes_start+byte_ind];
            byte n1 = c_str_pointer[byte_ind * 2+bytes_start];
            byte n2 = c_str_pointer[byte_ind * 2 + 1+bytes_start];

            if (n1 > '9') {
                n1 = (n1 - 'A') + 10;
            } else {
                n1 = (n1 - '0');
            }

            if (n2 > '9') {
                n2 = (n2 - 'A') + 10;
            } else {
                n2 = (n2 - '0');
            }
            return_buffer[byte_ind] = (n1 << 4) | n2;
        }
    }
    // Serial.print("SCAN ");
    // Serial.println(sscanf_result);


    return bytes;
}
int ModemClient::setHexMode(bool hex){
    char command[50];
    _buffer = "";
    snprintf(command,50, "AT+UDCONF=1,%d",hex);

    int val = commandSmartSend(command,_buffer,10,COMMAND_TIMEOUT ,true);
    if(val == -1){
        return -1;
    }
    return 1;
}
int ModemClient::socketConfigureSecurity(int socket, bool enabled, int profile){
    char command[50];
    _buffer = "";
    snprintf(command,50, "AT+USOSEC=%d, %d, %d",socket, enabled, profile);

    int val = commandSmartSend(command,_buffer,10,COMMAND_TIMEOUT ,true, 1000);
    if(val == -1){
        return -1;
    }
    return 1;
}
int ModemClient::socketSetSecurityProfile(int profile, int op_code){
    return -1;
}
// int setOperator(int mode, int format, char* operator){

// }
int ModemClient::getOperator(char* buffer){
    // char command[50];
    _buffer = "";
    // snprintf(command,50, "AT+COPS=%d,%d",socket, query_type);

    int val = commandSmartSend("AT+COPS?",_buffer,10,COMMAND_TIMEOUT ,true);
    if(val == -1){
        return -1;
    }
    int URC_start = _buffer.lastIndexOf("+COPS:");
    String reply_section = _buffer.substring(URC_start);

    int sscanf_result = sscanf(reply_section.c_str(),"+COPS: %*d, %*d, %s",buffer);
    // Serial.print("SCAN ");
    // Serial.println(sscanf_result);

    //if sscanf finds less than 1 value
    if(sscanf_result < 1){
        //expected reply value doesnt exists
        last_error = CE_REPLY_VALUE_INVALID;
        return -1;
    }
    return sscanf_result;
}
int ModemClient::attachDetatchGPRS(bool attach){
    char command[50];
    _buffer = "";
    snprintf(command,50, "AT+CGATT=%d",attach);

    int val = commandSmartSend(command,_buffer,10,100, true,100);
    if(val == -1){
        return -1;
    }
    return 1;
}
int ModemClient::getGPRSRegistrationStatus(){
// char command[50];
    _buffer = "";
    // snprintf(command,50, "AT+COPS=%d,%d",socket, query_type);

    // int val = commandSmartSend("AT+CGATT?",_buffer,10,10000 ,true);
    int val = commandSmartSend("AT+CGDCONT?",_buffer,10,10000 ,true);

    if(val == -1){
        return -1;
    }
    int URC_start = _buffer.lastIndexOf("+CGATT:");
    String reply_section = _buffer.substring(URC_start);
    int ret_val = 0;
    int sscanf_result = sscanf(reply_section.c_str(),"+CGATT: %d",&ret_val);
    // Serial.print("SCAN ");
    // Serial.println(_buffer);
    // Serial.println(val);

    //if sscanf finds less than 1 value
    if(sscanf_result < 1){
        //expected reply value doesnt exists
        last_error = CE_REPLY_VALUE_INVALID;
        return -1;
    }
    return ret_val;
}
int ModemClient::getNetworkRegistrationStatus(){
    _buffer = "";
    // snprintf(command,50, "AT+COPS=%d,%d",socket, query_type);

    int val = commandSmartSend("AT+CEREG?",_buffer,10,COMMAND_TIMEOUT ,true);
    if(val == -1){
        return -1;
    }
    int URC_start = _buffer.lastIndexOf("+CEREG:");
    String reply_section = _buffer.substring(URC_start);
    int ret_val = -1;
    int sscanf_result = sscanf(reply_section.c_str(),"+CEREG: %*d, %d",&ret_val);
    // Serial.print("SCAN ");
    // Serial.println(sscanf_result);

    //if sscanf finds less than 1 value
    if(sscanf_result < 1){
        //expected reply value doesnt exists
        last_error = CE_REPLY_VALUE_INVALID;
        return -1;
    }
    return ret_val;
}
int ModemClient::moduleOff(){
    _buffer = "";
    // snprintf(command,50, "AT+COPS=%d,%d",socket, query_type);

    int val = commandSmartSend("AT+CPWROFF",_buffer,10,40000 ,true);
    if(val == -1){
        return -1;
    }
    return val;
}
int ModemClient::shutdown(){
    int val = moduleOff();
    if(val != -1){
        modem->off();
    }
    return val;
}
int ModemClient::getTime(time_t &time,bool local){
    _buffer = "";
    // snprintf(command,50, "AT+COPS=%d,%d",socket, query_type);

    int val = commandSmartSend("AT+CCLK?",_buffer,10,1001 ,true);
    if(val == -1){
        return -1;
    }

    struct tm now;
    // struct tm tmp;
    // int dashIndex = _buffer.lastIndexOf('-');
    // if (dashIndex != -1) {
    // response.remove(dashIndex);
    // }
    // Serial.println(response);
    int URC_start = _buffer.lastIndexOf("+CCLK:");
    String reply_section = _buffer.substring(URC_start);
    int ret_val = 0;
    Serial.println(_buffer);
    if (strptime(reply_section.c_str(), "+CCLK: \"%y/%m/%d,%H:%M:%S", &now) != NULL) {
        // adjust for timezone offset which is +/- in 15 minute increments

        time = mktime(&now);
        // tmp = *gmtime(&time);
//        Serial.println((long)mktime(&tmp));
//        Serial.println(tmp.tm_year);
        time_t delta = ((reply_section.charAt(26) - '0') * 10 + (reply_section.charAt(27) - '0')) * (15 * 60);
//        Serial.println(time);
//        Serial.println(delta);
        if(local){
            if (reply_section.charAt(25) == '-') {
                time -= delta;
            } else if (reply_section.charAt(25) == '+') {
                time += delta;
            }
        }
        return 1;
    }
    //expected reply value doesnt exists
    last_error = CE_REPLY_VALUE_INVALID;
    return -1;
}

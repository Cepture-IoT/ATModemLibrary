#include "ModemClient.h"
#define __XSI_VISIBLE 1
#include <time.h>
ModemClient::ModemClient(SARAModem &modem){
    this->modem = &modem;
}
bool ModemClient::setSocket(int socket){
    char param_1[10];
    int param_2;
    _buffer[0] = '\0';
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
    int val = socketReadTCP(_current_socket, _buffer, 0);
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
    while(read_num > 0){
        read_num = socketReadTCP(_current_socket,_buffer,MCLIENT_BUFFER_SIZE);
    }

}
int ModemClient::connect(IPAddress ip, uint16_t port){
    //char host[17] = "255.255.255.255\0";
    snprintf(_buffer,17,"%d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);
    return connect((const char*)_buffer,port);
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
    //char tmp[10];
    int tmp2;
    int val = socketQuery(_current_socket,SQUERY_TCP_STATUS, _buffer, tmp2);
    //reuse tmp2 
    int status;
    tmp2 = sscanf(_buffer,"%d",&status);
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

int ModemClient::commandSmartSend(char *command, char* buffer,int attempts, int timeout, bool wait, unsigned long lag_timeout){
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
int ModemClient::commandSmartRead(char* buffer,int attempts, int timeout, bool wait){
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
    _buffer[0] = '\0';
    int protocol = use_tcp ? 6 : 17;
    snprintf(command,50, "AT+USOCR=%d,%d",protocol, port);

    int val = commandSmartSend(command,_buffer,10,COMMAND_TIMEOUT ,true);
    if(val == -1){
        return -1;
    }

    int URC_start = lastStrStr(_buffer,"+USOCR:");
    //check if expected reply exists
    if(URC_start == -1){
        //reply not found
        last_error = CE_REPLY_NOT_FOUND;
        return -1;
    }
    char* reply_p = _buffer + URC_start;
    int socket = -1;
    int sscanf_result = sscanf(reply_p,"+USOCR: %d",&socket);
    //if sscanf finds nothing or socket is out of range
    if(sscanf_result != 1 || socket < SOCKET_MIN || socket > SOCKET_MAX){
        //expected reply value doesnt exists
        last_error = CE_REPLY_VALUE_INVALID;
        return -1;
    }
    return socket;
}
int ModemClient::socketQuery(int socket, int query_type,char* param_1, int &param_2){
    _buffer[0] = '\0';
    snprintf(command,50, "AT+USOCTL=%d,%d",socket, query_type);

    int val = commandSmartSend(command,_buffer,10,COMMAND_TIMEOUT ,true);
    if(val == -1){
        return -1;
    }
    int URC_start = lastStrStr(_buffer,"+USOCTL:");
    //check if expected reply exists
    if(URC_start == -1){
        //reply not found
        last_error = CE_REPLY_NOT_FOUND;
        return -1;
    }
    char* reply_p = _buffer + URC_start;

    int sscanf_result = sscanf(reply_p,"+USOCTL: %*d, %*d, %s, %d",param_1,&param_2);
    //if sscanf finds nothing
    if(sscanf_result < 1){
        //expected reply value doesnt exists
        last_error = CE_REPLY_VALUE_INVALID;
        return -1;
    }
}

int ModemClient::socketClose(int socket, bool async){
    _buffer[0] = '\0';
    snprintf(command,50, "AT+USOCL=%d,%d",socket,async);

    int val = commandSmartSend(command,_buffer,10,COMMAND_TIMEOUT ,true, 10000);
    if(val == -1){
        return -1;
    }
    return 1;
}
int ModemClient::socketConnect(int socket, const char* address, int port){
    _buffer[0] = '\0';
    snprintf(command,50, "AT+USOCO=%d,\"%s\",%d",socket, address, port);

    int val = commandSmartSend(command,_buffer,10,10000 ,true,10000);
    if(val == -1){
        return -1;
    }
    return 1;
}

int ModemClient::socketWriteTCP(int socket, const char* buffer, size_t buffer_length){
    // String command;
    // command.reserve(19+(buffer_length < SOCKET_WRITE_MAX_SIZE ? buffer_length : SOCKET_WRITE_MAX_SIZE)*2);
    //used to store the two HEX characters to write using strncat
    char dual_byte[3] = "00";
    int written = 0;
    //split buffer into chunks, mostly stolen from NBMKR
    while(buffer_length){
        size_t chunk_size = buffer_length;

        if (chunk_size > SOCKET_WRITE_MAX_SIZE) {
            chunk_size = SOCKET_WRITE_MAX_SIZE;
        }

        snprintf(command,MCLIENT_BUFFER_SIZE, "AT+USOWR=%d,%d,\"",socket, (uint16_t)chunk_size);
        // strncpy(command,"AT+USOWR=",MCLIENT_BUFFER_SIZE);
        // command += socket;
        // command += ",";
        // command += (uint16_t)chunk_size;
        // command += ",\"";
        size_t command_len = strlen(command);
        for (size_t i = 0; i < chunk_size; i++) {
            byte b = buffer[i + written];

            byte n1 = (b >> 4) & 0x0f;
            byte n2 = (b & 0x0f);

            dual_byte[0] = (char)(n1 > 9 ? 'A' + n1 - 10 : '0' + n1);
            dual_byte[1] = (char)(n2 > 9 ? 'A' + n2 - 10 : '0' + n2);
            strncat(command,dual_byte,MCLIENT_BUFFER_SIZE-command_len-i);
        }
        command_len = strlen(command);
        strncat(command,"\"",MCLIENT_BUFFER_SIZE-command_len);

        int val = commandSmartSend(command,_buffer,10,COMMAND_TIMEOUT,true);
        if(val == -1){
            return -written;
        }
        written += chunk_size;
        buffer_length -= chunk_size;
    }
    return written;
}
int ModemClient::socketWriteTCP(int socket, char* buffer){
    return socketWriteTCP(socket, buffer, strlen(buffer));
}

int ModemClient::socketReadTCP(int socket, char* return_buffer, size_t size){
    _buffer[0] = '\0';
    //limit the max number of bytes to the max read size
    snprintf(command,MCLIENT_BUFFER_SIZE, "AT+USORD=%d,%d",socket, size > SOCKET_READ_MAX_SIZE ? SOCKET_READ_MAX_SIZE : size);


    int val = commandSmartSend(command,_buffer,10,COMMAND_TIMEOUT ,true);
    if(val == -1){
        return -1;
    }
    
    int URC_start = lastStrStr(_buffer,"+USORD:");
    //check if expected reply exists
    if(URC_start == -1){
        //reply not found
        last_error = CE_REPLY_NOT_FOUND;
        return -1;
    }
    char* reply_p = _buffer + URC_start;
    int bytes;
    int sscanf_result = sscanf(reply_p,"+USORD: %*d, %d",&bytes);
    
    //int sscanf_result = sscanf(reply_p,"+USORD: %d",&bytes);

    //if sscanf finds nothing
    if(sscanf_result < 1){
        //expected reply value doesnt exists
        last_error = CE_REPLY_VALUE_INVALID;
        return -1;
    }
    char* bytes_start = strstr(reply_p,"\"")+1;
    // if(bytes > 0){
    //     Serial.println(_buffer);
    //     //find where the bytes start
        
    //     Serial.println("---------------");
    //     Serial.println(reply_p);
    //     Serial.println("---------------");
    //     Serial.println(bytes_start);
    // }
    //copy the bytes from the reply section string to the return buffer up to the number of bytes sent
    //TODO: Make it that the thing wont read past the buffer size even though i dont think this will occur
    for(int byte_ind = 0; byte_ind < bytes; byte_ind++){
        // return_buffer[byte_ind] = c_str_pointer[bytes_start+byte_ind];
        byte n1 = bytes_start[byte_ind * 2];
        byte n2 = bytes_start[byte_ind * 2 + 1];

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
    // return_buffer[bytes] = '\0';
    // Serial.print("RP: ");
    // Serial.println(size);
    // for(int i = 0; i < bytes;i++){
    //     Serial.println((int)(return_buffer[i]));
    // }
    // Serial.println("END");

    return bytes;
}
int ModemClient::setHexMode(bool hex){
    _buffer[0] = '\0';
    snprintf(command,50, "AT+UDCONF=1,%d",hex);

    int val = commandSmartSend(command,_buffer,10,COMMAND_TIMEOUT ,true);
    if(val == -1){
        return -1;
    }
    return 1;
}
int ModemClient::socketConfigureSecurity(int socket, bool enabled, int profile){
    _buffer[0] = '\0';
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
    _buffer[0] = '\0';

    int val = commandSmartSend("AT+COPS?",_buffer,10,COMMAND_TIMEOUT ,true);
    if(val == -1){
        return -1;
    }
    int URC_start = lastStrStr(_buffer,"+COPS:");
    //check if expected reply exists
    if(URC_start == -1){
        //reply not found
        last_error = CE_REPLY_NOT_FOUND;
        return -1;
    }
    char* reply_p = _buffer + URC_start;
    int socket = -1;
    int sscanf_result = sscanf(reply_p,"+COPS: %*d, %*d, %s",buffer);

    //if sscanf finds less than 1 value
    if(sscanf_result < 1){
        //expected reply value doesnt exists
        last_error = CE_REPLY_VALUE_INVALID;
        return -1;
    }
    return sscanf_result;
}
int ModemClient::attachDetatchGPRS(bool attach){
    _buffer[0] = '\0';
    snprintf(command,50, "AT+CGATT=%d",attach);

    int val = commandSmartSend(command,_buffer,10,100, true,100);
    if(val == -1){
        return -1;
    }
    return 1;
}
int ModemClient::getGPRSRegistrationStatus(){
    _buffer[0] = '\0';
    int val = commandSmartSend("AT+CGDCONT?",_buffer,10,10000 ,true);

    if(val == -1){
        return -1;
    }
    int URC_start = lastStrStr(_buffer,"+CGATT:");
    //check if expected reply exists
    if(URC_start == -1){
        //reply not found
        last_error = CE_REPLY_NOT_FOUND;
        return -1;
    }
    char* reply_p = _buffer + URC_start;
    int ret_val = -1;
    int sscanf_result = sscanf(reply_p,"+CGATT: %d",&ret_val);
    //if sscanf finds less than 1 value
    if(sscanf_result < 1){
        //expected reply value doesnt exists
        last_error = CE_REPLY_VALUE_INVALID;
        return -1;
    }
    return ret_val;
}
int ModemClient::getNetworkRegistrationStatus(){
    _buffer[0] = '\0';

    int val = commandSmartSend("AT+CEREG?",_buffer,10,COMMAND_TIMEOUT ,true);
    if(val == -1){
        return -1;
    }
    int URC_start = lastStrStr(_buffer,"+CEREG:");
    //check if expected reply exists
    if(URC_start == -1){
        //reply not found
        last_error = CE_REPLY_NOT_FOUND;
        return -1;
    }
    char* reply_p = _buffer + URC_start;
    int ret_val = -1;
    int sscanf_result = sscanf(reply_p,"+CEREG: %*d, %d",&ret_val);
    //if sscanf finds less than 1 value
    if(sscanf_result < 1){
        //expected reply value doesnt exists
        last_error = CE_REPLY_VALUE_INVALID;
        return -1;
    }
    return ret_val;
}
int ModemClient::moduleOff(){
    _buffer[0] = '\0';
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
    _buffer[0] = '\0';
    int val = commandSmartSend("AT+CCLK?",_buffer,10,1001 ,true);
    if(val == -1){
        return -1;
    }
    int URC_start = lastStrStr(_buffer,"+CCLK:");
    //check if expected reply exists
    if(URC_start == -1){
        //reply not found
        last_error = CE_REPLY_NOT_FOUND;
        return -1;
    }
    char* reply_p = _buffer + URC_start;

    tm now;
    if (strptime(reply_p , "+CCLK: \"%y/%m/%d,%H:%M:%S", &now) != NULL) {
        // adjust for timezone offset which is +/- in 15 minute increments

        time = mktime(&now);
        time_t delta = ((reply_p[26]  - '0') * 10 + (reply_p[27] - '0') )* (15 * 60);

        if(local){
            if (reply_p[25] == '-') {
                time -= delta;
            } else if (reply_p[25] == '+') {
                time += delta;
            }
        }
        return 1;
    }
}

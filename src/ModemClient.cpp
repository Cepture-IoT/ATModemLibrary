#include "ModemClient.h"

ModemClient::ModemClient(SARAModem &modem, int buffer_size){
    this->modem = &modem;
    _buffer.reserve(buffer_size);
}

size_t ModemClient::write(uint8_t){

}
size_t ModemClient::write(const uint8_t* buf, size_t size){

}

int ModemClient::available(){

}
int ModemClient::read(){

}
int ModemClient::read(uint8_t *buf, size_t size){

}
int ModemClient::peek(){

}
void ModemClient::flush(){

}
int ModemClient::connect(IPAddress ip, uint16_t port){

}
int ModemClient::connect(const char *host, uint16_t port){

}
void ModemClient::stop(){

}
uint8_t ModemClient::connected(){

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
    Serial.println("HERE");
    Serial.println(_buffer);
    Serial.println(URC_start);
    Serial.println("HERE2");
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
     Serial.print("SCAN ");
     Serial.println(sscanf_result);

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
    snprintf(command,50, "AT+USOCL=%d",socket);

    int val = commandSmartSend(command,_buffer,10,COMMAND_TIMEOUT ,true, 1000);
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
int ModemClient::socketConnect(int socket, char* address, int port){
    char command[50];
    _buffer = "";
    snprintf(command,50, "AT+USOCO=%d,\"%s\",%d",socket, address, port);

    int val = commandSmartSend(command,_buffer,10,10000 ,true,10000);
    if(val == -1){
        return -1;
    }
    return 1;
}

int ModemClient::socketWriteTCP(int socket, char* buffer){
    size_t buffer_length = strlen(buffer);
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
    return socketWriteTCP(socket, (char*)buffer.c_str());
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

#ifndef _MODEMCLIENT_H_INCLUDED
#define _MODEMCLIENT_H_INCLUDED



// AT+COPS
// AT+URAT
// AT+CFUN
// AT+CGATT
// AT+CGACT
// AT+CGPADDR
//// AT+IPR
// AT+CEREG
// AT+CPWROFF
// AT+CMEE
// AT+CPIN
// AT+CMGF
// AT+UDCONF
// AT+CTZU
// AT+CGDCONT
// AT+UAUTHREQ
// AT+CCLK
// AT+USOCR
// AT+USOSEC
// AT+USECPRF
// AT+USOCO
// AT+USOCL
// AT+USOWR
// AT+CGSN
// AT+CCID
// AT+CPWD
// AT+CREG
#include "SARAModem.h"
#include <Client.h>
#define SOCKET_MIN 0
#define SOCKET_MAX 6
#define SOCKET_WRITE_MAX_SIZE 512
#define SOCKET_WRITE_MAX_SIZE 512
#define COMMAND_TIMEOUT 200
#define PROMP_TIMEOUT 100
enum CommandErrorEnum{
    CE_TIMEOUT,
    CE_ERROR,
    CE_NO_CARRIER,
    CE_CME_ERROR,
    CE_REPLY_NOT_FOUND,
    CE_REPLY_VALUE_INVALID,
    CE_BYTE_PROMPT_NOT_FOUND
};
enum SocketQueryTypeEnum{
    SQUERY_TYPE = 0,
    SQUERY_ERROR = 1,
    SQUERY_BYTES_SENT = 2,
    SQUERY_BYTES_RECEIVED = 3,
    SQUERY_REMOTE = 5,
    SQUERY_TCP_STATUS = 10,
    SQUERY_TCP_UNACKNOWLEDGED = 11
};
class ModemClient : public Client {
    public:
        ModemClient(SARAModem &modem, int buffer_size);

        size_t write(uint8_t);
        size_t write(const uint8_t* buf, size_t size);

        int available();
        int read();
        int read(uint8_t *buf, size_t size);
        int peek();
        void flush();
        int connect(IPAddress ip, uint16_t port);
        int connect(const char *host, uint16_t port);
        void stop();
        uint8_t connected();
        operator bool();

//AT COMMANDS
        int setOperator();
        int setRadioAccessTechnology();
        int setModuleFunctionality();
        int attachDetatchGPRS();
        int activateDeactivatePDPContext();
        int setPDPContext();
        int getPDPAddress();
        int getGPRSRegistrationStatus();
        int getNetworkRegistrationStatus();
        int moduleOff();
        int configureCMEErrorFormat();
        int enterPin();
        int setMessageFormat();
        int tuneNBIoTBandScanning();
        int configureAutoTimezoneUpdate();
        int setAuthenticationParameters();
        int getCardIdentifier();
        int getTime();
        //sockets
        int socketQuery(int socket, int query_type,char* param_1, int &param_2);
        int socketCreate(bool tcp, int port);
        int socketConfigureSecurity();
        int socketSetSecurityProfile();
        int socketConnect(int socket, char* address, int port);
        int socketClose(int socket, bool async);
        int socketWriteTCP(int socket, char* buffer);
        int socketWriteTCP(int socket, String &buffer);
        int socketReadTCP(int socket, char* return_buffer, size_t size);

        bool waitForBytePrompt(unsigned long timeout);
        CommandErrorEnum last_error = CE_TIMEOUT;
    private:
        int commandSmartSend(char *command, String &buffer, int attempts, int timeout, bool wait);
        SARAModem* modem;
        String _buffer;
        
};
#endif
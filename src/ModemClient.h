
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
#define SOCKET_READ_MAX_SIZE 512
#define COMMAND_TIMEOUT 200
#define PROMP_TIMEOUT 2000
#define MCLIENT_READ_BUFFER_SIZE 1024+64
#define MCLIENT_WRITE_BUFFER_SIZE 1024+64
enum CommandErrorEnum{
    CE_TIMEOUT,
    CE_ERROR,
    CE_NO_CARRIER,
    CE_CME_ERROR,
    CE_REPLY_NOT_FOUND,
    CE_REPLY_VALUE_INVALID,
    CE_BYTE_PROMPT_TIMEOUT,
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
enum SocketQueryTCPStatusEnum{
    TCPSTATUS_LISTEN = 1,
    TCPSTATUS_SYN_SENT = 2,
    TCPSTATUS_SYN_RECV = 3,
    TCPSTATUS_ESTABLISHED = 4,
    TCPSTATUS_FIN_WAIT_1 = 5,
    TCPSTATUS_FIN_WAIT_2 = 6,
    TCPSTATUS_CLOSE_WAIT = 7,
    TCPSTATUS_CLOSING = 8,
    TCPSTATUS_LAST_ACK = 9,
    TCPSTATUS_TIME_WAIT = 10
};
enum SocketSecurityOpCodesEnum{
    OPC_CERTIFICATE_LEVEL = 0,
    OPC_SSL_TLS_DTLS_VERSION = 1,
    OPC_CIPHER_SUITE = 2,
    OPC_TRUST_ROOT_CERT_NAME = 3,
    OPC_EXPECTED_SERVER_HOST_NAME = 4,
    OPC_CLIENT_CERT_INT_NAME = 5,
    OPC_CLIENT_PRIV_KEY_INT_NAME = 6,
    OPC_CLIENT_PRIV_KEY_PASSWRD = 7,
    OPC_PRE_SHARED_KEY = 8,
    OPC_PRE_SHARED_KEY_IDENTITY = 9,
    OPC_SERVER_NAME_ID = 10,
    OPC_PSK_KEY_ID = 11,
    OPC_SERVER_CERT_PIN = 12,
    OPC_TLS_SESS_RESUPTION = 13
};
enum NetworkStatusEnum{
    NETSTAT_NOT_REGISTERED = 0,
    NETSTAT_REGISTERED_HOME = 1,
    NETSTAT_SEARCHING = 2,
    NETSTAT_DENIED = 3,
    NETSTAT_UNKNOWN = 4,
    NETSTAT_REGISTERED_ROAMING = 5,
    NETSTAT_EMERGENCY_ONLY = 8
};

class ModemClient : public Client {
    public:
        ModemClient(SARAModem &modem);

        bool setSocket(int socket);
        int shutdown();
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
        //int setOperator();
        int getOperator(char* buffer);
        //int setRadioAccessTechnology();
        //int setModuleFunctionality();
        int attachDetatchGPRS(bool attach);
        //int activateDeactivatePDPContext();
        //int setPDPContext();
        //int getPDPAddress();
        int getGPRSRegistrationStatus();
        int getNetworkRegistrationStatus();
        int moduleOff();
        //int configureCMEErrorFormat();
        //int enterPin();
        //int setMessageFormat();
        //int tuneNBIoTBandScanning();
        //int configureAutoTimezoneUpdate();
        //int setAuthenticationParameters();
        //int getCardIdentifier();
        int getTime(time_t &time,bool local);
        //sockets
        int socketQuery(int socket, int query_type,char* param_1, int &param_2);
        int socketCreate(bool tcp, int port);
        int socketConfigureSecurity(int socket, bool enabled, int profile);
        int socketSetSecurityProfile(int profile, int op_code);
        int socketConnect(int socket, const char* address, int port);
        int socketClose(int socket, bool async);
        int socketWriteTCP(int socket, const char* buffer, size_t length);
        int socketWriteTCP(int socket, char* buffer);
        int socketReadTCP(int socket, char* return_buffer, size_t size);
        int setHexMode(bool hex);
        bool waitForBytePrompt(unsigned long timeout);
        CommandErrorEnum last_error = CE_TIMEOUT;

        int commandSmartSend(char *command, char* buffer, size_t size, int attempts, int timeout, bool wait);
        int commandSmartSend(char *command, char* buffer, size_t size,int attempts, int timeout, bool wait);
    private:
        int commandSmartSend(char *command, char* buffer, int attempts, int timeout, bool wait);
        int commandSmartRead(char* buffer,int attempts, int timeout, bool wait);
        int commandSmartSend(char *command, char* buffer, size_t size,int attempts, int timeout, bool wait);
        int commandSmartRead(char* buffer,size_t size,int attempts, int timeout, bool wait);
        SARAModem* modem;
        char _buffer[MCLIENT_READ_BUFFER_SIZE];
        char command[MCLIENT_WRITE_BUFFER_SIZE];
        int _current_socket;
        int peek_char = -1;
        
};
#endif
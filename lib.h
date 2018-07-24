
#ifndef LIB
#define LIB
#include <stddef.h>
#include <stdbool.h>

#define MAXL 250
#define TIME 5

#define HEADER_SIZE 4
#define TAIL_SIZE 3
#define NO_DATA_SIZE 7 //size of Pkt_header + Pkt_tail

#define TIMEOUT 5

#define SEND_INIT 'S'
#define FILE_HEADER 'F'
#define DATA 'D'
#define FEOF 'Z'
#define EOT 'B'
#define ACK 'Y'
#define NAK 'N'
#define ERROR 'E'

typedef struct {
  int len;
  char payload[1400];
} msg;

//Mini-Kermit pkt which will be included in Msg

typedef struct {
  //Pkt_header
  char soh;
  unsigned char len;
  char seq, type;
  //Pkt_data
  char *data;
  //Pkt_tail
  unsigned short check;
  char mark;
} pkt;

//Data of Send-Init(S) pkt

typedef struct {
  char maxl, time, npad, padc, eol, qctl, qbin, chkt, rept, capa, r;
} initial;

void init(char* remote, int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);
msg* receive_message_timeout(int timeout); //timeout in milliseconds
unsigned short crc16_ccitt(const void *buf, int len);

void composePktHeader(pkt *packet, int seq, char type, unsigned char dataSize) {
  packet->soh = 0x01;
  packet->len = NO_DATA_SIZE + dataSize - 2;
  packet->seq = seq;
  packet->type = type;
}

void composePktData(pkt *packet, bool hasData, unsigned char dataSize, char* data) {
  if (!hasData) {
    packet->data = NULL;
  } else {
    packet->data = (char *) malloc(dataSize);
    memcpy(packet->data, data, dataSize);
  }
}

void composePktTail(msg *message, pkt *packet) {
  packet->check = crc16_ccitt(message->payload, message->len);
  packet->mark = 0x0D;
}

void composeMsg(msg *message, pkt *packet, int seq, char type, bool hasData,
  unsigned char dataSize, char *data) {

    composePktHeader(packet, seq, type, dataSize);
    //add Pkt_header to message
    memcpy(&(message->payload[0]), &(packet->soh), 1);
    memcpy(&(message->payload[1]), &(packet->len), 1);
    memcpy(&(message->payload[2]), &(packet->seq), 1);
    memcpy(&(message->payload[3]), &(packet->type), 1);
    message->len = HEADER_SIZE;

    composePktData(packet, hasData, dataSize, data);
    //add Pkt_data to message
    memcpy(message->payload + message->len, packet->data, dataSize);
    message->len = message->len + dataSize;

    composePktTail(message, packet);
    //add Pkt_tail to message
    memcpy(&(message->payload[HEADER_SIZE + dataSize]), &(packet->check), 2);
    memcpy(&(message->payload[HEADER_SIZE + dataSize + 2]), &(packet->mark), 1);
    message->len = message->len + TAIL_SIZE;

  }

  unsigned char getMsgLen(msg *message) {
    return message->payload[1];
  }

  char getSeq(msg *message) {
    return message->payload[2];
  }

  char getType(msg *message) {
    return message->payload[3];
  }

  unsigned char getData(char* data, msg* message) {
    unsigned char dataSize = message->len - TAIL_SIZE - HEADER_SIZE;
    memcpy(data, message->payload + HEADER_SIZE, dataSize);
    return dataSize;
  }

  unsigned short getCRC(msg *message) {
    unsigned short t = 0;
    memcpy(&t, message->payload + message->len - TAIL_SIZE, TAIL_SIZE - 1);
    return t;
  }

  bool checkCRC(msg* message) {
    unsigned short currentCRC = getCRC(message);
    unsigned short crc = crc16_ccitt(message->payload, message->len - TAIL_SIZE);
    if (currentCRC == crc) return true;
    return false;
  }

  #endif

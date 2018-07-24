#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

msg lastMsg;
msg* tryReceiveMsg(int timeout) {
  bool hasData = false;
  msg* r; //receive message
  msg t; //ACK/NAK
  pkt p;
  char rSeq;
  int attemptsLeft = 3;

  while(1) {
    r = receive_message_timeout(timeout * 1000);
    if (r != NULL) { //received
      rSeq = getSeq(r);
      if (checkCRC(r)) { //Send ACK
        memset(&t, 0, sizeof(msg));
        memset(&p, 0, sizeof(pkt));
        composeMsg(&t, &p, rSeq, ACK, hasData, 0, NULL);
        send_message(&t);
        lastMsg = t;
        break;
      }  else { //Send NAK
        memset(&p, 0, sizeof(pkt));
        memset(&t, 0, sizeof(msg));
        composeMsg(&t, &p, rSeq, NAK, hasData, 0, NULL);
        send_message(&t);
        lastMsg = t;
      }
    } else { //haven't received
      if(lastMsg.len != 0 && getSeq(&lastMsg) != rSeq) {
        attemptsLeft --;
        if(attemptsLeft == 0) exit(-1); //timeout
        //retry send last ACK/NAK
        send_message(&lastMsg);
      }
    }

  }
  return r;
}

int main(int argc, char** argv) {

  init(HOST, PORT);
  msg *r;
  bool hasData;

  //Receive Initial Packet
  msg* i = tryReceiveMsg(5);
  if (i == NULL) {
    printf("Receiver: Initial Packet Error\n");
    fflush(stdout);
    exit(-1);
  }

  //Get Initial Paket Data
  char buffer[MAXL];
  unsigned char dataSize;
  dataSize = getData(buffer, i);
  int timeout = buffer[1];
  unsigned char maxLen = buffer[0];

  //Receive files until EOT

  r = tryReceiveMsg(timeout);
  char type = getType(r);

  while(type != EOT) {
  //file header
   char fileName[] = "recv_";
   char buffer[maxLen];
   unsigned char dataSize = getData(buffer, r);
   strcat(fileName, buffer);

   int file = open(fileName, O_WRONLY|O_CREAT|O_TRUNC, 0644);
   printf("(Receiver) Opening filename=%s\n", fileName);

   msg* q;
   q = tryReceiveMsg(timeout);
   char qtype = getType(q);

   //Write until FEOF
   while(qtype != FEOF) {
     char buff[maxLen];
     unsigned char dataLen = getData(buff, q);
     write(file, buff, dataLen);
     q = tryReceiveMsg(timeout);
     qtype = getType(q);
   }
   r = tryReceiveMsg(timeout);
   type = getType(r);
 }

  return 0;
}

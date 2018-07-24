#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

void trySendMsg(msg* message) {
  //y - received message
  send_message(message);
  int times = 0;
  while(1) {
    msg *y = receive_message_timeout(TIMEOUT * 1000);
    if (y != NULL) { //receive message
      if (getSeq(message) == getSeq(y)){
        if(getType(y) == ACK) {
          break;
        } else { //seqs don't match
          send_message(message);
        }
      }
    } else { //didn't receve message
        if(times == 3) { //timeout
          exit(1);
        }
        times++;
        send_message(message);
      }
  }
}

int main(int argc, char** argv) {

  init(HOST, PORT);

  int seq = 0;
  msg t;
  pkt p;
  initial sPkt;

  //initialize Send-Init(S) pkt
  sPkt = (initial) {
    .maxl = MAXL,
    .time = TIME,
    .npad = 0x00,
    .padc = 0x00,
    .eol  = 0x0D,
    .qctl = 0x00,
    .qbin = 0x00,
    .chkt = 0x00,
    .rept = 0x00,
    .capa = 0x00,
    .r = 0x00,
  };
  //Send initial packet
  int initDataLen = sizeof(initial);
  char initData[initDataLen];
  memcpy(initData, &sPkt, initDataLen);
  bool hasData = true;
  composeMsg(&t, &p, seq, SEND_INIT, hasData, initDataLen, initData);
  trySendMsg(&t);
  seq++;
  memset(t.payload, 0, 1400);	//reset msg
  memset(&p, 0, sizeof(pkt));//reset pkt

  //Send files
  for (int i = 1; i < argc; i++) {

    printf("(Sender) Sending filename=%s\n", argv[i]);
    int filenameSize = strlen(argv[i]);
    int file = open(argv[i], O_RDONLY);
    //Send FILE-HEADER
    hasData = true;
    composeMsg(&t, &p, seq, FILE_HEADER, hasData, filenameSize + 1, argv[i]);
    trySendMsg(&t);
    seq++;
    memset(t.payload, 0, 1400);
    memset(&p, 0, sizeof(pkt));

    //Send DATA
    char *buffer = (char *)malloc(MAXL);
    int chunkSize;
    while ((chunkSize = read(file, buffer, MAXL)) > 0) {
      hasData = true;
      composeMsg(&t, &p, seq, DATA, true, chunkSize, buffer);
      trySendMsg(&t);
      seq++;
      memset(t.payload, 0, 1400);
      memset(&p, 0, sizeof(pkt));
    }

      free(buffer);
      close(file);

    //Send FEOF
    hasData = false;
    composeMsg(&t, &p, seq, FEOF, hasData, 0, NULL);
    trySendMsg(&t);
    seq++;
    memset(t.payload, 0, 1400);
    memset(&p, 0, sizeof(pkt));
  }

  //Send EOT
  composeMsg(&t, &p, seq, EOT, hasData, 0, NULL);
  trySendMsg(&t);
  seq++;

  return 0;
}

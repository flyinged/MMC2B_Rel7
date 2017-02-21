#ifndef __TFTPD_H_
#define __TFTPD_H_

#include "ff.h"
#include "lwip/sockets.h"
#include "lwip/ip.h"

#define TFTP_OPCODE_LEN 2
#define TFTP_BLKNUM_LEN 2
#define TFTP_ERRCODE_LEN 2
#define TFTP_DATA_LEN_MAX 512

#define TFTP_DATA_PKT_HDR_LEN (TFTP_OPCODE_LEN + TFTP_BLKNUM_LEN)
#define TFTP_ERR_PKT_HDR_LEN (TFTP_OPCODE_LEN + TFTP_ERRCODE_LEN)
#define TFTP_ACK_PKT_LEN (TFTP_OPCODE_LEN + TFTP_BLKNUM_LEN)

#define TFTP_DATA_PKT_LEN_MAX (TFTP_DATA_PKT_HDR_LEN + TFTP_DATA_LEN_MAX)

#define TFTP_MAX_RETRIES 3
#define TFTP_TIMEOUT_INTERVAL 5

// TFTP opcodes as specified in RFC1350
typedef enum {
  TFTP_RRQ = 1,
  TFTP_WRQ = 2,
  TFTP_DATA = 3,
  TFTP_ACK = 4,
  TFTP_ERROR = 5
} tftp_opcode;

// TFTP error codes as specified in RFC1350
typedef enum {
  TFTP_ERR_NOTDEFINED,
  TFTP_ERR_FILE_NOT_FOUND,
  TFTP_ERR_ACCESS_VIOLATION,
  TFTP_ERR_DISKFULL,
  TFTP_ERR_ILLEGALOP,
  TFTP_ERR_UKNOWN_TRANSFER_ID,
  TFTP_ERR_FILE_ALREADY_EXISTS,
  TFTP_ERR_NO_SUCH_USER,
  TFTP_ERR_WRONG_FILENAME, //ML84
} tftp_errorcode;

typedef struct {
  int op;    /* Read/Write */
  FIL fd;    /* File Handle for FAT FS */
  uint8_t fid; //ML84 added

  /* last block read */
  char data[TFTP_DATA_PKT_LEN_MAX];
  int  data_len;

  /* destination ip:port */
  struct ip_addr to_ip;
  int to_port;

  /* next block number */
  int block;

  // total number of bytes transferred
  int tot_bytes;

  /* timer interrupt count when last packet was sent */
  /* this should be used to re send packets on timeout */
  unsigned long long last_time;

} tftp_connection_args;

// Function prototypes
int tftpd_init();

#endif

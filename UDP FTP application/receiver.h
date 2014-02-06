/*receiver.h*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "common.h"


//#include "list.h"

#define RECEIVER_UDP_CONTROL_PORT    10001       // file name sent, ACKs received
#define RECEIVER_UDP_DATA_PORT   6001           // transfer 
#define SENDER_UDP_NAK_PORT   8001           	// sending NAKs 
#define MY_UDP_NAK_PORT   9001           	// sending NAKs 


//Receiver states
#define INITIAL_STATE 0
#define CONT_RECVD 1
#define CONT_ACK_SENT 2
#define EOF_RCVD 4
#define END_OF_TRANSFER 8

//RTT measurement
#define SEC_TO_USEC 1000000
#define RTT_MULTIPLIER 2.5

//Receiver window size
#define MAX_RECVR_WINDOW_SIZE 256
#define IDX_START

//NAKs
#define NBR_DUPLICATE_NAKS 2 
#define WRITE_BUF_SIZE 745653 

//Show Progrss Thread
#define PROGRESS_THREAD_SECS 15

/*//End of transfer ACK - copies
#define LASK_ACK_DUP_COUNT   10
#define FIRST_ACK_DUP_COUNT  10*/

//Different implementations
//Send NAK on every packet received out of order
//#define PERPACKET_NAK 1


#define ASSERT(x) if(!(x)) { fprintf( stderr,  "%s (%d): Assertion failed!!!\n", __FILE__, __LINE__); }
#define IS_SET(s, b) ((s & b) == b)



#define DEBUG_ERR(msg) {fprintf(stderr, "%s (%d): %s\n", __FILE__, __LINE__, msg);  exit(1);}
#ifdef DEBUG
	#define DEBUG_MSG(msg) {fprintf(stderr, "%s (%d): %s\n", __FILE__, __LINE__, msg);}
#else
	#define DEBUG_MSG(msg) {}//do nothing)
#endif 




typedef struct session
{
    	struct stat _stat;                  /*file stats*/
    	FILE * fp;             
    	int tcp_listen_sockfd;
	int tcp_sockfd;
    	int    udp_sockfd;        
	int    udp_sockfd_nak; 
                           
    	char  local_file_path[FILE_NAME_LENGTH]; /*populated from the prg input*/
    	char  remote_file_path[FILE_NAME_LENGTH]; 
    	size_t packet_size;                 /*size of each packet*/
  	int seq_no;                   
    	unsigned int conn_state; 
	char *dest_path;
	char  dest_dir[MAX_PATH_LENGTH];       // where the file needs to be placed at the receiver
    	char  file_name[FILE_NAME_LENGTH];      // name of the file being transferred
    	unsigned int file_size;                 // size
    	int num_packets;
	int expected_seqno;
	suseconds_t oneEndDelay;		// one side delay taken from TCP handshake
	suseconds_t timeoutVal;			// Timeout value for NAK sending thread

	int sendrAddressUpdated;
	struct sockaddr_in udpSendTo; 
	struct sockaddr_in myAddr;

    struct sockaddr_in udpCtrlSendTo; 
    struct sockaddr_in myAddr2;

	int max_seq_in_buffer;		//Required by NAKsender to know till what seq no
						//to look for gaps
	double endToEndDelay;			//In milliseconds

	int nbr_nak_duplicates;
	long nak_sender_delay;
	long inter_packet_delay;
    int numPacketBuffer;

} session_t;


session_t sd; /* session data */


/* Mutex for NAK thread synchronization */
pthread_mutex_t mtx_nak_sndr;
pthread_mutex_t mtx_buf_wrtr;
pthread_cond_t	cv_naksndr_start;
pthread_cond_t	cv_buf_wrtr;

typedef struct writebuf
{
	long long nbr_bytes;
	long long size;
	unsigned char *data;
} wbuf_t;

wbuf_t wbuf;

/*
1. check file permissions (validate) - use stat
2. 
*/
FILE * open_file (char *file_name);

/*we should be able to seek and read particular chunk of file, beginning at a byte offset*/
size_t read_data (FILE * fp, size_t bytes_to_read);

/*inputs: RTT
output: optimal packet size*/
size_t calc_packet_size(); // Venkat

/*procedure:
calculate bandwidth delay product*/
size_t calc_num_packets_to_send();

/*Uses TCP connection to receive fie path and other info*/
int recv_control();

/*Uses TCP connection to receive ACKs [both for control and data]*/
int recv_ack();

/*Uses UDP to send the packet_data*/
int send_data();

/*Uses UDP to receive the packet_data*/
void *recv_data(void*);

/* Open TCP conn */
int TCPtoUDPconn();

/* Open UDP conn */
int UDPconn();

/* Send an ACK for the data */
int send_ack(int);

int write_to_file(FILE *fp, unsigned char *data, long long size);
void resetConnState();
FILE* open_file(char* filename);
int recv_UDP(packet_t *p);
int packet_deepcopy(packet_t *dst, packet_t *src);
void packet_destroy(packet_t *pkt);

void * NAKSender(void * arg);

void * progress_thread(void *arg);

void * BufferedWriter(void * arg);

/* send a nack with ack_no */
int send_nack(nak_packet_t *);

/* UDP client */
int udpClient (session_t *localSession);

/* Send an ACK for the data */
int send_ack_udp(int);

void setParameters();



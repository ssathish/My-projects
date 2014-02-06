/*server.h*/
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include "common.h"

#define RECEIVER_UDP_CONTROL_PORT   10001       // file name sent, ACKs received
#define RECEIVER_UDP_DATA_PORT      6001       // transfer 
#define MY_UDP_PORT		            7001
#define NAK_UDP_PORT                8001

#define TCP_SEND_BUFFER             1024

// Updated Parameter
//#define FILE_READ_BUFFER            1440*745655
#define NUMBER_OF_COPIES                1
#define NUMBER_OF_COPIES_TO_RETRANSMIT  1
#define RETRANSMIT_READ_BUFFER      65536

typedef struct session
{
    int id;
    struct stat _stat;                  /*file stats*/
    FILE * fp;             
    int    tcp_sockfd;
    int    udp_sockfd; 
    int    udp_nak_sockfd;                  
    size_t packet_size;                 /*size of each packet*/
    int seq_num;                      // Current Sequence number
    int retransmit_seq_num;         // Sequence number to retrasmit 
    
    unsigned char endOfFile;
    char local_file_path[MAX_PATH_LENGTH];
    struct sockaddr_in udpSendTo; 
    struct sockaddr_in myAddr; 
    char  dest_name[MAX_HOSTNAME_LENGTH];	// destination hostname
    int num_packets;
    struct timeval start, end;

    int startSequenceNum;
    int lastSequenceNum;
    unsigned int lastSeqPktSize;

	double endToEndDelay;
	int nbr_retransmissions;
	int nbr_transmissions;
	long inter_packet_delay;
	
	nak_packet_t nak;
} session_t;


// wrapper for the tcp_controller thread input arg
typedef struct sess_ctl
{
	session_t *p_session;
	control_t *p_control;
} sess_ctl_t;

pthread_mutex_t mtx_ack_check;
unsigned int nak_recvd;
unsigned int end_xmit;

pthread_mutex_t mtx_thread_ctl;
pthread_cond_t	cv_send;// variable to signal udp thread to start sending the data
unsigned int recvd_start_signal = 0;
/*
1. check file permissions (validate) - use stat
2. 
*/
FILE * open_file (char *file_name);

/*we should be able to seek and read particular chunk of file, beginning at a byte offset*/
size_t read_data (FILE * fp, size_t bytes_to_send);

/*inputs: RTT
output: optimal packet size*/
size_t calc_packet_size(); // Venkat

/*procedure:
calculate bandwidth delay product*/
size_t calc_num_packets_to_send();

/* Used for opening UDP connection and populating
socket descriptor in session */
int udpClient ( session_t *localSession);

void setParameters(session_t *);


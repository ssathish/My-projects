/*
	common.h
*/

/*#define DEBUG 1
#define DEBUG_2 2
#define DEBUG_3
*/
#define SHOW_PROGRESS


//#define TEXT_DISPLAY

#define FILE_NAME_LENGTH            256
#define MAX_PATH_LENGTH             1024
#define DEST_NAME_LENGTH            256
#define MAX_HOSTNAME_LENGTH         256

#define PACKET_SIZE                1440
// recvfrom buffer size - sockopt
#define SOCKOPT_RECVFROM_BUFFER     (131072)

#define TEST_DELAY_ONEMS           18 

#define CTRL_PACKET_DUP_COUNT   10
//End of transfer ACK - copies
#define LASK_ACK_DUP_COUNT  1
#define FIRST_ACK_DUP_COUNT  20


//NAKSender Timeout
#define NAK_SENDER_TIMEOUT_USECS  (10000)

#define DELAY_MULTIPLIER	2

#define NAK_ARRAY_SIZE		350

typedef struct control
{
    char        dest_dir[MAX_PATH_LENGTH];		// where the file needs to be placed at the receiver
    char        file_name[FILE_NAME_LENGTH];      // name of the file being transferred
    //char      dest_name[MAX_HOSTNAME_LENGTH];	// destination hostname
    long long   file_size;					// size
    int   	num_packets;					// todo: number of packets that make up the file
    time_t      tv_sec;
    suseconds_t tv_usec;   
} control_t;

typedef struct control_ack
{
    int 	ack_no;
    time_t      tv_sec;
    suseconds_t tv_usec;   
} control_ack_t;

typedef struct header { 
  int           end_of_file;
  int     	pkt_id;
  unsigned int  pkt_size;
} header_t;

/* data packet to be sent over UDP*/
typedef struct packet
{
    header_t        msg_header;
    unsigned char   pkt_data[PACKET_SIZE];    
          /*in bytes*/
} packet_t;

// we receive ack on TCP
typedef struct ack_packet
{
	int	seq_no; 
    	int 	ack_no;        // carries the nak
} ack_packet_t;

// we receive nak on UDP
typedef struct nak_packet {
    	int seq_no;
	int ack_no;  /* only filled in case of last packet */
    	int nbr_naks;
    	int nak_array[NAK_ARRAY_SIZE];
} nak_packet_t;



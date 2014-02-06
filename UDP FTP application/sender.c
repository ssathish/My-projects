/*sender main file*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#include "sender.h"

void make_packet(session_t *,unsigned char *, int);
void send_data(session_t* , packet_t*, int );
void calculate_throughput(session_t* , control_t* );

int packet_deepcopy(packet_t *dst, packet_t *src)
{
    if(src == NULL)
        return -1;

    memcpy((unsigned char *)&dst->msg_header, (unsigned char *)&src->msg_header, sizeof(header_t));
    memcpy((unsigned char *)dst->pkt_data, (unsigned char *)src->pkt_data, PACKET_SIZE);    

    return 0;
}

// thread function 1: tcp sender and ack receiver
void * udp_controller(void * arg)
{
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    size_t c_len = sizeof(control_t);
    unsigned char send_buffer[c_len];
	unsigned char *recv_buffer = NULL;
	control_ack_t *p_ack_pkt;
	struct timeval cur_time;    

#ifdef DEBUG
    fprintf (stdout, "In UDP Controller: Thread started\n");
#endif
    sess_ctl_t *pwrap = (sess_ctl_t *) arg;

	control_t *pctl = pwrap->p_control;
	session_t *pses = pwrap->p_session;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { 
        fprintf (stderr, "udp controller: ERROR opening tcp socket\n");
		return NULL;
	}

	// todo: set sockopt - keep alive

    server = gethostbyname(pses->dest_name);
    if (server == NULL) {
        fprintf(stderr,"udp controller: ERROR no such destination host\n");
        return (NULL);
    }
    
    memset((char *) &serv_addr,0, sizeof(serv_addr));
	
// set the time of day
    gettimeofday(&cur_time, NULL); 
    pctl->tv_sec = cur_time.tv_sec;
    pctl->tv_usec = cur_time.tv_usec;

    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    bcopy((char *)server->h_addr, (char *)&pses->udpSendTo.sin_addr.s_addr, server->h_length);

    serv_addr.sin_port = htons(RECEIVER_UDP_CONTROL_PORT);
    

    /*if (connect(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)  {
        fprintf(stderr, "udp controller: ERROR connecting to destination host\n");
		return NULL;
	}*/

#ifdef DEBUG
	//fprintf(stdout, "%s (%d): My TCP port No-->%d, Remote host name->%s, Remote Port no-->%d", __FILE__, __LINE__,
	
#endif
	// send the control information
	memset(send_buffer,0, c_len);
	/* Done: Populate the control packet contents */
	memcpy(send_buffer, pctl, c_len);

    int z = 0;
    for (; z < CTRL_PACKET_DUP_COUNT; ++z) {
        
        n = sendto(sockfd,send_buffer,sizeof(send_buffer),0,(struct sockaddr *) &serv_addr, sizeof(serv_addr));
        
        //n = write(sockfd, send_buffer, sizeof(send_buffer));
        if (n < 0) {
           fprintf(stderr, "udp_controller: ERROR writing to socket");
    	   return NULL;
    	}
        usleep(TEST_DELAY_ONEMS);
    }

#ifdef DEBUG
    fprintf (stdout, "In udp controller: Waiting for first ACK\n");
#endif
	recv_buffer = (unsigned char *)malloc(sizeof(control_ack_t));
    while (1) 
    // for (i = 0; i < 1; ++i)
    {
        memset(recv_buffer, 0, sizeof(control_ack_t));

        socklen_t len = sizeof(server);
		n = recvfrom(sockfd, recv_buffer, sizeof(control_ack_t), 0,(struct sockaddr *) &server, &len);	// flags = 0, behaves like write
		if (n < 0) {
             fprintf (stderr, "udp controller: ERROR reading from socket\n");
			 return NULL;
		}
		
		p_ack_pkt = (control_ack_t *) recv_buffer;
		if (p_ack_pkt->ack_no == -1) {

#ifdef DEBUG            
		fprintf (stdout, "udp controller: recvd ack for control packet\n");
            	fprintf (stdout, "In udp controller: Signaling the UDP sender to start\n");
#endif

		//calculate end-to-end delay
		struct timeval now;
		
		//Get time stamp measurement
		if(gettimeofday(&now, 0) < 0)
		{
			fprintf(stderr, "gettimeofday returned error!!!\n");
			exit(0);
		}

		
	 	pses->endToEndDelay = (double)((now.tv_sec * 1000000 + now.tv_usec) - (p_ack_pkt->tv_sec * 1000000 + p_ack_pkt->tv_usec)) * DELAY_MULTIPLIER / (double) 1000.0;

		setParameters(pses);

			pthread_mutex_lock(&mtx_thread_ctl);
			recvd_start_signal = 1;

			pthread_cond_signal (&cv_send);
			pthread_mutex_unlock(&mtx_thread_ctl);

            break;

		} /*else {
			
            pthread_mutex_lock(&mtx_ack_check);
			if (p_ack_pkt->ack_no  >= pses->num_packets) {
				end_xmit = 1;

                gettimeofday(&pses->end, NULL);

				pthread_mutex_unlock(&mtx_ack_check);
#ifdef DEBUG
                fprintf (stdout, "In udp controller: recvd last ACK from recv\n");
                fprintf (stdout, "In udp controller: UDP sender notified to stop\n");
                fprintf (stdout, "In udp controller: signalling UDP receiver to stop\n");

#endif
                shutdown(pses->udp_nak_sockfd, SHUT_RDWR);

				break;

			} 
			pthread_mutex_unlock(&mtx_ack_check);
		}*/
        
        // fprintf(stdout, "udp controller: server says:%s",buffer);
    }

	free(recv_buffer);
	close(sockfd);
	return NULL;
}

void *udp_receiver(void *arg)
{
    // sess_ctl_t *pwrap = (sess_ctl_t *) arg;
    int sockfd, n;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    size_t pkt_size = sizeof(nak_packet_t);
    unsigned char msg[pkt_size];
    int expected_seqno = 0;
    nak_packet_t *p_ack = NULL;
    int recvfrom_buf = SOCKOPT_RECVFROM_BUFFER;

    int start_seq=0;

#ifdef SELECT    
    // for select
    struct timeval timeout;
    int readsocks = 0;
    fd_set socks;
    int highsock;


    timeout.tv_sec = 3; // every 3 seconds we wake up if the transmission has ended
    timeout.tv_usec = 0;
#endif

    sess_ctl_t *pwrap = (sess_ctl_t *) arg;

    control_t *pctl = pwrap->p_control;
    session_t *pses = pwrap->p_session;

    fprintf(stdout, "in udp_receiver: thread started\n" );
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "udp_receiver:Failed to create a datagram socket\n");
        return NULL;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recvfrom_buf, sizeof(int)) == -1) {
        fprintf(stderr, "udp_receiver:Failed to set the receiver buffer\n");
        return NULL;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &recvfrom_buf, sizeof(int)) == -1) {
        fprintf(stderr, "udp_receiver:Failed to set the receiver buffer\n");
        return NULL;
    }


    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(NAK_UDP_PORT);
    bind(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr));

    pses->udp_nak_sockfd = sockfd;

#ifdef SELECT
    // set the parameters for select
    highsock = sockfd;
    FD_ZERO(&socks);
    FD_SET(sockfd, &socks);
#endif

    while (1) {
        len = sizeof(cliaddr);

#ifdef SELECT
        // use select and timed wait here
        readsocks = select(highsock+1, &socks, (fd_set *) 0, (fd_set *) 0, &timeout);

        if (readsocks < 0) {
            fprintf(stderr, "Error in select\n");
            return NULL;
        } 
        
        if (readsocks == 0) {
            /* Nothing ready to read, just show that
               we're alive */
#endif            
            pthread_mutex_lock(&mtx_ack_check);
            
            if (end_xmit) {
                fprintf(stdout, "Exiting from udp receiver thread!\n");
                pthread_mutex_unlock(&mtx_ack_check);
                break;
            }
                
            pthread_mutex_unlock(&mtx_ack_check);

#ifdef SELECT            
        } else {
#endif
            // read retransmission seq num from global
            pthread_mutex_lock(&mtx_ack_check);
            start_seq = pses->startSequenceNum;

            pthread_mutex_unlock(&mtx_ack_check);

            memset(msg, 0, pkt_size);
            n = recvfrom(sockfd, msg, sizeof(msg) , 0, (struct sockaddr*)&cliaddr, &len);

            //fprintf(stdout, "udp receiver: received %d bytes\n", n);

            if (n == sizeof(nak_packet_t)) {

                p_ack = (nak_packet_t*) msg;
#ifdef DEBUG 
        fprintf(stdout,"Size of NAK_PACKET :%d\n",sizeof(pses->nak));  
        fprintf (stdout, "Gotta NAK !!, expected %d , Got %d\n",expected_seqno,p_ack->seq_no);
#endif

                if (expected_seqno <= p_ack->seq_no) {
#ifdef DEBUG    
        fprintf (stdout, "Processing NAK now  !!\n");
#endif
                    /* Handle a valid NAK 
                        - Set the flag
                        - Notify udp sender
                    */

                    // handle the end of transfer scenario
//-------------------------Segment from controller begins------------------------
                    pthread_mutex_lock(&mtx_ack_check);
                    if (p_ack->ack_no  >= pses->num_packets) {
                        end_xmit = 1;

                        gettimeofday(&pses->end, NULL);

                        
#ifdef DEBUG    
                        fprintf (stdout, "In udp controller: recvd last ACK from recv\n");
                        fprintf (stdout, "In udp controller: UDP sender notified to stop\n");
                        fprintf (stdout, "In udp controller: signalling UDP receiver to stop\n");
#endif
                        calculate_throughput(pses, pctl);

                        pthread_mutex_unlock(&mtx_ack_check); 

                        shutdown(pses->udp_nak_sockfd, SHUT_RDWR);

                        break;

                    } 
                    pthread_mutex_unlock(&mtx_ack_check);
//-------------------------Segment from controller ends------------------------

#ifdef DEBUG
                    fprintf(stdout, "udp receiver: nak sq expected %d arrived %d - packet requested: %d\n",
                        expected_seqno, p_ack->seq_no, p_ack->ack_no );
#endif
                    pthread_mutex_lock(&mtx_ack_check);
                    
                    if (p_ack->ack_no >= start_seq) {

                    // UPDATED: re-transmit sequence no
                        //pses->retransmit_seq_num = p_ack->ack_no;
                        expected_seqno = p_ack->seq_no + 1;          
                        nak_recvd = 1;
			            memcpy((unsigned char *) &pses->nak, p_ack, sizeof(pses->nak));
			
                    } else {
#ifdef DEBUG
                    fprintf(stdout, "udp receiver: not servicing NAK %d\n", p_ack->ack_no);
#endif                        
                    }
                    pthread_mutex_unlock(&mtx_ack_check);      

                }
            }
#ifdef SELECT       
        }
#endif
    }

    close(sockfd);
    return NULL;
}

int initialize(session_t *psession, control_t * pcntl, const char *file_name, const char *rem_host, const char *rem_dir)
{
    int ret = 0;

#ifdef DEBUG
    fprintf(stdout, "Entered initialize\n");
#endif    
    pthread_mutex_init(&mtx_ack_check, NULL);
	pthread_mutex_init(&mtx_thread_ctl, NULL);
	pthread_cond_init (&cv_send, NULL);

    memset(psession, 0, sizeof (session_t));
    memset(pcntl, 0, sizeof(control_t));

    // set the session paramters
    ret = stat(file_name, &psession->_stat);
    if (ret < 0) {
        fprintf(stderr, "stat call failed: error");
        return 0;
    }
    strncpy(psession->local_file_path, file_name, MAX_PATH_LENGTH);

    // set the control packet
    strncpy(pcntl->file_name, file_name, FILE_NAME_LENGTH);
    strncpy(pcntl->dest_dir, rem_dir, MAX_PATH_LENGTH);
    strncpy(psession->dest_name, rem_host, DEST_NAME_LENGTH);
    pcntl->file_size = (long long) (psession->_stat.st_size);
	
    pcntl->num_packets = (int) (psession->_stat.st_size / PACKET_SIZE);

    if (psession->_stat.st_size % PACKET_SIZE > 0)
		pcntl->num_packets += 1;
    
    psession->num_packets = pcntl->num_packets;	

#ifdef DEBUG
    fprintf(stdout, "File name: %s\n", pcntl->file_name);
    fprintf(stdout, "Dest dir: %s\n", pcntl->dest_dir);
    fprintf(stdout, "Dest Host name: %s\n", psession->dest_name);
    fprintf(stdout, "File size: %lld\n", pcntl->file_size);
    fprintf(stdout, "Calculated number of packets: %d\n", psession->num_packets);
#endif

    return ret;
}

void* udp_sender( void* arg) { 
  
    // open File and update the file pointer in the session_t
    
    FILE *fp;
    unsigned char* fileReadBuffer=NULL;
    unsigned char* retrnsmitBufferSize=NULL;
    long long file_read_buffer = 0;
    
    /* Not Need for Multiple NACK Thing */
    int retrans_seq_no = 0; 

    /* Amogh: begin */
    /* We need to wait until the first ACK has been received to ensure control info has reached the receiver */

#ifdef DEBUG
    fprintf (stdout, "In UDP Sender: Thread started\n");
#endif

    sess_ctl_t *pwrap = (sess_ctl_t *) arg;

    session_t *pses = pwrap->p_session;
    control_t *pcntl = pwrap->p_control;

    file_read_buffer = pcntl->file_size;

    pthread_mutex_lock(&mtx_thread_ctl);
    while (recvd_start_signal == 0) {
        pthread_cond_wait(&cv_send, &mtx_thread_ctl);
    }
    pthread_mutex_unlock(&mtx_thread_ctl);
    /* Amogh: end */

    if (recvd_start_signal) {
        // start sending the data
        
        fp = open_file(pses->local_file_path);
        pses->fp = fp;  
     
    /* UPDATED: Adding UDPClient for UDP connection */
    (void) udpClient(pses);

        // We need to get the packet , We will make make_packet,  
        fileReadBuffer = (unsigned char *) malloc(file_read_buffer);

        if(NULL == fileReadBuffer) { 
            fprintf(stderr, "Malloc returned error !!,When requested for file_read_buffer\n");
            exit(0);
        } 

        // For Retransmission 

        retrnsmitBufferSize = (unsigned char *) malloc(RETRANSMIT_READ_BUFFER);

        if(NULL == retrnsmitBufferSize) { 
            fprintf(stderr, "Malloc returned error !!,When requested for retrnsmitBufferSize\n");
            exit(0);
        } 

        int retransmission = 0;
        int readData=1;
        
        while (1) { 

            if (readData) {
                int nbytes;
#ifdef DEBUG
    fprintf(stdout,"Now Reading and getting Data into Buffer,%d\n",pses->startSequenceNum);
#endif          
                /* 
                pthread_mutex_lock(&mtx_ack_check);
                pses->startSequenceNum = pses->lastSequenceNum;
                pthread_mutex_unlock(&mtx_ack_check);
                */
                /*if (fseek(fp,pses->startSequenceNum*PACKET_SIZE, SEEK_SET) < 0  )  { 
                   fprintf(stderr, "fseek returned error\n");
                    exit(0);
                }*/
                memset(fileReadBuffer,0, file_read_buffer);
                nbytes=fread(fileReadBuffer,1,file_read_buffer,fp); 
                if ( nbytes < 0 ) { 
                    fprintf(stderr,"fread returned error\n");
                    exit(0);
                }
                
                // Logic to get to Know about the last sequence number read                 
                int max=0;
                int rem=0;
                
                rem = nbytes % PACKET_SIZE; 
                if ( rem == 0 )  { 
                    max = (nbytes/PACKET_SIZE)-1; 
                    pses->lastSeqPktSize=PACKET_SIZE;
                } else { 
                        max = (nbytes/PACKET_SIZE);
                        pses->lastSeqPktSize=rem;
                }
                pses->lastSequenceNum=pses->startSequenceNum + max;
                readData=0;
            }
            
            /*
pthread_mutex_lock(&mtx_ack_check);
//Not Needed for Multiple NAK
            retrans_seq_no = pses->retransmit_seq_num;
pthread_mutex_unlock(&mtx_ack_check);
            */

            if (pses->endOfFile !=  1) {
                if ( retrans_seq_no > pses->lastSequenceNum)  {
                    readData=1;
                    /*pthread_mutex_lock(&mtx_ack_check);
                    pses->startSequenceNum = pses->lastSequenceNum ;
                    pthread_mutex_unlock(&mtx_ack_check);
                    */
                } else {
                       if ( pses->seq_num > pses->lastSequenceNum) {
                         // Do Nothing , Wait for NAK 
                       } else { 
                            make_packet(pses,fileReadBuffer,retransmission);
                       }
                }
            } /*else if (pses->endOfFile == 1) { 
                printf("End of Packet Hit , Will reset to last Retransmit valile  %d \n",retrans_seq_no);
                pses->endOfFile= 0;
                localSeq=retrans_seq_no;
                // UPDATE Restarting 
                //pses->seq_num=0;
            } */

            pthread_mutex_lock(&mtx_ack_check);   

            if(nak_recvd) {
                nak_recvd=0;
                retransmission = 1;
            /* TODO: We can buffer the nak_list contenets and release the
                lock before entering make_packet */
                if ( retrans_seq_no > pses->lastSequenceNum)   {
                    readData=1;
                    pses->startSequenceNum=pses->lastSequenceNum+1 ;
                } else {
                    make_packet(pses,fileReadBuffer,retransmission);
                }
                retransmission = 0;
            } else { 
            }
            
            pthread_mutex_unlock(&mtx_ack_check);

            if (end_xmit) {
#ifdef DEBUG
                fprintf (stdout, "udp sender: ending transmission\n");
#endif
                close(pses->udp_sockfd);
                fclose(pses->fp);
                free(fileReadBuffer);
                free(retrnsmitBufferSize);
                break;
            }
        }
    }
    return NULL;    
}

void make_packet(session_t* s, unsigned char *fileReadBuffer, int retransmit) 
{ 
    packet_t *p=NULL;   
    p=(packet_t*) malloc( sizeof(packet_t));
    unsigned char *cur_read_loc = NULL;
    int i=0; // For NAK
    
    if ( NULL == p) { 
        fprintf(stderr,"udp_sender: make_packet, Malloc returned an error \n");
        exit(0);
    }

    FILE *fp = NULL;
    if (NULL != s)
        fp = s->fp;
    
    if (NULL == fp) {
        fprintf(stderr, "udp sender: cannot open file - null pointer\n");
        // error
        exit(0);
    }

    if (retransmit) { 
#ifdef DEBUG
        fprintf(stdout, "Entered retransmit section\n");
#endif      
        
        for (i=0;i<s->nak.nbr_naks;i++) {
#ifdef DEBUG
        fprintf(stdout, "Now Retransmitting packet %d\n",s->nak.nak_array[i]);
#endif 

            cur_read_loc = fileReadBuffer + (s->nak.nak_array[i]-s->startSequenceNum)*PACKET_SIZE;
            if (s->nak.nak_array[i] == s->num_packets-1) {
                memcpy(p->pkt_data,cur_read_loc,s->lastSeqPktSize);
                p->msg_header.pkt_size=s->lastSeqPktSize;
                p->msg_header.pkt_id=s->nak.nak_array[i];   
                p->msg_header.end_of_file = 1;
                s->endOfFile = 1;
            } else {
                 if ( s->nak.nak_array[i] == s->lastSequenceNum ) { 
                    memcpy(p->pkt_data,cur_read_loc,s->lastSeqPktSize);
                    p->msg_header.pkt_size=PACKET_SIZE;
                    p->msg_header.pkt_id=s->nak.nak_array[i]; 
                    p->msg_header.end_of_file = 0;
                    s->endOfFile = 0;
                } else { 
                    memcpy(p->pkt_data,cur_read_loc,PACKET_SIZE);
                    p->msg_header.pkt_size=PACKET_SIZE;
                    p->msg_header.pkt_id=s->nak.nak_array[i]; 
                    p->msg_header.end_of_file = 0;
                    s->endOfFile = 0;       
                }
            }
            send_data(s,p,s->nbr_retransmissions); 
        }    
    } else { 

            cur_read_loc = fileReadBuffer + (s->seq_num-s->startSequenceNum)*PACKET_SIZE;
        if (s->seq_num == s->num_packets-1) {
              memcpy(p->pkt_data,cur_read_loc,s->lastSeqPktSize);
              p->msg_header.pkt_size=s->lastSeqPktSize;
              p->msg_header.pkt_id=s->seq_num;  
              p->msg_header.end_of_file = 1;
              s->endOfFile = 1;
         } else {
             if ( s->seq_num == s->lastSequenceNum ) { 
                memcpy(p->pkt_data,cur_read_loc,s->lastSeqPktSize);
                p->msg_header.pkt_size=PACKET_SIZE;
                p->msg_header.pkt_id=s->seq_num;    
                p->msg_header.end_of_file = 0;
                s->endOfFile = 0;
            } else { 
                memcpy(p->pkt_data,cur_read_loc,PACKET_SIZE);
                p->msg_header.pkt_size=PACKET_SIZE;
                p->msg_header.pkt_id=s->seq_num;    
                p->msg_header.end_of_file = 0;
                s->endOfFile = 0;       
            }
            
         }
         s->seq_num++;

         send_data(s, p, s->nbr_transmissions);   
    }
    free(p);
}

void send_data(session_t* s,packet_t* p, int numCopy) 
{ 
    int retcode;
    int i=0;
#ifdef DEBUG
    fprintf(stdout, "Sending Packet: %d\n end of file: %d\n", p->msg_header.pkt_id,
                p->msg_header.end_of_file);
    fprintf(stdout, "packet size: %u\n", p->msg_header.pkt_size);

#endif

    for (i=0;i<numCopy;i++) {
#ifdef TEXT_DISPLAY
        fprintf(stdout, "\n\n############packet ID : %llu########,\n%s\n############\n\n",
            p->msg_header.pkt_id,p->pkt_data);
#endif
       retcode = sendto(s->udp_sockfd,(void*)p,PACKET_SIZE+sizeof(header_t),0,(struct sockaddr *) &s->udpSendTo,
            sizeof(s->udpSendTo));
       if (retcode <= -1) {
          printf("client: sendto failed: %d\n",errno); 
          exit(0);  
       }
       usleep(s->inter_packet_delay);
    }
      
}

FILE* open_file(char* filename) 
{ 
    
	FILE *fp=NULL;
	fp = fopen(filename,"rb");
	if ( fp == NULL ) { 
		fprintf(stderr,"fopen returned error\n");
		exit(0);
	}

#ifdef DEBUG
    fprintf(stdout, "File :%s opened successfully for sending\n", filename);
#endif
	return fp;
}

void calculate_throughput(session_t *s, control_t *c) 
{
    double throughput = 0.0f;
    long timediff = 0;

    fprintf(stdout, "Timediff: %ld (us)\n", ((s->end.tv_sec * 1000000 + s->end.tv_usec)
          - (s->start.tv_sec * 1000000 + s->start.tv_usec)));
    
    timediff = ((s->end.tv_sec * 1000000 + s->end.tv_usec) - (s->start.tv_sec * 1000000 + s->start.tv_usec));

    throughput = ( (double) (c->file_size) * 8.0) / ((double)(timediff) * (1.024 * 1.024));
    fprintf(stdout, "Throughput: %lf (Mbps)\n", throughput);
}

/*switch to getopt or getoptlong*/
int main(int argc, char **argv)
{
    int err = 0;
	sess_ctl_t sctl;
	pthread_t tcp_th_id, udp_th_id, udp_th_id2;
	// pthread_attr_t attr;
	void *res;	// pthread_join result - don't care

    session_t ft_session;
    control_t ft_control;

    if (argc < 3) {
        fprintf(stderr, "Need 3 arguments atleast\n");
        fprintf(stderr, "sender <local_file> <remote_host> <remote_dir>\n\n");
        return 0;
    }
    // fill up the ft_session
	

    err = initialize(&ft_session, &ft_control, argv[1], argv[2], argv[3]);
    if (err < 0) {
		fprintf(stderr, "could not initialize\n");
		return 0;
    }

	sctl.p_session = &ft_session;
	sctl.p_control = &ft_control;

    gettimeofday(&(ft_session.start), NULL);

	err = pthread_create (&tcp_th_id, NULL, &udp_controller, &sctl);
	if (err < 0) {
		fprintf (stderr, "error in pthread_create udp_controller\n");
		return 0;
	}
		
	err = pthread_create (&udp_th_id, NULL, &udp_sender, &sctl);
	if (err < 0) {
		fprintf (stderr, "error in pthread_create udp_sender\n");
		return 0;
	}

    err = pthread_create (&udp_th_id2, NULL, &udp_receiver, &sctl);
    if (err < 0) {
        fprintf (stderr, "error in pthread_create udp_receiver\n");
        return 0;
    }
	/*
	err = pthread_attr_destroy(&attr);
	if (err < 0) {
		fprintf (stderr, "error in pthread_attr_destroy\n");
		return 0;
	}
	*/

	err = pthread_join(tcp_th_id, &res);
	if (err < 0) {
		fprintf (stderr, "error in pthread_join udp_controller\n");
		return 0;
	}

	err = pthread_join(udp_th_id, &res);
	if (err < 0) {
		fprintf (stderr, "error in pthread_join udp_sender\n");
		return 0;
	}

    err = pthread_join(udp_th_id2, &res);
    if (err < 0) {
        fprintf (stderr, "error in pthread_join udp_receiver\n");
        return 0;
    }   
    
	return err;
}

int udpClient ( session_t *localSession)  { 
		
	int sockfd; 
#ifdef DEBUG    
	fprintf (stdout, "Entered udpClient: Creating UDP socket\n" );
#endif
	// Creating the socket 
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)  {
		fprintf(stderr,"Server :socket error: %d\n",errno);
	    exit(0); 
	}
	
	// Setting the Struct to Zero , Don't use Memset , it doesn't work 
	memset((char *) &localSession->myAddr, 0, sizeof(localSession->myAddr)); 
	localSession->myAddr.sin_family= AF_INET;
	localSession->myAddr.sin_addr.s_addr=htons(INADDR_ANY);
	localSession->myAddr.sin_port = htons(MY_UDP_PORT);

	// Now Bind this Addr
    if ( (bind(sockfd, (struct sockaddr *) &localSession->myAddr, sizeof(localSession->myAddr)) < 0) ) { 
    	fprintf(stderr,"Server: bind fail: %d\n",errno); 
    	exit(0); 
    }

    localSession->udpSendTo.sin_family=AF_INET;
    localSession->udpSendTo.sin_port=htons(RECEIVER_UDP_DATA_PORT);
    localSession->udp_sockfd= sockfd;
    
    return sockfd;
}

void setParameters(session_t *sess)
{

#ifdef DEBUG
	fprintf(stdout, "EndtoEnd Delay is %lf msec. Setting parameters\n", sess->endToEndDelay);
#endif
	
	if(sess->endToEndDelay < 200)
	{
		sess->inter_packet_delay = 18; /* in Microsecs */
		sess->nbr_retransmissions = 1;
		sess->nbr_transmissions = 1;
	}
	else
	{
		sess->inter_packet_delay = 1; /* in Microsecs */
		sess->nbr_retransmissions = 1;
		sess->nbr_transmissions = 1;
	}

#ifdef DEBUG
	fprintf(stdout, "Parameters set:\n\t# of Retransmissions - %d\n\t# of Transmissions - %d\n\tinter-packet delay - %ld\n", sess->nbr_retransmissions, sess->nbr_transmissions, sess->inter_packet_delay);
#endif

}





#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/time.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
#include <pthread.h>
#include <stdint.h>
#include "receiver.h"
	

unsigned char* bytemap;

int isSet(int x)
{
	return (int) *((unsigned char*)bytemap + x);
}

void setByte(int x)
{
	*(bytemap + x) = 1;	
}

void clearByte(int x)
{
	*(bytemap + x) = 0;		
}

void error(const char *msg)
{
	perror(msg);
    	exit(1);
}

int main(int argc, char** argv)
{

	pthread_t thr_udp_naksndr, thr_udp_recvr, thr_prog_thr;/*, thr_buf_writer*/;
	void *res;	// pthread_join result - don't care
	int err = 0;

	DEBUG_MSG("In Main");

	//Check command line arguments
	//fetch port?

	while(1)
	{
		//Clear the session variable
		memset((unsigned char *) &sd, 0, sizeof (session_t));
	
		//check connection state
		//Open and bind a TCP socket
		if(TCPtoUDPconn() < 0)	// earlier TCP now changed to UDP - should update comments and 
			// names later
		{
			DEBUG_ERR("Error opening TCP socket");
		}

		//block for data read
		(void ) recv_control();

		int i;
		//Send ack
		for(i=0; i < FIRST_ACK_DUP_COUNT; i++)
		{
			(void) send_ack(-1);
		}

		//change state
		sd.conn_state = CONT_ACK_SENT;

		//Open and bind UDP socket
		if(UDPconn() < 0)
		{
			DEBUG_ERR("Error opening UDP socket");
		}

		sd.expected_seqno = 0;
		sd.seq_no =0;
		sd.sendrAddressUpdated = 0;
		wbuf.data = (unsigned char *) malloc(sizeof(char) * sd.num_packets * PACKET_SIZE);
#ifdef MEMCHK
		fprintf(stdout, "Loc data: %u, data+5 : %u, Size of data %lu, size of char %lu\n", (wbuf.data), ((unsigned char *)wbuf.data + (int)5),sizeof(wbuf.data), sizeof(char));
		
#endif
		memset(wbuf.data, 0, sd.num_packets * PACKET_SIZE);	

		//Initialize Mutex
		pthread_mutex_init(&mtx_nak_sndr, NULL);
		pthread_cond_init (&cv_naksndr_start, NULL);
		//pthread_mutex_init(&mtxwbuf_wrtr, NULL);
		//pthread_cond_init (&cvwbuf_wrtr, NULL);


		char *filename = (char *) malloc (sizeof(char) * (MAX_PATH_LENGTH + FILE_NAME_LENGTH));
		sprintf(filename, "%s/%s",  sd.dest_dir, sd.file_name);

		//Open file to write
		sd.fp = open_file(filename);
		/* TODO: Need to return error if file opening failed. 
			The error needs to be communicated to sender */

		//Start the UDP receiver thread
		err = pthread_create (&thr_udp_recvr, NULL, &recv_data, NULL);
		if (err < 0)
			error("error in pthread_create UDP receiver\n");

		//Start the NAKsender thread
		err = pthread_create (&thr_udp_naksndr, NULL, &NAKSender, NULL);
		if (err < 0)
			error("error in pthread_create NAKs sender\n");
		
		//Start the progress thread
		err = pthread_create (&thr_prog_thr, NULL, &progress_thread, NULL);
		if (err < 0)
			error("error in pthread_create progress tracker\n");
		/*
		//Start the BufWriter thread
		err = pthread_create (&thrwbuf_writer, NULL, &BufferedWriter, NULL);
		if (err < 0)
			error("error in pthread_create Buffered writer\n");
		*/

		//Wait for UDP receiver to finish executing
		err = pthread_join(thr_udp_recvr, &res);
		if (err < 0) {
			error("error in pthread_join NAKs sender\n");
		}

		//Wait for NAK sender to finish executing
		err = pthread_join(thr_udp_naksndr, &res);
		if (err < 0) {
			error("error in pthread_join NAKs sender\n");
		}

		err = pthread_join(thr_prog_thr, &res);
		if (err < 0) {
			error("error in pthread_join Progress Tracker\n");
		}
		/*
		//Wait for NAK sender to finish executing
		err = pthread_join(thrwbuf_writer, &res);
		if (err < 0) {
			error("error in pthread_join Buf writer\n");
		}
		*/

		//Clear all state from receiver
		resetConnState();
	}
}

//Open and bind a TCP socket
int TCPtoUDPconn()
{
	DEBUG_MSG("In TCPtoUDPconn");

	int sockfd /*, childsockfd*/;
     	struct sockaddr_in serv_addr/*, cli_addr*/;
     	int opt=1 ;
    	/*socklen_t clilen;*/

	//open socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
     	if (sockfd < 0) 
        	error("ERROR opening socket");

	
     	memset((unsigned char *) &serv_addr,0, sizeof(serv_addr));
     	serv_addr.sin_family = AF_INET;
     	serv_addr.sin_addr.s_addr = INADDR_ANY;
     	serv_addr.sin_port = htons(RECEIVER_UDP_CONTROL_PORT);

     	//set socket options to reuse a port
     	if( setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(const char *)&opt,sizeof(int)) < 0)
		error("ERROR on setsockopt");
	
	//
     	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
		error("ERROR on binding");

#ifdef DEBUG
    	fprintf(stdout, "%s (%d): Opened Control UDP socket on port %d\n", __FILE__, __LINE__, RECEIVER_UDP_CONTROL_PORT);
#endif

	// Populate the TCP listen socket
	sd.tcp_listen_sockfd = sockfd;
	
	//Listen
	listen(sockfd,5);

	//Accept
	/*clilen = sizeof(cli_addr);
     	childsockfd = accept(sockfd, 
               		(struct sockaddr *) &cli_addr, 
                 &clilen);

     	if (childsockfd < 0) 
          	error("ERROR on accept");*/

#ifdef DEBUG
    	fprintf(stdout, "%s (%d): Got a coonection request. Opened Control UDP socket %d\n", __FILE__, __LINE__, sockfd);
#endif

	// Populate the TCP child socket
	sd.tcp_sockfd = sockfd;

	return 0;
}

//Open and bind a UDP socket
int UDPconn()
{

	DEBUG_MSG("In UDPconn");
	int sockfd;
     	struct sockaddr_in serv_addr;
     	int opt=1;
    	const int buff_size = SOCKOPT_RECVFROM_BUFFER;
 
	//open socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
     	if (sockfd < 0) 
        	error("ERROR opening socket");

	
     	memset((unsigned char *) &serv_addr,0, sizeof(serv_addr));
     	serv_addr.sin_family = AF_INET;
     	serv_addr.sin_addr.s_addr = INADDR_ANY;
     	serv_addr.sin_port = htons(RECEIVER_UDP_DATA_PORT);

     	//set socket options to reuse a port
     	if( setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(const char *)&opt,sizeof(int)) < 0)
		error("ERROR on setsockopt");

	
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &buff_size, sizeof(buff_size)) == -1) 
		error("ERROR on setsockopt");
	
	if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &buff_size, sizeof(buff_size)) == -1) 
		error("ERROR on setsockopt");

     	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
		error("ERROR on binding");

#ifdef DEBUG
    	fprintf(stdout, "%s (%d): Opened UDP socket on port %d\n", __FILE__, __LINE__, RECEIVER_UDP_DATA_PORT);
#endif

	// Populate the UDP socket
	sd.udp_sockfd = sockfd;

	return 0;
}

int recv_control()
{	
	DEBUG_MSG("In recv_control");
	
	int n, p_len = sizeof(control_t);
	control_t *p_control = NULL;	
	struct timeval now;
	socklen_t fromlen;
 
	unsigned char *buffer = (unsigned char *) malloc(p_len);
	memset(buffer,0, p_len);

	//non-blocking read
	fromlen = sizeof(sd.udpCtrlSendTo);
	n = recvfrom(sd.tcp_sockfd,buffer,p_len, 0, (struct sockaddr*) &sd.udpCtrlSendTo, &fromlen);
    
	if (n < 0) 
         error("ERROR reading from socket");

	if (n < p_len)
	 DEBUG_ERR("Could not receive proper control packet");

	p_control = (control_t *) buffer;

	/* Populate the path and file name */
	strcpy(sd.dest_dir, p_control->dest_dir);
	strcpy(sd.file_name, p_control->file_name);
	sd.file_size = p_control->file_size;                 // size
    	sd.num_packets = p_control->num_packets;

	/* take RTT measurement */
	gettimeofday(&now, NULL);
	sd.endToEndDelay = ((double) ((now.tv_sec - p_control->tv_sec) * 1000000 + (now.tv_usec - p_control->tv_usec))) * DELAY_MULTIPLIER / ((double) 1000.0)  ;
	
	setParameters(sd.endToEndDelay);

#ifdef DEBUG
    	fprintf(stdout, "%s (%d): Received control packet (%s, %s, %u, %d, %ld) \n", __FILE__, __LINE__, sd.dest_dir, sd.file_name, sd.file_size, sd.num_packets, sd.timeoutVal);
#endif

	/* change connection state */
	sd.conn_state = CONT_RECVD;

	free(buffer);

    return 0;
}

/* send an ack with ack_no */
int send_ack(int ack_no)
{
	DEBUG_MSG("In send_ack");

	int n = 0;
	struct timeval now;

	control_ack_t msg;
	msg.ack_no = ack_no; 

	//Get time stamp measurement
	if(gettimeofday(&now, 0) < 0)
	{
		DEBUG_ERR("gettimeofday returned error!!!");
	}

	msg.tv_sec = now.tv_sec;
	msg.tv_usec = now.tv_usec; 

	n = sendto(sd.tcp_sockfd, (unsigned char *) &msg, sizeof(msg), 0,
				(struct sockaddr*)&sd.udpCtrlSendTo, sizeof (sd.udpCtrlSendTo));
	
	if (n < 0) 
		error("ERROR writing to socket");

#ifdef DEBUG
    	fprintf(stdout, "%s (%d): Sent Control Acknowledgement with ack_no %d\n", __FILE__, __LINE__, ack_no);
#endif

	return 0;	

}

/* receive data and act on it */
void * recv_data(void *arg)
{
	//Receive message

	// amogh: allocate the memory for bytemap
	bytemap = (unsigned char *) malloc(sizeof(unsigned char) * (sd.num_packets));
	memset(bytemap, 0, sizeof(unsigned char) * (sd.num_packets));
	
	while(1)
	{
		DEBUG_MSG("In recv_data");

		packet_t p;
		header_t *hdr = NULL;

	#ifdef DEBUG_3
	    	fprintf(stdout, "%s (%d): Invoking recv_UDP() to get next packet\n", __FILE__, __LINE__);
	#endif
		//Receive the packet
		recv_UDP(&p);	

		hdr = (header_t *) &(p.msg_header);

	#ifdef DEBUG
	    	fprintf(stdout, "%s (%d): Expected Seq no is %d. Received packet with seq no %d\n", __FILE__, __LINE__, sd.expected_seqno, hdr->pkt_id);
	#endif

		//get sequence no
		//1. Duplicate Packet received, already written to file
		// Drop it

		
		if(hdr->pkt_id < sd.expected_seqno)
		{
	#ifdef DEBUG
	    	fprintf(stdout, "%s (%d): Received duplicate packet for already acknowledged. Dropped\n", __FILE__, __LINE__);
	#endif

			/* TODO: something else ? */
	       
			continue;
		}
	
		/* 2. Received sequenced or out-of-order packet, buffer it
			if not already in buffer */

		else if(hdr->pkt_id >= sd.expected_seqno)
		{
			/* check if data already in buffer */
			//if(!ListExistsById(&p_list, hdr->pkt_id))
			if(!isSet(hdr->pkt_id))
			{
	#ifdef DEBUG
	    			fprintf(stdout, "%s (%d): The Packet is not there in buffer. adding it.\n", __FILE__, __LINE__);
	#endif
				//Copy packet contents into buffer
				memcpy((unsigned char *) wbuf.data + hdr->pkt_id * PACKET_SIZE, (unsigned char*) p.pkt_data, hdr->pkt_size);
				wbuf.size++;
				wbuf.nbr_bytes += hdr->pkt_size;
				sd.numPacketBuffer++;
				pthread_mutex_lock(&mtx_nak_sndr);
				//set byte in 
				setByte(hdr->pkt_id);	
				
	
				if(hdr->pkt_id > sd.max_seq_in_buffer)
				{
					sd.max_seq_in_buffer = hdr->pkt_id;
				}
				pthread_mutex_unlock(&mtx_nak_sndr);

				int cnt = sd.expected_seqno;

				while(isSet(cnt) && (cnt <= sd.max_seq_in_buffer))
				{

	#ifdef DEBUG
	    				fprintf(stdout, "%s (%d): The Packet is inorder packet. Writing packet id %d to file.", __FILE__, __LINE__, cnt);
	#endif		
					//update expected_seqno
					pthread_mutex_lock(&mtx_nak_sndr);
					sd.expected_seqno++;

					pthread_mutex_unlock(&mtx_nak_sndr);

				
					/* Check whether it was the last packet */
					if(cnt == (sd.num_packets - 1))
					{
						//Write whatever data is in buffer
						if(wbuf.size)
						{
							//write the packet to file
							write_to_file(sd.fp, (unsigned char *)wbuf.data, wbuf.nbr_bytes);

							//reset buffer
							memset(wbuf.data, 0, sd.num_packets * PACKET_SIZE);
						}

						//Special case, send ack to indicate end of transfer
						nak_packet_t np;
						memset(&np, 0, sizeof(np));
						np.ack_no = sd.expected_seqno;						
						
						pthread_mutex_lock(&mtx_nak_sndr);
						sd.conn_state = END_OF_TRANSFER;
						pthread_mutex_unlock(&mtx_nak_sndr);
	#ifdef DEBUG
	    					fprintf(stdout, "%s (%d): Last packet!!! Setting connection state to End of Transfer (%d) .\n", __FILE__, __LINE__, EOF_RCVD);

	#endif
						
						DEBUG_MSG("Exiting fron end of function recv_data (while loop) !!!");

						while (1)
						{	
							send_nack(&np);
						}
						
						pthread_exit(0);
					}
				
					cnt++;
				
				}

			
			}

		}
	
	} /* End of While */

	free(bytemap);
	
	DEBUG_MSG("Exiting fron end of function recv_data() !!!");
}


/* receive a UDP packet */
int recv_UDP(packet_t *p)
{
	DEBUG_MSG("In recv_UDP");

	int byte_count;
	socklen_t fromlen;
	struct sockaddr_storage addr;
	int p_len = PACKET_SIZE + sizeof(header_t);

	unsigned char buf[p_len];
	//char ipstr[INET6_ADDRSTRLEN];

	//just recvfrom():

	fromlen = sizeof addr;
	byte_count = recvfrom(sd.udp_sockfd, buf, sizeof buf, 0, ( struct sockaddr *) &addr, &fromlen);

	if(byte_count < p_len)
		DEBUG_ERR("Improper UDP packet received");
	
	//keep the sender's address
	if(!sd.sendrAddressUpdated)
	{	
		bcopy((char *) &addr, &sd.udpSendTo, fromlen);
		
#ifdef DEBUG
		char ipstr[MAX_PATH_LENGTH];
		inet_ntop(addr.ss_family,&sd.udpSendTo.sin_addr,ipstr, sizeof ipstr);
		printf("Sender's IP address %s\n", ipstr);			
#endif

		//signal NAK sender thread to start
		pthread_mutex_lock(&mtx_nak_sndr);
		sd.sendrAddressUpdated = 1;

		pthread_cond_signal (&cv_naksndr_start);
		pthread_mutex_unlock(&mtx_nak_sndr);
	}

	// memcpy((char *)p, buf, p_len);
    	packet_deepcopy(p, (packet_t *)buf);
	
	//Fetch the Packet header
	
#ifdef DEBUG
    	fprintf(stdout, "%s (%d): recv_UDP() - Received %d bytes\n", __FILE__, __LINE__, byte_count);
#endif

	/*printf("recv()'d %d bytes of data in buf\n", byte_count);*/
	/*printf("from IP address %s\n",
	inet_ntop(addr.ss_family,
	(addr.ss_family == AF_INET)?((struct sockaddr_in *)&addr)->sin_addr:((struct sockaddr_in6 *)&addr)->sin6_addr,
		ipstr, sizeof ipstr);
	*/

	return 0;


}

FILE* open_file(char* filename) {

  FILE *fp=NULL;
  fp=fopen(filename,"wb+");
  if ( fp == NULL ) {
      fprintf(stderr,"fopen returned error while trying to open a File\n");
      exit(0);
  }

#ifdef DEBUG
    	fprintf(stdout, "%s (%d): Opened file %s for writing\n", __FILE__, __LINE__, filename);
#endif

  return fp;
}

/* write data into file
	1. open file if not already open
	2. Write to file
*/
int write_to_file(FILE *fp, unsigned char *data, long long size)
{
	long long nbytes;


#ifdef DEBUG
    	fprintf(stdout, "%s (%d): Writing %lld bytes to file\n", __FILE__, __LINE__, size);
#endif

   	nbytes=fwrite((unsigned char *) data, 1, size, fp);
   	if ( nbytes < 0 )
		DEBUG_ERR("Fwrite returned an error, while writting to file");


	if ( nbytes < size )
		DEBUG_ERR("Fwrite returned an error, complete data not written to file.");

	return nbytes;
}

/* Resets all connection state
	1. Clear all field in session data
	2. Close any open sockets
	3. Any other?
*/
void resetConnState()
{
	/* TODO: To be implemented */
	
	//close sockets
	close(sd.tcp_listen_sockfd);
	close(sd.tcp_sockfd);
    	close(sd.udp_sockfd);
	close(sd.udp_sockfd_nak);

	//close file 
	fclose(sd.fp);

	memset(&sd, 0, sizeof(sd)); 

	free(wbuf.data);
	free(bytemap);
}

int packet_deepcopy(packet_t *dst, packet_t *src)
{
	if(src == NULL)
		return -1;

	memcpy((unsigned char *)&dst->msg_header, (unsigned char *)&src->msg_header, sizeof(header_t));
	memcpy((unsigned char *) dst->pkt_data, (unsigned char *) src->pkt_data, PACKET_SIZE);	

	return 0;
}

void packet_destroy(packet_t *pkt)
{
	//Free the packet data
	free(pkt);
}


/* 
	Thread takes care of sending NAKs to the sender 
*/
void * NAKSender(void * arg)
{
	DEBUG_MSG("In NAKSender");
	
	//Wait until a packet has been received over UDP
	// to fetch sender's address before begin sending

	pthread_mutex_lock(&mtx_nak_sndr);
	while (sd.sendrAddressUpdated == 0) {
		pthread_cond_wait(&cv_naksndr_start, &mtx_nak_sndr);
	}
	pthread_mutex_unlock(&mtx_nak_sndr);

	//open a UDP socket for sending NAKs
	if(udpClient (&sd) < 0)
		DEBUG_ERR("Some error occurred while opening socket for sending NAKs");

	while(1)
	{
		//Check if the signal to stop has been received
		pthread_mutex_lock(&mtx_nak_sndr);		
		if(IS_SET(sd.conn_state, END_OF_TRANSFER))
		{
			pthread_mutex_unlock(&mtx_nak_sndr);
            		DEBUG_MSG("Calling pthread_exit from function NAKsender !!!");

			pthread_exit(0);
		}
		pthread_mutex_unlock(&mtx_nak_sndr);

		// Sleep for some time
		//usleep(sd.timeoutVal);
		usleep(sd.nak_sender_delay);

        	DEBUG_MSG("Woke Up after Sleep");

		pthread_mutex_lock(&mtx_nak_sndr);

#ifdef DEBUG
		fprintf(stdout, "%s (%d): Next expecteded sequence no is  - %d. Sending NACK for it\n", __FILE__, __LINE__, sd.expected_seqno);
#endif		
		nak_packet_t np;
		memset(&np, 0, sizeof(np));

		np.ack_no = sd.expected_seqno;

		int i = sd.expected_seqno;
		for(; (i<sd.max_seq_in_buffer) && (np.nbr_naks <= NAK_ARRAY_SIZE) ; i++)
		{
			if(!isSet(i))
			{

				np.nak_array[np.nbr_naks] = i; 
				np.nbr_naks++;
			}
		}
		
		if(!np.nbr_naks) { 
			i = sd.expected_seqno;
			for(; (i<sd.num_packets) && (np.nbr_naks <= NAK_ARRAY_SIZE) ; i++)
			{
				np.nak_array[np.nbr_naks] = i; 
				np.nbr_naks++;
			}
		}

		if(np.nbr_naks)
		{
			send_nack(&np);
		}
		pthread_mutex_unlock(&mtx_nak_sndr);

	}

    DEBUG_MSG("Exiting fron end of function NAKsender !!!");

}

int udpClient (session_t *localSession)  { 
		
	int sockfd; 
	const int buff_size = SOCKOPT_RECVFROM_BUFFER ;

#ifdef DEBUG    
	fprintf (stdout, "Entered udpClient: Creating UDP socket\n" );
#endif
	// Creating the socket 
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)  {
		error("Receiver :socket error:");
	}
	
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &buff_size, sizeof(buff_size)) == -1) 
		error("ERROR on setsockopt");
	
	if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &buff_size, sizeof(buff_size)) == -1) 
		error("ERROR on setsockopt");

	// Setting the Struct to Zero , Don't use Memset , it doesn't work 
	memset((char *) &localSession->myAddr, 0, sizeof(localSession->myAddr)); 
	localSession->myAddr.sin_family= AF_INET;
	localSession->myAddr.sin_addr.s_addr=htons(INADDR_ANY);
	localSession->myAddr.sin_port = htons(MY_UDP_NAK_PORT);

	// Now Bind this Addr
    if ( (bind(sockfd, (struct sockaddr *) &localSession->myAddr, sizeof(localSession->myAddr)) < 0) ) { 
    	error("Receiver: bind fail:");  
    }

    localSession->udpSendTo.sin_family=AF_INET;
    localSession->udpSendTo.sin_port=htons(SENDER_UDP_NAK_PORT);
    localSession->udp_sockfd_nak= sockfd;
    
    return sockfd;
}

/* send a nack with ack_no */
int send_nack(nak_packet_t *msg)
{
	DEBUG_MSG("In send_nack");

	int n = 0,i;

	msg->seq_no = sd.seq_no;  

	if (msg->ack_no == (sd.num_packets - 1) )
	{
		for(i=0; i < LASK_ACK_DUP_COUNT; i++)
		{
			n = sendto(sd.udp_sockfd_nak, (void*)msg, sizeof(nak_packet_t),0,(struct sockaddr *) &sd.udpSendTo,
	 			sizeof(sd.udpSendTo));
		
			if (n < 0) 
				error("ERROR writing to UDP socket");

			if(!n)
				DEBUG_ERR("Connection Reset By Peer");

			// usleep(sd.inter_packet_delay);
		}
	}
	//Send it NBR_DUPLICATE_NAKS times
	for(i=0; i < sd.nbr_nak_duplicates; i++)
	{
		n = sendto(sd.udp_sockfd_nak, (void*)msg, sizeof(nak_packet_t),0,(struct sockaddr *) &sd.udpSendTo,
 			sizeof(sd.udpSendTo));
	
		if (n < 0) 
			error("ERROR writing to UDP socket");

		if(!n)
			DEBUG_ERR("Connection Reset By Peer");
	}
	
	

#ifdef DEBUG
	    fprintf(stdout, "%s (%d): Sent  NACk with seq_no %d and content - ", __FILE__, __LINE__, msg->seq_no);
	
	for (i=0; i<msg->nbr_naks; i++)
	{	
		fprintf(stdout, "%d,", msg->nak_array[i]);
	}

	fprintf(stdout, "\n");
#endif
	//Increment the seq_no
	sd.seq_no++;
	//free the memory

	return 0;	

}

/* send a nack with ack_no */
int send_ack_udp(int sno)
{
	DEBUG_MSG("In send_ack_udp");

	int n = 0,i;

	ack_packet_t msg;
	msg.seq_no = sd.seq_no; 
	msg.ack_no = sno; 

	if (sno == (sd.num_packets - 1) )
	{
		for(i=0; i < LASK_ACK_DUP_COUNT; i++)
		{
			n = sendto(sd.udp_sockfd_nak, (void*)&msg, sizeof(msg),0,(struct sockaddr *) &sd.udpSendTo,
	 			sizeof(sd.udpSendTo));
		
			if (n < 0) 
				error("ERROR writing to UDP socket");

			if(!n)
				DEBUG_ERR("Connection Reset By Peer");

			usleep(sd.inter_packet_delay);
		}
	}
	else 
	{

		//Send it NBR_DUPLICATE_NAKS times
		for(i=0; i < sd.nbr_nak_duplicates; i++)
		{
			n = sendto(sd.udp_sockfd_nak, (void*)&msg, sizeof(msg),0,(struct sockaddr *) &sd.udpSendTo,
	 			sizeof(sd.udpSendTo));
		
			if (n < 0) 
				error("ERROR writing to UDP socket");

			if(!n)
				DEBUG_ERR("Connection Reset By Peer");

			usleep(sd.inter_packet_delay);
		}
	}
	//Increment the seq_no
	sd.seq_no++;

#ifdef DEBUG
    	fprintf(stdout, "%s (%d): Sent NACk with seq_no %d and ack_no %d\n", __FILE__, __LINE__, msg.seq_no, msg.ack_no);
#endif

	//free the memory

	return 0;	

}

void * progress_thread(void *arg)
{
	double copied = 0;
	double total = 0;
	while (1) 
	{
		sleep(PROGRESS_THREAD_SECS);	
		pthread_mutex_lock(&mtx_nak_sndr);

		copied = (double)sd.expected_seqno;
		total = (double)sd.num_packets;

		pthread_mutex_unlock(&mtx_nak_sndr);
#ifdef SHOW_PROGRESS
		if (copied > 0) {
			fprintf(stdout, "(%.02f)%% Completed - %.0f out of %.0f transferred ", (copied/total * 100.0f), copied, total);
			fprintf(stdout, "numPacketBuffer : %d\n",sd.numPacketBuffer );
		}

		pthread_mutex_lock(&mtx_nak_sndr);		
		if(IS_SET(sd.conn_state, END_OF_TRANSFER))
		{
			pthread_mutex_unlock(&mtx_nak_sndr);
            		DEBUG_MSG("Calling pthread_exit from progress_thread !!!");

			pthread_exit(0);
		}
		pthread_mutex_unlock(&mtx_nak_sndr);

		//if (copied && (copied == total) )
		//	break;
#endif
	}
	pthread_exit(0);
	return NULL;
}

/* 
	Thread takes care of sending NAKs to the sender 
*/
#ifdef NOT_YET
void *BufferedWriter(void * arg)
{
	DEBUG_MSG("In Buffered Writer");
	
	while(1)
	{

		//Check if the signal to stop has been received
		pthread_mutex_lock(&mtx_nak_sndr);		
		if(IS_SET(sd.conn_state, END_OF_TRANSFER))
		{
			pthread_mutex_unlock(&mtx_nak_sndr);
            		DEBUG_MSG("Calling pthread_exit from function BufferedWriter !!!");

			pthread_exit(0);
		}
		pthread_mutex_unlock(&mtx_nak_sndr);

		//Wait until the buffer is full

		pthread_mutex_lock(&mtxwbuf_wrtr);
		while (wbuf.size != WRITE_BUF_SIZE) {
			pthread_cond_wait(&cvwbuf_wrtr, &mtxwbuf_wrtr);
		}
	
		//Else, write the buffer to file
		write_to_file(sd.fp, wbuf.data, wbuf.size);
		
		//reset the buffer
		memset(wbuf.data, 0, wbuf.size);

		//Signal the 
		pthread_mutex_unlock(&mtx_nak_sndr);
	}
}
#endif


void setParameters()
{

#ifdef DEBUG
	fprintf(stdout, "EndtoEnd Delay is %lf msec. Setting parameters\n", sd.endToEndDelay);
#endif
	
	if(sd.endToEndDelay < 150)
	{
		sd.nbr_nak_duplicates = 1;
		sd.nak_sender_delay = 45000;  	/* in microsec */
		sd.inter_packet_delay = 18;	/* in microsec */
	}
	else
	{
		sd.nbr_nak_duplicates = 1;
		sd.nak_sender_delay = 95000;  	/* in microsec */
		sd.inter_packet_delay = 18;	/* in microsec */
	}

#ifdef DEBUG
	fprintf(stdout, "Parameters set:\n\t# of duplicate NAKs - %d\n\tNAK sender delay - %ld\n\tinter-packet delay - %ld\n", sd.nbr_nak_duplicates, sd.nak_sender_delay, sd.inter_packet_delay);
#endif
	
}

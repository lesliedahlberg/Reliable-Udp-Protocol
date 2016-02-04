
//socket create for server
    struct sockaddr_in si_me, si_other;
     
    int s, i, recv_len;
     
    //create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
     
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //bind socket to port
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        die("bind");
    }



void recieve_packet()
{
    PACKET pack;
    while(1)
    {
        printf("Waiting for data...");
        fflush(stdout);
         
        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(s, pack, sizeof(PACKET), 0, (struct sockaddr *) &si_other, sizeof(si_other))) == -1)
        {
            die("recvfrom()");
        }

        printf("RECIEVE PACKET: placed in server buffer\n");
        strcpy(server_buf.packet[server_buf.seq_2].data, pack.data);
        server_buf.packet[server_buf.seq_2].ack = pack.ack;
        server_buf.packet[server_buf.seq_2].fin = pack.fin;
        server_buf.packet[server_buf.seq_2].syn = pack.syn;
        server_buf.packet[server_buf.seq_2].sum = pack.sum;
        server_buf.packet[server_buf.seq_2].seq = pack.seq;
        server_buf.seq_2 = next_seq(server_buf.seq_2);
         
        print details of the client/peer and the data received
        printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        printf("Data: %s\n" , pack.data);
    }
 
    close(s);
}
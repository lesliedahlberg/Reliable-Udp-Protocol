void send_ack(int seq){
  PACKET ack_packet;

  ack_packet.ack = 1;
  ack_packet.seq = seq;
  unsigned int slen = sizeof(server_socket_client_address);
  //SEND over network
  if(sendto(server_socket, &ack_packet, sizeof(PACKET), 0, (struct sockaddr *) &server_socket_client_address, &slen)==-1)
    {
        //die("sendto()");
        perror("Sendto.\n");
        exit(EXIT_FAILURE);
    }
  printf("SEND ACK: seq: %d;\n", ack_packet.seq);
}
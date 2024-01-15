// AF Server side implementation of UDP server for debugging the HM NN encoder
// just compile with gcc -Wall -g udp_server.c -o udp_server

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

// TODO move these defines to commandline args
// Which UDP port to listen to
#define PORT 8032
// 8192 bytes is enough for context of 64x64, 16bpp
#define MAX_IN_SIZE 8192
// 2048 bytes corresponds to an enhanced predictor of 32x32, 16bpp
#define OUT_SIZE 2048
// The received predictors + context will be dumped here
#define DUMP_FILE_IN "udp_server_in.yuv"
// The enhanced predictors transmitted will be dumped here
#define DUMP_FILE_OUT "udp_server_out.yuv"

int main() { 

    socklen_t addrlen;
    FILE *inFile, *outFile;
    int rcvdBytes, sockfd; 
    char inBuffer[MAX_IN_SIZE], outBuffer[OUT_SIZE];
    struct sockaddr_in servaddr, src_addr; 
    
    // Received data will be dumped here
    inFile = fopen(DUMP_FILE_IN, "wb");
    outFile = fopen(DUMP_FILE_OUT, "wb");
    
    // Creating server socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    }
    
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&src_addr, 0, sizeof(src_addr)); 
        
    // Filling server information 
    servaddr.sin_family    = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORT); 
        
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ) {
        perror("ERROR Could not bind UDP socket on specified port, is another server running ?");
        exit(EXIT_FAILURE);
    } 
    
    printf ("UDP echo server listening at address 0.0.0.0:%d\n", PORT);
    
    // main loop
    while (1) {
      // receiving
      addrlen = sizeof (servaddr.sin_addr.s_addr);
      memset(inBuffer, 0, MAX_IN_SIZE); 
      rcvdBytes = recvfrom(sockfd, inBuffer, MAX_IN_SIZE,
                  MSG_WAITALL,
                  (struct sockaddr *) &src_addr, &addrlen);
      
      // A message with 0 or 1 bytes is a debug request
      if (rcvdBytes <= 1) {
          sprintf(outBuffer, "Echo server listening");
      }
      // Received predictor only, without context (not used anymore)
      else if (rcvdBytes == 2048) {
        memcpy(outBuffer, inBuffer, rcvdBytes);
      }
      // Received predictor with context
      else if (rcvdBytes == 8192) {
        //TODO we crop the bottom-right 32x32 corner of the received 64x64 predictor
        char *inBufferPtr = inBuffer + (32 * 64 *2) /* to account for the first 32 lines to skip */ + (32 *2) /* to skip the first 32 pixles of the 33th line */;
        char *outBufferPtr = outBuffer;
        for (int y=32; y<64; y++) {
          memcpy(outBufferPtr, inBufferPtr, 32 *2);
          inBufferPtr += 64 *2;
          outBufferPtr += 32 *2;
        }
      }
      else {
        perror("ERROR Unhandled input image size");
        exit(EXIT_FAILURE);
      }
      
      // echo'ing back the 1024 bytes predcitor
      sendto(sockfd, outBuffer, OUT_SIZE,
                 MSG_CONFIRM,
                 (struct sockaddr *) &src_addr, addrlen);
      
      printf("Received %d bytes, echoed back %d bytes, client %s:%d\n", rcvdBytes, OUT_SIZE, inet_ntoa(src_addr.sin_addr), src_addr.sin_port);
      
      // Here we dump the pixels to the files
      fwrite(inBuffer, rcvdBytes, 1, inFile);
      fwrite(outBuffer, OUT_SIZE, 1, outFile);
    }
    
    fclose(inFile);
    fclose(outFile);
    
    return 0; 
} 


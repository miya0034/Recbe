#define MAX_LINE_LENGTH 1024
#define MAX_PARAM_LENGTH 20
#define RBCP_VER 0xFF
#define RBCP_CMD_WR 0x80
#define RBCP_CMD_RD 0xC0
#define DEFAULT_IP 192.168.0.16
#define UDP_BUF_SIZE 2048

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#include "rbcp.h"
#include "rbcp_com.c"
#include "myAtoi.c"
#include "myScanf.c"

int rbcp_com(char* ipAddr, unsigned int port, struct rbcp_header* sndHeader, char* sndData, char* readData, char dispMode);

int OnHelp();

int DispatchCommand(char* pszVerb,
		    char* pszArg1,
		    char* pszArg2,
		    char* ipAddr,
		    unsigned int rbcpPort,
		    struct rbcp_header* sndHeader,
		    char dispMode
		    );

int myScanf(char* inBuf, char* argBuf1, char* argBuf2,  char* argBuf3);
int myGetArg(char* inBuf, int i, char* argBuf);

int main(int argc, char* argv[]){

  char* sitcpIpAddr;
  unsigned int sitcpPort;

  struct rbcp_header sndHeader;

  char tempKeyBuf[MAX_LINE_LENGTH];
  char szVerb[MAX_PARAM_LENGTH];
  char szArg1[MAX_PARAM_LENGTH];
  char szArg2[MAX_PARAM_LENGTH];
  int rtnValue;
  char *load;
  load  = "load";

  FILE *fin;

  if(argc !=5){
    puts("\nThis application controls bus of a SiTCP chip for debugging.");
    printf("Usage: %s <IP address> <Port #> <load> <file name>\n\n", argv[0]);
    return -1;
  }else{
    sitcpIpAddr = argv[1];
    sitcpPort   = atoi(argv[2]);
  }

  sndHeader.type=RBCP_VER;
  sndHeader.id=0;

    printf("Command:%s %s\n ",argv[3],argv[4]);
      if((fin = fopen(argv[4],"r"))==NULL){
	printf("ERROR: Cannot open %s\n",argv[4]);
	  return -1;
      }
      while(fgets(tempKeyBuf, MAX_LINE_LENGTH, fin)!=NULL){
	if((rtnValue=myScanf(tempKeyBuf,argv[3], argv[4], argv[2]))<0){
	  printf("ERROR: myScanf(): %i\n",rtnValue);
	  return -1;
	}

	sndHeader.id++;

	if(DispatchCommand(argv[3], argv[4], argv[2], sitcpIpAddr, sitcpPort, &sndHeader,1)<0) exit(EXIT_FAILURE);
      }

      fclose(fin);
  return 0;
}


int DispatchCommand(char* pszVerb,
		    char* pszArg1,
		    char* pszArg2,
		    char* ipAddr,
		    unsigned int rbcpPort,
		    struct rbcp_header* sndHeader,
		    char dispMode
		    ){
  //  char sendData[UDP_BUF_SIZE];
  char recvData[UDP_BUF_SIZE];

  unsigned int tempInt;

  if(strcmp(pszVerb, "wrb") == 0){
    tempInt = myAtoi(pszArg2);    
    pszArg2[0]= (char)(0xFF & tempInt);

    sndHeader->command= RBCP_CMD_WR;
    sndHeader->length=1;
    sndHeader->address=htonl(myAtoi(pszArg1));
    
    return rbcp_com(ipAddr, rbcpPort, sndHeader, pszArg2,recvData,dispMode);
  }
  else if(strcmp(pszVerb, "wrs") == 0){
    tempInt = myAtoi(pszArg2);    
    pszArg2[1]= (char)(0xFF & tempInt);
    pszArg2[0]= (char)((0xFF00 & tempInt)>>8);
 
    sndHeader->command= RBCP_CMD_WR;
    sndHeader->length=2;
    sndHeader->address=htonl(myAtoi(pszArg1));

    return rbcp_com(ipAddr, rbcpPort, sndHeader, pszArg2,recvData,dispMode);
  }
  else if(strcmp(pszVerb, "wrw") == 0){
    tempInt = myAtoi(pszArg2);

    pszArg2[3]= (char)(0xFF & tempInt);
    pszArg2[2]= (char)((0xFF00 & tempInt)>>8);
    pszArg2[1]= (char)((0xFF0000 & tempInt)>>16);
    pszArg2[0]= (char)((0xFF000000 & tempInt)>>24);

    sndHeader->command= RBCP_CMD_WR;
    sndHeader->length=4;
    sndHeader->address=htonl(myAtoi(pszArg1));

    return rbcp_com(ipAddr, rbcpPort, sndHeader, pszArg2,recvData,dispMode);
  }
  else if(strcmp(pszVerb, "rd") == 0){
    sndHeader->command= RBCP_CMD_RD;
    sndHeader->length=myAtoi(pszArg2);
    sndHeader->address=htonl(myAtoi(pszArg1));
    
    return rbcp_com(ipAddr, rbcpPort, sndHeader, pszArg2,recvData,dispMode);
  }
  else if(strcmp(pszVerb, "help") == 0){
    return OnHelp();
  }
  else if(strcmp(pszVerb, "quit") == 0){
    return -1;
  }
  puts("No such command!\n");
  return 0;
  
}


int OnHelp()
{
	puts("\nCommand list:");
	puts("   wrb [address] [byte_data] : Write byte");
	puts("   wrs [address] [short_data]: Write short(16bit)");
	puts("   wrw [address] [word_data] : Write word(32bit)");
	puts("   rd [address] [length]     : Read data");
	puts("   load [file name]          : Execute a script");
	puts("   quit                      : quit from this program\n");

	return 0;
}

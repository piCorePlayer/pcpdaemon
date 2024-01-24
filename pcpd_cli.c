#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define FINDER_PKT "pCP Request"
#define FINDER_ACK "Found"
#define PORT 50435
#define DISCOVER_PACKETS 2

void usage() {
	printf( "Usage: pcp-cli <options>\n");
	printf( "          -t <timeout>  set response timeout. 3s default\n");
	printf( "          -h            help\n");
	printf( "          -p            show playerstabs output\n");
	printf( "          -v            verbose\n\n");
}

int main(int argc, char *argv[]) {
	int sock;
	int yes = 1;
	struct sockaddr_in broadcast_addr;
	struct sockaddr_in server_addr;
	int addr_len;
	char found_ip[1024][17];
	int count;
	int ret;
	fd_set readfd;
	char buffer[1024];
	int i;
	bool verbose = false;
	bool playertabs = false;

	struct timeval* recv_timeout;
	struct timeval* timeout;
	int pcp_devices_found = 0;
	int discover_packets_sent = 0;

	timeout = (struct timeval *) malloc(sizeof(struct timeval));
	timeout->tv_sec = 3;
	timeout->tv_usec = 0;

	int opt;

	while((opt = getopt(argc, argv, ":hpt:v")) != -1) {
		switch(opt) {
			case 'h':
				usage();
				return 0;
				break;
			case 't':
				timeout->tv_sec = atoi(optarg);
				break;
			case 'v':
				verbose = true;
				break;
			case 'p':
				playertabs = true;
				break;
			case ':':
				printf("option needs a value\n");
				return -1;
				break;  
			case '?':
				printf("unknown option: %c\n", optopt); 
				break;
		}
	}

	// optind is for the extra arguments 
	// which are not parsed 
//	for(; optind < argc; optind++){
//		printf("extra arguments: %s\n", argv[optind]);  
//	} 

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("sock error");
		return -1;
	}
	ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&yes, sizeof(yes));
	if (ret == -1) {
		perror("setsockopt error");
		return 0;
	}

	addr_len = sizeof(struct sockaddr_in);

	memset((void*)&broadcast_addr, 0, addr_len);
	broadcast_addr.sin_family = AF_INET;
	broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	broadcast_addr.sin_port = htons(PORT);

	if (! playertabs)
		printf("Hostname                 pCP Version   Kernel                 IP Address\n");
	for (i=0;i<1;i++) {

		FD_ZERO(&readfd);
		FD_SET(sock, &readfd);

		while (1){
			if (discover_packets_sent < DISCOVER_PACKETS)
				ret = sendto(sock, FINDER_PKT, strlen(FINDER_PKT), 0, (struct sockaddr*) &broadcast_addr, addr_len);
			discover_packets_sent++;

			recv_timeout = (struct timeval *) malloc(sizeof(struct timeval));
			recv_timeout->tv_sec = timeout->tv_sec;
			recv_timeout->tv_usec = timeout->tv_usec;

			ret = select(sock + 1, &readfd, NULL, NULL, recv_timeout);
			if (ret>0){
				timeout->tv_sec = recv_timeout->tv_sec;
				timeout->tv_usec = recv_timeout->tv_usec;
			}
			free(recv_timeout);

			if (verbose)
				printf("%lu.%lu\n", timeout->tv_sec, timeout->tv_usec);

			if ((ret > 0) || (discover_packets_sent < DISCOVER_PACKETS)) {
				if (FD_ISSET(sock, &readfd)) {
					memset(buffer, 0, sizeof(buffer));
					count = recvfrom(sock, buffer, 1024, 0, (struct sockaddr*)&server_addr, &addr_len);
					char *tmp_ip = inet_ntoa(server_addr.sin_addr);
					if (strstr(buffer, FINDER_ACK)) {
						bool newdevice = true;
						for (i=0; i<count; i++){
							if (strstr(found_ip[i], tmp_ip)){
								newdevice=false;
							}
						}
						if (newdevice) {
							strcpy(found_ip[pcp_devices_found], tmp_ip);
							// Skip over Found: in string.
							char *pbuffer = buffer + 6;
							char *host = strtok(pbuffer, "&&");
							char *version = strtok(NULL, "&&");
							char *kernel = strtok(NULL, "&&");
							if (playertabs)
								printf("%s,%s,1\n", host, inet_ntoa(server_addr.sin_addr));
							else
								printf("%-24s v%-10s   %-22s %s\n", host, version, kernel, inet_ntoa(server_addr.sin_addr));
							pcp_devices_found++;
						}
					}
				}
			} else {
				if (!playertabs) {
					if (pcp_devices_found == 0)
						printf("No pCP devices found.  Are they running \"pcpd\"?\n");
					else
						printf("\nDone.\nFound %d pCP devices on the network.\n", pcp_devices_found);
				}
				break;
			}
		}
	}
}


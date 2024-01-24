#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/utsname.h>
#include <stdbool.h>

#define FINDER_PKT "pCP Request"
#define PCPVERSION "/usr/local/etc/pcp/pcpversion.cfg"

#define PORT 50435

void removeChar(char *str, char c) {
	char *rS = str;
	char *rD = str;
	for(; *rD != 0; rS++) {
		if(*rS == c && *rS)
			continue;
		*rD = *rS;
		rD++;
		if(!*rS)
		return;
	}
}

void usage() {
	printf( "Usage: pcpd <options>\n");
	printf( "          -v  verbose\n");
	printf( "          -h  help\n\n");
}

int main(int argc, char *argv[]) {
	int sock;
	int yes = 1;
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;
	int addr_len;
	int count;
	int ret;
	fd_set readfd;
	char buffer[1024];
	char *pcpversion;
	char *unknown = "Unknown";
	char ACK[1024];
	struct utsname unamebuf;
	bool verbose = false;
	int opt;

	while((opt = getopt(argc, argv, ":hv")) != -1) {
		switch(opt) {
			case 'h':
				usage();
				return 0;
				break;
			case 'v':
				verbose = true;
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

	if (uname(&unamebuf) < 0) {
		perror("uname");
		exit(EXIT_FAILURE);
	};

	FILE *fd = fopen(PCPVERSION, "r");
	if (fd == NULL){
		perror("pCP version file not found");
		return 1;
	}
	while (fgets(buffer, 1024, fd)){
		if (strstr(buffer, "PCPVERS=")){
			strtok(buffer, "=");
			char *tmp = strtok(NULL, "=");
			strtok(tmp, "piCorePlayer");
			pcpversion = strtok(NULL, "piCorePlayer ");
			removeChar(pcpversion, '"');
			removeChar(pcpversion, '\r');
			removeChar(pcpversion, '\n');
		}
	}
	if (strlen(pcpversion) < 5)
		pcpversion=unknown;

	// snprintf(ACK, 1024, "Found: %-24s v%-10s   %-22s", unamebuf.nodename, pcpversion, unamebuf.release);
	snprintf(ACK, 1024, "Found:%s&&v%s&&%s&&", unamebuf.nodename, pcpversion, unamebuf.release);

	if (verbose)
		printf("%s\n", ACK);
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("Socket Error\n");
		return -1;
	}

	addr_len = sizeof(struct sockaddr_in);

	memset((void*)&server_addr, 0, addr_len);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htons(INADDR_ANY);
	server_addr.sin_port = htons(PORT);

	ret = bind(sock, (struct sockaddr*)&server_addr, addr_len);
	if (ret < 0) {
		perror("Error binding to socket.\n");
		return -1;
	}
	while (true) {
		FD_ZERO(&readfd);
		FD_SET(sock, &readfd);

		ret = select(sock+1, &readfd, NULL, NULL, 0);
		if (ret > 0) {
			if (FD_ISSET(sock, &readfd)) {
				count = recvfrom(sock, buffer, 1024, 0, (struct sockaddr*)&client_addr, &addr_len);
				if (strstr(buffer, FINDER_PKT)) {
					if (verbose)
						printf("\nClient connection information:\n\t IP: %s, Port: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
					memcpy(buffer, ACK, strlen(ACK)+1);
					count = sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr*)&client_addr, addr_len);
				}
			}
		}

	}
}

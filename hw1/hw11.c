// Student ID : 20202076
// Name : Kim Taewoo


// MAC
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 윈도우
// #include <unistd.h>
// #include <sys/types.h>
// #include <winsock2.h> //
// #include <sys/time.h>
// #include <sys/stat.h>
// #include <ctype.h>
// #include <errno.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

#define PROMPT() {printf("\n> ");fflush(stdout);}
#define GETCMD "down"
#define QUITCMD "quit"

int ConnectToServer(char *hostname, char * pnum, char *filename);
int ParseResponse(int sd, char *fname);

int main(int argc, char *argv[]) {
	int socktoserver = -1;
	char buf[BUFSIZ];

	char hostname[BUFSIZ];
	char filename[BUFSIZ];
	char *pnum = "80"; // port number 고정 .. http는 기본80

	char fname[BUFSIZ];

	printf("Student ID : 20202076\n");
	printf("Name : Taewoo Kim\n");

	PROMPT();

	for (;;) {
		// 사용자 입력을 받음.
		if (!fgets(buf, BUFSIZ - 1, stdin)) {
			if (ferror(stdin)) {
				perror("stdin");
				exit(1);
			}
			exit(0);
		}

		char *cmd = strtok(buf, " \t\n\r");
		// 입력된 명령어 구분.

		if((cmd == NULL) || (strcmp(cmd, "") == 0)) {
			PROMPT(); 
			continue;
		} else if(strcasecmp(cmd, QUITCMD) == 0) {
				exit(0);
		}

		if(strcasecmp(cmd, GETCMD) != 0) {
			// 두 문자열이 같지 않으면 wrong commnand..
			printf("Wrong command %s\n", cmd);
			PROMPT(); 
			continue;
		}
		// connect to a server
		// NEED TO IMPLEMENT HERE
		
		char * url = strtok(NULL," \t\n\r");
		// NULL은 이전 strtok함수가 반환한 문자열의 다음 부분부터
		char* host_name = strstr(url,"//")+2;
		char *slash = strchr(host_name, '/');
		char* file_name = slash;
		strcpy(filename,file_name);
		*slash = '\0';
		strcpy(hostname,host_name);

		printf("hostname: %s\n",hostname);
		printf("filename: %s\n",filename);

		socktoserver = ConnectToServer(hostname,pnum,filename);
		// 서버 연결 해주고

		if (socktoserver < 0) {
            printf("Failed to connect to server.\n");
            exit(0);
        }

		char *realname = strrchr(filename, '/');
		if (realname == NULL)
			strcpy(fname, filename);
		else
			strcpy(fname, realname+1);

        // 서버가 보낸 소켓을 파싱한다.
        if (ParseResponse(socktoserver, fname) < 0) {
            printf("Failed to download file from server.\n");
        }
	}
}

// 호스트이름, 포트번호, 파일경로
// http://netapp.cs.kookmin.ac.kr/member/palladio.JPG
// 파일 URL에서 실제 파일 이름을 찾아내서 그 이름으로 저장..
// 서버에 연결이 안되면 에러 메시지 출력
// HTTP Response 메시지의 status 코드가 200이 아닌경우
// 주어진 URL 프로토콜이 http가 아닌 경우에도 에러 메시지 출력

int ConnectToServer(char *hostname, char * pnum, char *filename) {
	int socktoserver;
	struct sockaddr_in server;

	if ((socktoserver = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("socket");
		exit(1);
	}

	struct hostent *hostp;

	if ((hostp = gethostbyname(hostname)) == 0) {
		fprintf(stderr,"%s: unknown host\n", hostname);
		return -1;
	}

	memset((void *) &server, 0, sizeof server); 
	server.sin_family = AF_INET;
	memcpy((void *) &server.sin_addr, hostp->h_addr, hostp->h_length);
	server.sin_port = htons((u_short)atoi(pnum));

	if (connect(socktoserver, (struct sockaddr *)&server, sizeof server) < 0) {
		return -1;
	}

	char msg[BUFSIZ];
	sprintf(msg, "GET %s HTTP/1.0\r\nHost: %s\r\nUser-agent: webcli/1.0\r\nConnection: close\r\n\r\n", filename, hostname);
	if(write(socktoserver, msg, strlen(msg)) < 0) {
		perror("write");
		exit(1);
	}

	return socktoserver;
}


int ParseResponse(int sd, char *fname) { 
	char buf[BUFSIZ];
	FILE *fpSock = fdopen(sd, "r");

	unsigned int numread = 0;
	unsigned int numtoread = 0;

	while(1) {
		if (fgets(buf, BUFSIZ - 1, fpSock) != NULL) {
			if(strcmp(buf, "\r\n") == 0) {
				break;
			} else if(strncmp(buf, "HTTP/", 5) == 0) {
				char *field = strtok(buf, " ");
				char *value = strtok(NULL, " ");
				char *rest = strtok(NULL, "");
				int val = atoi(value);
				if(val != 200) {
					printf("%s %s\n", value, rest);
					fclose(fpSock);
					return -1;
				}
			} else {
				char *field = strtok(buf, ":");
				char *value = strtok(NULL, " \t\n\r");
				if(strcmp(field, "Content-Length") == 0) {
					numtoread = atoi(value); 
					printf("Total Size %d bytes\n", numtoread);
				} 
				printf("%s %s\n", field, value);
			} 
		}
		else {
			printf("input error");
			return 0;
		}
	}  

	// now, open the file write
	FILE *fp = fopen(fname, "w+");
	if(fp == NULL) {
		printf("File Open Fail %s", fname);
	}

	unsigned int step = 1;
	while(1) {
		int nread = fread(buf, sizeof(char), BUFSIZ - 1, fpSock);
		if( nread <= 0)
			break;
		numread += (unsigned int)nread;
		fwrite(buf, sizeof(char), nread, fp);
		unsigned int fill = (unsigned int)numread * 10 / numtoread;
		if(fill >= step) {
			printf("Current Downloading %d/%d (bytes) %.0f%%\n", 
				numread, numtoread, (float)numread / numtoread * 100);
			step = fill + 1;
		}
	}		
	fclose(fpSock);
	fclose(fp);
	printf("Download Complete: %s, %d/%d\n",
		fname, numread, numtoread);
	return 1;
}
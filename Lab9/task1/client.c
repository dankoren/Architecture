#include <sys/socket.h>
#include <netdb.h>
#include "line_parser.c"
#include "common.c"

/**************Global Variables*********************/
client_state client_st;
struct addrinfo hints, *res;
int sockfd;
int debug_flag = 0;

void init_client(){
	client_st.server_addr = "nill";
	client_st.conn_state = IDLE;
	client_st.client_id = NULL;
	client_st.sock_fd = -1;
}

void init_hints(){
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
}

void handle_debug_flag(char* msg){
	if(debug_flag){
		printf("%s|Log %s\n",client_st.server_addr,msg);
	}
}

int receive(int sockfd,char* buf, int len){
	int numbytes;
	if ((numbytes = recv(sockfd, buf, len, 0)) == -1) {
        perror("recv");
        exit(-1);
    }
    buf[numbytes] = '\0';
    handle_debug_flag(buf);
    if(strncmp(buf,"nok",3)==0){
    	fprintf(stderr,"Server Error: %s\n",(buf+4));
    	freeaddrinfo(res); // free the linked-list
		close(client_st.sock_fd);
		init_client();
		return -1;
    }
    return 0;
}

void exec_connect(char* server_ip){
	char buf[MAX_RESP_SIZE];
	if(client_st.conn_state != IDLE){
		fprintf(stderr,"Client is already connected!\n");
		exit(-2);
	}
	init_hints();
	if(getaddrinfo(server_ip, PORT, &hints, &res)!= 0){
		perror("Error in getaddrinfo");
		exit(-1);
	}
	if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
		perror("Error in socket");
		exit(-1);
	}
	if(connect(sockfd, res->ai_addr, res->ai_addrlen) == -1){
		perror("Error in connect");
		exit(-1);
	}
	client_st.conn_state = CONNECTING;
	if(send(sockfd, "hello", 5, 0) == -1){
		perror("Error in send");
		exit(-1);
	}
    if (receive(sockfd,buf,MAX_RESP_SIZE-1)==-1) {
        perror("recv");
        exit(-1);
    }
    if(strncmp(buf,"hello",5)==0){
		client_st.client_id = (buf + 6);
    	client_st.server_addr = server_ip;
    	client_st.conn_state = CONNECTED;
    	client_st.sock_fd = sockfd;
    }
    
}

void exec_bye(){
	if(client_st.conn_state != CONNECTED){
		fprintf(stderr,"Error in bye: client is not connected!\n");
		exit(-2);
	}
	if(send(client_st.sock_fd, "bye", 3, 0) == -1){
		perror("Error in send");
		exit(-1);
	}
	freeaddrinfo(res); // free the linked-list
	close(client_st.sock_fd);
	init_client();
}

void exec_ls(){
	char buf[MAX_RESP_SIZE];
	if(client_st.conn_state != CONNECTED){
		fprintf(stderr,"Error in ls: client is not connected!\n");
		exit(-2);
	}
	if(send(client_st.sock_fd, "ls", 2, 0) == -1){
		perror("Error in send");
		exit(-1);
	}
	if(receive(client_st.sock_fd,buf,2) == -1){
		exit(-1);
	}
	if(strncmp(buf,"ok",2) != 0){
		perror("recv");
		exit(-1);
	}
	memset(buf,0,MAX_RESP_SIZE);
	if(receive(client_st.sock_fd,buf,MAX_RESP_SIZE-1) == -1){
		exit(-1);
	}
	printf("%s\n",buf);
}

/*void exec_get(char* file_name){
	int filesize;
	char buf[MAX_RESP_SIZE];
	if(client_st.conn_state != CONNECTED){
		fprintf(stderr,"Error in ls: client is not connected!\n");
		exit(-2);
	}
	char send_str[strlen(file_name) + 4];
    sprintf(send_str, "get %s", file_name);
    printf("sending %s\n",send_str);
	if(send(client_st.sock_fd, send_str, strlen(file_name) + 4, 0) == -1){
		perror("Error in send");
		exit(-1);
	}
	if(receive(client_st.sock_fd,buf,MAX_RESP_SIZE-1) == -1){
		exit(-1);
	}
	if(strncmp(buf,"ok",2) == 0){
		filesize = atoi(buf+3);
		char file_name_tmp [strlen(file_name) + 4];
		sprintf(file_name_tmp, "%s.tmp", file_name);
		client_st.conn_state = DOWNLOADING;
		FILE* file_tmp = fopen(file_name,"w+");
		int bytes_received = 0;
		while(bytes_received < filesize){
			memset(buf,0,MAX_RESP_SIZE);
			int numbytes;
			if((numbytes = receive(client_st.sock_fd,buf,FILE_BUFF_SIZE)) == -1){
				exit(-1);
			}
			//write(fileno(file_tmp),buf, numbytes);
			int bytes_written = fwrite(buf,numbytes,1,file_tmp);
			bytes_received = bytes_received + bytes_written;
		}
		if(send(client_st.sock_fd, "done", 4, 0) == -1){
			perror("Error in send");
			exit(-1);
		}
		char answer[100];
		int res = recvtimeout(client_st.sock_fd,answer, 100, 10);
		if(res == -1 || strncmp(answer, "nok", 3)==0){
			remove(file_name_tmp);
			fprintf(stderr, "Error while downloading file %s",file_name);
			client_st.client_id = NULL;
			client_st.conn_state = IDLE;
			close(client_st.sock_fd);
			client_st.sock_fd = -1;
			client_st.server_addr = "nil";
		}else{
			client_st.conn_state = CONNECTED;
			rename(file_name_tmp, file_name);
			close(fileno(file_tmp));
		}
	}
}*/

int exec(char* cmd, char** args, int args_len){
	if(strcmp(cmd,"bye") == 0){
		if(args_len == 2 && strncmp(args[1],"-d",2)==0)
			debug_flag = 1;
		exec_bye();
	}
	else if(strcmp(cmd,"conn")==0){
		if(args_len == 3 && strncmp(args[2],"-d",2)==0)
			debug_flag = 1;
		else if(args_len!=2){
			fprintf(stderr,"Invalid number of arguments!\n");
			exit(-1);
		}
		char* server_ip = args[1];
		exec_connect(server_ip);
	}
	else if(strcmp(cmd,"ls")==0){
		if(args_len == 2 && strncmp(args[1],"-d",2)==0)
			debug_flag = 1;
		exec_ls();
	}
	/*else if(strcmp(cmd,"get")==0){
		if(args_len!=2){
			fprintf(stderr,"Invalid number of arguments!\n");
			exit(-1);
		}
		char* file_name = args[1];
		exec_get(file_name);
	}*/
	return 1;
}


int main(int argc, char **argv){
	char input [INPUT_MAX];
	cmd_line* cmdline;
	init_client();
	while(1){
		printf("server:%s> ",client_st.server_addr);
    	fgets(input,INPUT_MAX,stdin);
    	if(strncmp(input,"quit",4)==0){
    		exit(0);
    	}
    	else if(strncmp(input,"\n",1)!= 0){
	    	cmdline = parse_cmd_lines(input);
	    	exec(cmdline->arguments[0],cmdline->arguments,cmdline->arg_count);
    	}
	}
	return 1;
}
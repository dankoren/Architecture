#include <sys/socket.h>
#include <netdb.h>
#include "line_parser.c"
#include "common.c"
#include <sys/sendfile.h>
#define MAX_PENDING_CONNECTIONS 3


/**************Global Variables*********************/
client_state client_st;
struct addrinfo hints, *res;
int sockfd;
int next_client_id = 0;
int is_terminated;

void init_client(){
	char hostname[INPUT_MAX];
	gethostname(hostname,INPUT_MAX);
	client_st.server_addr = hostname;
	client_st.conn_state = IDLE;
	client_st.client_id = NULL;
	client_st.sock_fd = -1;
}

void init_hints(){
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
}


void init_server(){
	memset(&hints, 0, sizeof(hints)); // make sure the struct is empty
	hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
	if(getaddrinfo(NULL, PORT, &hints, &res)!= 0){
		perror("Error in getaddrinfo");
		exit(-1);
	}
	if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
		perror("Error in socket");
		exit(-1);
	}
	if(bind(sockfd, res->ai_addr, res->ai_addrlen) == -1){
		perror("Error in bind");
		exit(-1);
	}
	if(listen(sockfd,MAX_PENDING_CONNECTIONS) == -1){
		perror("Error in listen");
		exit(-1);
	}
	
}

void send_error(char* err_str){
	if(send(client_st.sock_fd, err_str, strlen(err_str), 0) == -1){
		perror("Error in send");
		exit(-1);
	}
	printf("Client %s disconnected\n", client_st.client_id);
	client_st.conn_state = IDLE;
	free(client_st.client_id);
	close(client_st.sock_fd);
	is_terminated = 1;
}

void receive_client(int connection_sockfd){
	client_st.conn_state = CONNECTED;
    client_st.client_id = malloc(20);
    sprintf(client_st.client_id, "%d", next_client_id++);
    client_st.sock_fd = connection_sockfd;
}

void handle_hello(int connection_sockfd){
	if(client_st.conn_state!=IDLE){
		send_error("nok state");
		return;
	}
	receive_client(connection_sockfd);
	char* response = malloc(6 + strlen(client_st.client_id));
	sprintf(response,"hello %s",client_st.client_id);
	if(send(client_st.sock_fd, response, strlen(response), 0) == -1){
		perror("Error in send");
		exit(-1);
	}
	fprintf(stdout,"Client %s is conneted\n", client_st.client_id);
	fflush(stdout);
	free(response);
}

void handle_bye(){
	if(client_st.conn_state!=CONNECTED){
		send_error("nok state");
		return;
	}
	if(send(client_st.sock_fd, "bye", 3, 0) == -1){
		perror("Error in send");
		exit(-1);
	}
	printf("Client %s disconnected\n", client_st.client_id);
	client_st.conn_state = IDLE;
	free(client_st.client_id);
	close(client_st.sock_fd);
	is_terminated = 1;
}

void handle_ls(){
	if(client_st.conn_state!=CONNECTED){
		send_error("nok state");
		return;
	}
	char* listdir = list_dir();
	if(listdir==NULL){
		send_error("nok filesystem");
		return;
	}
	if(send(client_st.sock_fd, "ok", 2, 0) == -1){
		perror("Error in send");
		exit(-1);
	}
	if(send(client_st.sock_fd, listdir, strlen(listdir), 0) == -1){
		perror("Error in send");
		exit(-1);
	}
	//printf("%s\n",listdir);
	free(listdir);
	char wd[150];
	getcwd(wd,150);
	printf("Listed files at %s\n",wd);
}

/*void handle_get(char* file_name){
	if(client_st.conn_state!=CONNECTED){
		send_error("nok state");
		return;
	}
	int filesize = file_size(file_name);
	printf("file size is %d\n",filesize);
	if(filesize==-1){
		send_error("nok file");
		return;
	}
	char response[MAX_RESP_SIZE];
	sprintf(response,"ok %d",filesize);
	if(send(client_st.sock_fd, "ok", 2, 0) == -1){
		perror("Error in send");
		exit(-1);
	}
	printf("sent response %s\n",response);
	client_st.conn_state = DOWNLOADING;

	FILE* file_fd = fopen(file_name, "r+");
	sendfile(client_st.sock_fd, fileno(file_fd), NULL, filesize);
	printf("sending file\n");
	char buf[5];
	recv(client_st.sock_fd, buf, 5, 0);
	if(strcmp(buf, "done")==0){
		send(client_st.sock_fd, "ok", strlen("ok"),0);
		client_st.conn_state = CONNECTED;
		printf("Sent file %s\n", file_name);
		close(fileno(file_fd));
	}else{
		send_error("nok done");
	}

	int filefd = open(file_name,O_RDONLY);
	int bytes_written = 0;
	while(bytes_written < filesize){
		bytes_written += sendfile(client_st.sock_fd,filefd,
			bytes_written,FILE_BUFF_SIZE);

	}
}*/

int main(int argc, char **argv){
	char buf[MAX_INPUT];
	int connection_sockfd,numbytes;
	struct sockaddr client_addr;
	socklen_t addr_size;
	init_server();
	init_client();
	while(1){ //accepting new connections
		addr_size = sizeof (client_addr);
		connection_sockfd = accept(sockfd,&client_addr,&addr_size);
		if (connection_sockfd == -1) {
            perror("accept");
            continue;
        }
        is_terminated=0;
        while(!is_terminated){ // receiving messages from a connection
        	memset(buf, 0, MAX_INPUT); // make sure the struct is empty
        	//printf("connection sockfd: %d\n",connection_sockfd);
			if ((numbytes = recv(connection_sockfd, buf, MAX_INPUT-1, 0)) == -1) {
		        perror("recv");
		        exit(-1);
	    	}
	    	if(strncmp(buf,"hello",5)==0){
	    		handle_hello(connection_sockfd);
	    	}
	    	else if(strncmp(buf,"ls",2)==0){
	    		handle_ls();
	    	}
	    	else if(strncmp(buf,"bye",3)==0){
	    		handle_bye();
	    	}
	    	/*else if(strncmp(buf,"get",3)==0){
	    		printf("%s\n", buf+4);
	    		handle_get(buf+4);
	    	}*/
	    	else{
	    		send_error("nok message");
	    	}
    	}
    	
	}
	return 1;
}
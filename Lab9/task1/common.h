
#define INPUT_MAX 2048
#define MAX_RESP_SIZE 2048
#define PORT "2018"
#define FILE_BUFF_SIZE 1024

typedef enum {
	IDLE,
	CONNECTING,
	CONNECTED,
	DOWNLOADING,
} c_state;

typedef struct {
	char* server_addr;	// Address of the server as given in the [connect] command. "nil" if not connected to any server
	c_state conn_state;	// Current state of the client. Initially set to IDLE
	char* client_id;	// Client identification given by the server. NULL if not connected to a server.
	int sock_fd;		// The file descriptor used in send/recv
} client_state;

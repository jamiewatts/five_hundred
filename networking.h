#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

struct in_addr* hostname_to_ip(char*);
int connect_to(struct in_addr*, int);
char* read_socket_message(FILE*);
void send_socket_message(FILE*, char*);
int open_listen(int);

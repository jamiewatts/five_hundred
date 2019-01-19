#include "networking.h"
#include "game.h"

/*
*   Converts a hostname string to an IP address struct.
*/
struct in_addr* hostname_to_ip(char* hostname)
{
    int error;
    struct addrinfo* addressInfo;

    /* Convert hostname to an address */
    error = getaddrinfo(hostname, NULL, NULL, &addressInfo);
    if (error) {
        return NULL;
    }
    /* Extract the IP address from the address structure and return it */
    return &(((struct sockaddr_in*)(addressInfo->ai_addr))->sin_addr);
}

/*
*   Returns a file descriptor to a socket connected to the ip:port provided.
*/
int connect_to(struct in_addr* ipAddress, int port)
{
    struct sockaddr_in socketAddr;
    int fd;
    /* Create TCP socket */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        fprintf(stderr, "Bad Server.\n");
        exit(2);
    }

    /* Create structure that represents the address (IP address and port
     * number) to connect to
     */
    socketAddr.sin_family = AF_INET;    /* Address family - IPv4 */
    socketAddr.sin_port = htons(port);  /* Port number - network byte order */
    socketAddr.sin_addr.s_addr = ipAddress->s_addr;

    /* Attempt to connect to that remote address */
    if (connect(fd, (struct sockaddr*)&socketAddr, sizeof(socketAddr)) < 0) {
        fprintf(stderr, "Bad Server.\n");
        exit(2);
    }

    return fd;
}

/*
*   Reads all of the data contained in the reading end of the socket and
*   returns it as a string.
*/
char* read_socket_message(FILE* socket) {

    char* buffer = malloc(sizeof(char));
    char c;
    int k = 0;


    while (1) {
        c = (char)fgetc(socket);
        if (c == EOF) {
            return "EOF";
        }
        buffer = realloc(buffer, (sizeof(char) * (k + 1)) + 1);
        buffer[k++] = c;
        if (c == '\n') {
            buffer[k-1] = '\0';
            break;
        }
    }

    fflush(socket);
    return buffer;
}

/*
*   Opens a listening socket on a specified port.
*/
int open_listen(int port) {
    int fd;
    struct sockaddr_in serverAddr;
    int optVal;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
        exit(5);
    }

    optVal = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int)) < 0) {
        exit(5);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (struct sockaddr*)&serverAddr,
            sizeof(struct sockaddr_in)) < 0) {
        fprintf(stderr, "Port Error\n");
        exit(5);
    }
    if(listen(fd, SOMAXCONN) < 0) {
        exit(5);
    }

    return fd;
}

/*
*   Send a message through a specicfied file descriptor.
*/
void send_socket_message(FILE* fd, char* message) {
    fflush(fd);
    fprintf(fd, "%s\n", message);
    fflush(fd);
}

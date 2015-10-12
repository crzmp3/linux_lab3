#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define BUF_SIZE 1024

const char *USAGE = "Usage: ./client <addr> <port> <filepath>";


void fatal(const char *message)
{
    perror(message);
    exit(1);
}


void handle_connection(int socket, const char *filepath)
{
    FILE *output_file = fopen(filepath, "wb");
    if (output_file == NULL)
        fatal("Cannot open output file");

    char buf[BUF_SIZE];

    ssize_t bytes = recv(socket, buf, sizeof(size_t), 0);
    if (bytes <= 0)
        fatal("Cannot get file length");

    size_t length = *((size_t *)&buf);
    fprintf(stderr, "Receiving %lu bytes...\n", length);
    size_t byte_count = 0;

    while (byte_count < length) {
        ssize_t bytes = recv(socket, buf, BUF_SIZE, 0);
        if (bytes == -1)
            fatal("recv error");
        else if (bytes == 0)
            fatal("Not enough data");

        byte_count += bytes;
        if (fwrite(buf, 1, bytes, output_file) != (size_t)bytes)
            fatal("Error saving data");
    }

    fclose(output_file);
    close(socket);
}


int connect_to_server(const char *addr, int port)
{
    int client_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == -1)
        fatal("Could not create socket");

    struct sockaddr_in client_params;
    memset(&client_params, 0, sizeof(client_params));
    client_params.sin_family = AF_INET;
    client_params.sin_addr.s_addr = inet_addr(addr);
    client_params.sin_port = htons(port);

    if (connect(client_socket, (struct sockaddr *)&client_params, sizeof(client_params)) == -1)
        fatal("Failed to connect to server");

    return client_socket;
}


int main(int argc, char *argv[])
{
    if (argc != 4)
        fatal(USAGE);
    const char *addr = argv[1];
    int port = atoi(argv[2]);
    const char *filepath = argv[3];

    int client_socket = connect_to_server(addr, port);

    handle_connection(client_socket, filepath);

    return 0;
}

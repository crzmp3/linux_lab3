#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>


#define MAX_QUEUE_SIZE 10

#define BUF_SIZE 1024

const char *USAGE = "Usage: ./server <port> <filepath>";


struct ThreadTask
{
    int socket;
    const char *filepath;
};


bool send_all(int socket, const char *buffer, size_t length)
{
    size_t sent_count = 0;
    while (sent_count < length)
    {
        int count = send(socket, buffer + sent_count, length - sent_count, 0);
        if (count <= 0) {
            return false;
        }
        sent_count += count;
    }
    return true;
}


void fatal(const char *message)
{
    perror(message);
    exit(1);
}


void handle_connection(int socket, const char *filepath)
{
    FILE *input_file = fopen(filepath, "rb");
    if (input_file == NULL)
        fatal("Cannot open input file");

    struct stat st;
    if (fstat(fileno(input_file), &st) == -1)
        fatal("Cannot get size of file");

    size_t size = st.st_size;
    if (!send_all(socket, (const char *)&size, sizeof(size_t)))
        fatal("Error sending data");
    fprintf(stderr, "Sending %lu bytes...\n", size);

    char buf[BUF_SIZE];
    while (1) {
        size_t read_count = fread(buf, 1, BUF_SIZE, input_file);
        if (read_count <= 0) {
            if (ferror(input_file))
                fatal("Error reading file");
            else if (feof(input_file))
                break;
        }

        if (!send_all(socket, buf, read_count))
            fatal("Error sending data");
    }

    fclose(input_file);
    close(socket);
}


void *thread_worker(void *arg)
{
    struct ThreadTask *task = (struct ThreadTask *)arg;
    handle_connection(task->socket, task->filepath);

    return NULL;
}


void spawn_handler(int socket, const char *filepath)
{
#ifdef THREAD_HANDLER
    struct ThreadTask *task = (struct ThreadTask *)malloc(sizeof(struct ThreadTask));
    task->socket = socket;
    task->filepath = filepath;

    pthread_t thread;
    int code = pthread_create(&thread, NULL, thread_worker, task);
    if (code != 0)
        fatal("Could not create worker thread");
    else
        fprintf(stderr, "Worker thread created\n");
#else
    pid_t pid = fork();
    if (pid == -1) {
        fatal("Could not create worker process");
    } else if (pid == 0) {
        handle_connection(socket, filepath);
        exit(0);
    } else {
        close(socket);
        fprintf(stderr, "Worker process created\n");
        return;
    }
#endif
}


int listen_to_port(int port)
{
    int server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == -1)
        fatal("Could not create socket");

    int enable = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
        fatal("Cannot set reuseaddr");

    struct sockaddr_in server_params;
    memset(&server_params, 0, sizeof(server_params));
    server_params.sin_family = AF_INET;
    server_params.sin_addr.s_addr = htonl(INADDR_ANY);
    server_params.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_params, sizeof(server_params)) == -1)
        fatal("Could not bind socket");

    return server_socket;
}


int main(int argc, char *argv[])
{
    if (argc != 3)
        fatal(USAGE);
    int port = atoi(argv[1]);
    const char *filepath = argv[2];

    int server_socket = listen_to_port(port);

    if (listen(server_socket, MAX_QUEUE_SIZE) == -1)
        fatal("Could not listen on server socket");

    while (1) {
        struct sockaddr_in client_params;
        const size_t client_params_len = sizeof(client_params);
        int client_socket = accept(
            server_socket,
            (struct sockaddr *)&client_params,
            (socklen_t *)&client_params_len
        );
        if (client_socket == -1)
            fatal("Failed to accept client connection");

        fprintf(stderr, "Client %s connected\n", inet_ntoa(client_params.sin_addr));
        spawn_handler(client_socket, filepath);
    }

    return 0;
}

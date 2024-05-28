#include <stdio.h>
#include <unistd.h>
int betriebssystem = 1;
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <ws2def.h>
#pragma comment(lib, "Ws2_32.lib")
#include <windows.h>
#include <io.h>
#include <stdlib.h> 
#include <string.h> 
#include <sys/types.h> 

#define MAX 80
#define PORT 8080
#define SA struct sockaddr

typedef struct {
    char type[500];
    char message[500];
    int status;
} HarrisonRequest;

void start_chat(int connfd) {
    char buff[MAX];
    int n;

    for (;;) {
        memset(&buff, 0, sizeof(buff));

        // read message from client, copy to buffer
        recv(connfd, buff, sizeof(buff), 0);
        printf("From client: %s\t To client : ", buff);
        memset(&buff, 0, sizeof(buff));
        
        n = 0;
        // copy server message in the buffer
        while ((buff[n++] = getchar()) != '\n');

        // send buffer to client
        send(connfd, buff, sizeof(buff), 0);

        if((strncmp("exit", buff, 4)) == 0) {
            printf("exiting server.\n");
            break;
        }
    }
}

char* serialiseRequestObject(HarrisonRequest request) {
    char* format = "{type:'%s'\xACmessage:'%s'\xACstatus:%d}";

    size_t bufferSize = 500;
    char* buffer = (char*)malloc(bufferSize);
    if(buffer == NULL) {
        return NULL; // malloc failed
    }

    int written = sprintf(buffer, format, request.type, request.message, request.status);

    if (written < 0) {
        free(buffer); // formatting failed
        return NULL;
    }

    if ((size_t)written >= bufferSize) {
        // buffer too small, allocate new with required size
        bufferSize = written + 1;
        char* newBuffer = (char*)realloc(buffer, bufferSize);
        if (newBuffer == NULL) {
            free(buffer); // realloc failed
            return NULL;
        }

        buffer = newBuffer;

        written = sprintf(buffer, format, request.type, request.message, request.status);

        if (written < 0) {
            free(buffer); // formatting error
            return NULL;
        }
    }

    return buffer;
}

int eq_ignorecase(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        char c1 = tolower(*s1);
        char c2 = tolower(*s2);

        if (c1 != c2) {
            return c1 - c2;
        }

        s1++;
        s2++;
    }

    return tolower(*s1) - tolower(*s2);
}

HarrisonRequest* deserialiseRequest(char* req) {
    size_t bufferSize = 500;
    int idx = 0;
    char* buffer = (char*)malloc(bufferSize); // stores current character as string
    int obj_open = 0;
    int obj_closed = 0;
    int is_prop = 1;
    char* c_prop = (char*)malloc(bufferSize);
    char* c_word = (char*)malloc(bufferSize);
    HarrisonRequest* request = malloc(bufferSize);

    if (buffer == NULL) {
        free(buffer);
        free(c_prop);
        free(c_word);
        return NULL;
    }

    // initialise with empty value
    c_prop[0] = '\0';
    c_word[0] = '\0';

    for (;idx < bufferSize;idx++) {
        int written = sprintf(buffer, "%c", req[idx]);

        if (written < 0) {
            free(buffer);
            free(c_prop);
            free(c_word);
            return NULL;
        }

        if ((size_t)written >= bufferSize) {
            bufferSize = written+1;
            char* newBuffer = (char*)realloc(buffer, bufferSize);

            if (newBuffer == NULL) {
                free(buffer);
                free(c_prop);
                free(c_word);
                return NULL;
            }

            written = sprintf(buffer, "%c", req[idx]);

            if (written < 0) {
                free(buffer);
                free(c_prop);
                free(c_word);
                return NULL;
            }
        }

        int open = strcmp(buffer, "{");
        int close = strcmp(buffer, "}");
        int val_end = strcmp(buffer, "\xac");
        int prop_end = strcmp(buffer, ":");

        if (open == 0 && !obj_open) {
            obj_open = 1;
            continue;
        } else if (close == 0 && (obj_open && !obj_closed)) {
            obj_closed = 1;

            if (obj_closed && obj_open) {
                if (eq_ignorecase(c_prop, "type") == 0) {
                    strcpy(request->type, c_word);
                } else if(eq_ignorecase(c_prop, "message") == 0) {
                    strcpy(request->message, c_word);
                } else if(eq_ignorecase(c_prop, "status") == 0) {
                    request->status = atoi(c_word);
                }

                // free(buffer);
                // free(c_prop);
                // free(c_word);

                return request;
            }
        }

        if (val_end == 0) {
            if (eq_ignorecase(c_prop, "type") == 0) {
                strcpy(request->type, c_word);
            } else if(eq_ignorecase(c_prop, "message") == 0) {
                strcpy(request->message, c_word);
            } else if(eq_ignorecase(c_prop, "status") == 0) {
                request->status = atoi(c_word);
            }

            memset(c_prop, 0, bufferSize);
            memset(c_word, 0, bufferSize);

            is_prop = 1;

            continue;
        }

        if (prop_end == 0) {
            is_prop = 0;
            continue;
        }

        if (is_prop) {
            written = sprintf(c_prop, "%s%s", c_prop, buffer);
        } else {
            written = sprintf(c_word, "%s%s", c_word, buffer);
        }

        if (written < 0) {
            free(buffer);
            free(c_prop);
            free(c_word);
            return NULL;
        }

        if ((size_t)written >= bufferSize) {
            bufferSize = written+1;
            char* newBuffer;

            if (is_prop) {
                newBuffer = (char*)realloc(c_prop, bufferSize);
            } else {
                newBuffer = (char*)realloc(c_word, bufferSize);
            }

            if (newBuffer == NULL) {
                free(buffer);
                free(c_prop);
                free(c_word);
                return NULL;
            }

            if (is_prop) {
                written = sprintf(c_prop, "%s%s", c_prop, buffer);
            } else {
                written = sprintf(c_word, "%s%s", c_word, buffer);
            }

            if (written < 0) {
                free(buffer);
                free(c_prop);
                free(c_word);
                return NULL;
            }
        }
    }

    free(c_prop);
    free(c_word);
    free(buffer);

    return request;
}

int main() {
    WSADATA wsaData;
    
    HarrisonRequest request = { .type="Announcement", .message="Hey there!", .status=1 };
    char* ser_obj = serialiseRequestObject(request);
    printf("%s\n", ser_obj);
    HarrisonRequest* response = deserialiseRequest(ser_obj);

    printf("type: %s\nmessage: %s\nstatus: %d\n", response->type, response->message, response->status);
    
    free(response);
    free(ser_obj);

    exit(0);

    if (WSAStartup(MAKEWORD(1, 1), &wsaData) == SOCKET_ERROR) {
        perror("WSAStartup");
        printf("error initialising wsa.\n");
        exit(0);
    }

    int sockfd, connfd, len;
    struct sockaddr_in serveraddr, cli;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        printf("socket creation failed. (error code: %d)\n", sockfd);
        exit(0);
    } else {
        printf("socket creation succeeded.\n");
    }

    memset(&serveraddr, 0, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(PORT);

    if ((bind(sockfd, (SA*)&serveraddr, sizeof(serveraddr))) != 0) {
        printf("socket bind failed.\n");
        exit(0);
    } else {
        printf("socket bind succeeded.\n");
    }

    if ((listen(sockfd, 5)) != 0) {
        printf("listen failed.\n");
        exit(0);
    } else {
        printf("listening...\n");
    }

    len = sizeof(cli);

    connfd = accept(sockfd, (SA*)&cli, &len);
    if (connfd < 0) {
        printf("server accept failed.\n");
        exit(0);
    } else {
        printf("server accept succeeded.\n");
    }

    start_chat(connfd);

    close(sockfd);

    return 0;
}

#include "header.h"

void send_all(int s, void *buffer, int size)
{
    int total = 0;
    int n;

    while (total < size)
    {
        n = send(s, (char *)buffer + total, size - total, 0);

        if (n <= 0)
            return;

        total += n;
    }
}

int recv_all(int s, void *buffer, int size)
{
    int total = 0;
    int n;

    while (total < size)
    {
        n = recv(s, (char *)buffer + total, size - total, 0);

        if (n <= 0)
            return 0;

        total += n;
    }

    return 1;
}

void get_time(char *buffer)
{
    time_t t;
    struct tm *tm_info;

    t = time(NULL);
    tm_info = localtime(&t);

    strftime(buffer, 100, "%Y-%m-%d %H:%M:%S", tm_info);
}

void write_log(char *ip, int port, char *activity)
{
    FILE *fp;
    char current_time[100];

    get_time(current_time);

    fp = fopen("server.log", "a");

    if (fp == NULL)
        return;

    fprintf(fp, "[%s] Client %s:%d - %s\n",
            current_time, ip, port, activity);

    fclose(fp);
}

void upload_file(int s, char *ip, int port)
{
    char filename[100];
    long filesize;
    FILE *fp;
    char buffer[BUFFER_SIZE];
    long remaining;
    int chunk;

    /* Receive file name */
    if (!recv_all(s, filename, sizeof(filename)))
        return;

    /* Receive file size */
    if (!recv_all(s, &filesize, sizeof(filesize)))
        return;

    fp = fopen(filename, "wb");

    if (fp == NULL)
    {
        send_all(s, "ERROR", 6);
        return;
    }

    send_all(s, "READY", 6);

    remaining = filesize;

    while (remaining > 0)
    {
        chunk = remaining > BUFFER_SIZE ? BUFFER_SIZE : remaining;

        if (!recv_all(s, buffer, chunk))
        {
            fclose(fp);
            return;
        }

        fwrite(buffer, 1, chunk, fp);

        remaining -= chunk;
    }

    fclose(fp);

    send_all(s, "UPLOAD_SUCCESS", 15);

    printf("File uploaded: %s\n", filename);

    write_log(ip, port, "File uploaded");
}

void download_file(int s, char *ip, int port)
{
    char filename[100];
    FILE *fp;
    char buffer[BUFFER_SIZE];
    long filesize;
    long remaining;
    int chunk;

    if (!recv_all(s, filename, sizeof(filename)))
        return;

    fp = fopen(filename, "rb");

    if (fp == NULL)
    {
        filesize = -1;
        send_all(s, &filesize, sizeof(filesize));
        return;
    }

    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    rewind(fp);

    send_all(s, &filesize, sizeof(filesize));

    remaining = filesize;

    while (remaining > 0)
    {
        chunk = remaining > BUFFER_SIZE ? BUFFER_SIZE : remaining;

        fread(buffer, 1, chunk, fp);

        send_all(s, buffer, chunk);

        remaining -= chunk;
    }

    fclose(fp);

    printf("File downloaded by client: %s\n", filename);

    write_log(ip, port, "File downloaded");
}

void send_date_time(int s, char *ip, int port)
{
    char current_time[100];

    get_time(current_time);

    send_all(s, current_time, sizeof(current_time));

    printf("Date/time requested by client\n");

    write_log(ip, port, "Requested server date and time");
}

void send_system_info(int s, char *ip, int port)
{
    struct utsname info;
    char buffer[1024];

    if (uname(&info) < 0)
    {
        strcpy(buffer, "Unable to get system information");
    }
    else
    {
        sprintf(buffer,
                "System Name : %s\n"
                "Node Name   : %s\n"
                "Release     : %s\n"
                "Version     : %s\n"
                "Machine     : %s\n",
                info.sysname,
                info.nodename,
                info.release,
                info.version,
                info.machine);
    }

    send_all(s, buffer, sizeof(buffer));

    printf("System information requested by client\n");

    write_log(ip, port, "Requested system information");
}

void handle_client(int s, struct sockaddr_in clientAddr)
{
    char username[50];
    char password[50];
    char response[100];
    char choice;
    char client_ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET,
              &clientAddr.sin_addr,
              client_ip,
              sizeof(client_ip));

    int client_port = ntohs(clientAddr.sin_port);

    printf("\nClient connected.\n");
    printf("Client IP   : %s\n", client_ip);
    printf("Client Port : %d\n", client_port);

    /* Receive username */
    if (!recv_all(s, username, sizeof(username)))
        return;

    /* Receive password */
    if (!recv_all(s, password, sizeof(password)))
        return;

    printf("Username received: %s\n", username);

    /* Authentication */
    if (strcmp(username, USERNAME) == 0 &&
        strcmp(password, PASSWORD) == 0)
    {
        strcpy(response, "SUCCESS");
        send_all(s, response, sizeof(response));

        printf("Authentication successful.\n");

        write_log(client_ip,
                  client_port,
                  "User authenticated successfully");
    }
    else
    {
        strcpy(response, "FAILED");
        send_all(s, response, sizeof(response));

        printf("Authentication failed.\n");

        write_log(client_ip,
                  client_port,
                  "Authentication failed");

        return;
    }

    /* Menu processing */
    while (1)
    {
        if (!recv_all(s, &choice, sizeof(choice)))
            break;

        switch (choice)
        {
            case '1':
                upload_file(s, client_ip, client_port);
                break;

            case '2':
                download_file(s, client_ip, client_port);
                break;

            case '3':
                send_date_time(s, client_ip, client_port);
                break;

            case '4':
                send_system_info(s, client_ip, client_port);
                break;

            case '5':
                write_log(client_ip,
                          client_port,
                          "Client terminated session");

                printf("Client terminated session.\n");

                return;

            default:
                strcpy(response, "INVALID");

                send_all(s, response, sizeof(response));

                write_log(client_ip,
                          client_port,
                          "Invalid request received");

                break;
        }
    }
}

int main()
{
    int ls;
    int s;
    int waitSize = 10;

    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;

    socklen_t clientAddrLen;

    memset(&serverAddr, 0, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(SERV_PORT);

    /* Create socket */
    ls = socket(AF_INET, SOCK_STREAM, 0);

    if (ls < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    /* Bind */
    if (bind(ls,
             (struct sockaddr *)&serverAddr,
             sizeof(serverAddr)) < 0)
    {
        perror("Bind failed");
        exit(1);
    }

    /* Listen */
    if (listen(ls, waitSize) < 0)
    {
        perror("Listen failed");
        exit(1);
    }

    printf("=====================================\n");
    printf("     TCP MULTI-SERVICE SERVER\n");
    printf("=====================================\n");
    printf("Server started on port %d\n", SERV_PORT);
    printf("Waiting for clients...\n");

    while (1)
    {
        clientAddrLen = sizeof(clientAddr);

        s = accept(ls,
                   (struct sockaddr *)&clientAddr,
                   &clientAddrLen);

        if (s < 0)
        {
            perror("Accept failed");
            continue;
        }

        handle_client(s, clientAddr);

        close(s);

        printf("Connection closed.\n");
        printf("Waiting for clients...\n");
    }

    close(ls);

    return 0;
}

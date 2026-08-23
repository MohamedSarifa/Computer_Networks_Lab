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

void upload_file(int s)
{
    char filename[100];
    char buffer[BUFFER_SIZE];
    FILE *fp;
    long filesize;
    long remaining;
    int chunk;
    char response[20];

    printf("Enter file name to upload: ");
    scanf("%s", filename);

    fp = fopen(filename, "rb");

    if (fp == NULL)
    {
        printf("File not found.\n");
        return;
    }

    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    rewind(fp);

    send_all(s, filename, sizeof(filename));

    send_all(s, &filesize, sizeof(filesize));

    recv_all(s, response, sizeof(response));

    if (strcmp(response, "READY") != 0)
    {
        printf("Server is not ready.\n");
        fclose(fp);
        return;
    }

    remaining = filesize;

    while (remaining > 0)
    {
        chunk = remaining > BUFFER_SIZE ? BUFFER_SIZE : remaining;

        fread(buffer, 1, chunk, fp);

        send_all(s, buffer, chunk);

        remaining -= chunk;
    }

    fclose(fp);

    recv_all(s, response, sizeof(response));

    printf("Server: %s\n", response);
}

void download_file(int s)
{
    char filename[100];
    char buffer[BUFFER_SIZE];
    FILE *fp;
    long filesize;
    long remaining;
    int chunk;

    printf("Enter file name to download: ");
    scanf("%s", filename);

    send_all(s, filename, sizeof(filename));

    recv_all(s, &filesize, sizeof(filesize));

    if (filesize < 0)
    {
        printf("File does not exist on server.\n");
        return;
    }

    fp = fopen(filename, "wb");

    if (fp == NULL)
    {
        printf("Cannot create file.\n");
        return;
    }

    remaining = filesize;

    while (remaining > 0)
    {
        chunk = remaining > BUFFER_SIZE ? BUFFER_SIZE : remaining;

        if (!recv_all(s, buffer, chunk))
            break;

        fwrite(buffer, 1, chunk, fp);

        remaining -= chunk;
    }

    fclose(fp);

    printf("File downloaded successfully.\n");
}

void get_date_time(int s)
{
    char buffer[100];

    recv_all(s, buffer, sizeof(buffer));

    printf("\nServer Date & Time : %s\n", buffer);
}

void get_system_info(int s)
{
    char buffer[1024];

    recv_all(s, buffer, sizeof(buffer));

    printf("\n===== SERVER SYSTEM INFORMATION =====\n");
    printf("%s", buffer);
}

int main(int argc, char *argv[])
{
    int s;
    int servPort;
    char *servName;

    struct sockaddr_in serverAddr;

    char username[50];
    char password[50];
    char response[100];

    char choice;

    if (argc < 3)
    {
        printf("Usage: %s <server-ip> <port>\n", argv[0]);
        exit(1);
    }

    servName = argv[1];
    servPort = atoi(argv[2]);

    /* Server address */
    memset(&serverAddr, 0, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;

    inet_pton(AF_INET,
              servName,
              &serverAddr.sin_addr);

    serverAddr.sin_port = htons(servPort);

    /* Create socket */
    s = socket(AF_INET, SOCK_STREAM, 0);

    if (s < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    /* Connect */
    if (connect(s,
                (struct sockaddr *)&serverAddr,
                sizeof(serverAddr)) < 0)
    {
        perror("Connection failed");
        exit(1);
    }

    printf("Connected to server.\n");

    /* Authentication */
    printf("\n========== LOGIN ==========\n");

    printf("Username: ");
    scanf("%s", username);

    printf("Password: ");
    scanf("%s", password);

    send_all(s, username, sizeof(username));
    send_all(s, password, sizeof(password));

    recv_all(s, response, sizeof(response));

    if (strcmp(response, "SUCCESS") != 0)
    {
        printf("Authentication failed!\n");
        close(s);
        return 0;
    }

    printf("\nAuthentication successful!\n");

    /* Menu */
    while (1)
    {
        printf("\n=====================================\n");
        printf("          SERVER SERVICES\n");
        printf("=====================================\n");
        printf("1. Upload File\n");
        printf("2. Download File\n");
        printf("3. Server Date and Time\n");
        printf("4. Server System Information\n");
        printf("5. Exit\n");
        printf("=====================================\n");

        printf("Enter your choice: ");
        scanf(" %c", &choice);

        send_all(s, &choice, sizeof(choice));

        switch (choice)
        {
            case '1':
                upload_file(s);
                break;

            case '2':
                download_file(s);
                break;

            case '3':
                get_date_time(s);
                break;

            case '4':
                get_system_info(s);
                break;

            case '5':
                printf("Session terminated.\n");
                close(s);
                return 0;

            default:
                recv_all(s, response, sizeof(response));

                printf("Invalid request.\n");
                break;
        }
    }

    close(s);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main()
{
    int sockfd;

    struct sockaddr_in receiver;
    struct sockaddr_in sender;

    socklen_t len;

    int frame;
    int ack;

    len = sizeof(sender);

    /* Create UDP socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    /* Configure receiver */
    receiver.sin_family = AF_INET;
    receiver.sin_addr.s_addr = INADDR_ANY;
    receiver.sin_port = htons(PORT);

    /* Bind socket */
    if (bind(sockfd,
             (struct sockaddr *)&receiver,
             sizeof(receiver)) < 0)
    {
        perror("Bind failed");
        exit(1);
    }

    printf("====================================\n");
    printf("       SELECTIVE REPEAT RECEIVER    \n");
    printf("====================================\n");
    printf("Waiting for frames...\n\n");

    while (1)
    {
        recvfrom(sockfd,
                 &frame,
                 sizeof(frame),
                 0,
                 (struct sockaddr *)&sender,
                 &len);

        /* End of transmission */
        if (frame == -1)
        {
            printf("\nTransmission completed.\n");
            break;
        }

        printf("Receiver: Frame %d received.\n",
               frame);

        /*
         * Selective Repeat:
         * Every correctly received frame
         * is acknowledged independently.
         */
        printf("Receiver: Frame %d accepted.\n",
               frame);

        ack = frame;

        printf("Receiver: Sending ACK %d.\n\n",
               ack);

        sendto(sockfd,
               &ack,
               sizeof(ack),
               0,
               (struct sockaddr *)&sender,
               len);
    }

    close(sockfd);

    printf("Receiver stopped.\n");

    return 0;
}

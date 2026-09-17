#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main()
{
    int sockfd;
    struct sockaddr_in receiver, sender;
    socklen_t len;

    int frame;
    int ack;
    int expected = 0;

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
    printf("       STOP-AND-WAIT RECEIVER       \n");
    printf("====================================\n");
    printf("Waiting for frames...\n\n");

    while (1)
    {
        /* Receive frame */
        recvfrom(sockfd,
                 &frame,
                 sizeof(frame),
                 0,
                 (struct sockaddr *)&sender,
                 &len);

        /* -1 indicates end of transmission */
        if (frame == -1)
        {
            printf("\nTransmission completed.\n");
            break;
        }

        printf("Receiver: Frame %d received.\n",
               frame);

        /*
         * Check whether received frame
         * is the expected frame.
         */
        if (frame == expected)
        {
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

            expected++;
        }
        else
        {
            /*
             * Duplicate frame.
             * Send previous ACK again.
             */
            ack = expected - 1;

            printf("Receiver: Duplicate Frame %d.\n",
                   frame);

            printf("Receiver: Sending previous ACK %d.\n\n",
                   ack);

            sendto(sockfd,
                   &ack,
                   sizeof(ack),
                   0,
                   (struct sockaddr *)&sender,
                   len);
        }
    }

    close(sockfd);

    printf("Receiver stopped.\n");

    return 0;
}

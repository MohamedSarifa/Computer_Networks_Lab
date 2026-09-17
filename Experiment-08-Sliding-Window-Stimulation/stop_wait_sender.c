#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define PORT 8080
#define MAX 100
#define TIMEOUT 3

/* Circular Queue */
struct Queue
{
    int data[MAX];
    int front;
    int rear;
    int count;
};

/* Initialize queue */
void init(struct Queue *q)
{
    q->front = 0;
    q->rear = -1;
    q->count = 0;
}

/* Insert frame */
void enqueue(struct Queue *q, int frame)
{
    if (q->count == MAX)
    {
        printf("Queue full.\n");
        return;
    }

    q->rear = (q->rear + 1) % MAX;
    q->data[q->rear] = frame;
    q->count++;
}

/* Get front frame */
int peek(struct Queue *q)
{
    return q->data[q->front];
}

/* Remove frame */
void dequeue(struct Queue *q)
{
    if (q->count == 0)
        return;

    q->front = (q->front + 1) % MAX;
    q->count--;
}

int main()
{
    int sockfd;
    struct sockaddr_in receiver;

    struct Queue q;

    int n;
    int frame;
    int ack;

    int transmissions = 0;
    int retransmissions = 0;

    struct timeval tv;

    init(&q);

    printf("Enter number of frames: ");
    scanf("%d", &n);

    /* Insert frames into circular queue */
    for (int i = 0; i < n; i++)
    {
        enqueue(&q, i);
    }

    /* Create UDP socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    /* Configure receiver address */
    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(PORT);
    receiver.sin_addr.s_addr =
        inet_addr("127.0.0.1");

    /* Set ACK timeout */
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;

    setsockopt(sockfd,
               SOL_SOCKET,
               SO_RCVTIMEO,
               &tv,
               sizeof(tv));

    printf("\n====================================\n");
    printf("         STOP-AND-WAIT SENDER       \n");
    printf("====================================\n");

    while (q.count > 0)
    {
        frame = peek(&q);

        printf("\nSender: Sending Frame %d.\n",
               frame);

        sendto(sockfd,
               &frame,
               sizeof(frame),
               0,
               (struct sockaddr *)&receiver,
               sizeof(receiver));

        transmissions++;

        printf("Sender: Waiting for ACK %d...\n",
               frame);

        int result =
            recvfrom(sockfd,
                     &ack,
                     sizeof(ack),
                     0,
                     NULL,
                     NULL);

        if (result < 0)
        {
            /*
             * ACK timeout.
             */
            printf("Sender: Timeout for Frame %d.\n",
                   frame);

            printf("Sender: Retransmitting Frame %d.\n",
                   frame);

            retransmissions++;
        }
        else if (ack == frame)
        {
            printf("Sender: ACK %d received.\n",
                   ack);

            printf("Sender: Frame %d successfully transmitted.\n",
                   frame);

            /* Remove acknowledged frame */
            dequeue(&q);
        }
        else
        {
            printf("Sender: Wrong ACK received.\n");

            printf("Sender: Retransmitting Frame %d.\n",
                   frame);

            retransmissions++;
        }
    }

    /* End transmission */
    frame = -1;

    sendto(sockfd,
           &frame,
           sizeof(frame),
           0,
           (struct sockaddr *)&receiver,
           sizeof(receiver));

    printf("\n------------------------------------\n");
    printf("All frames transmitted successfully.\n");
    printf("Total transmissions : %d\n",
           transmissions);
    printf("Retransmissions     : %d\n",
           retransmissions);
    printf("------------------------------------\n");

    close(sockfd);

    return 0;
}
mohamed_sarifa@LENOVO:~/NWL/EX8$ cat sw_receiver.c
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

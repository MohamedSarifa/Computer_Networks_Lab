#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>

#define PORT 8080

#define MAX_WINDOW 10
#define MAX_FRAMES 100

#define TIMEOUT 2

/* Frame structure */
struct Frame
{
    int seq;
    int ack;
};

/* Circular queue */
struct Queue
{
    struct Frame data[MAX_WINDOW];

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
void enqueue(struct Queue *q,
             struct Frame f)
{
    if (q->count == MAX_WINDOW)
    {
        printf("Window is full.\n");
        return;
    }

    q->rear =
        (q->rear + 1) % MAX_WINDOW;

    q->data[q->rear] = f;

    q->count++;
}

/* Remove frame */
void dequeue(struct Queue *q)
{
    if (q->count == 0)
        return;

    q->front =
        (q->front + 1) % MAX_WINDOW;

    q->count--;
}

/* Display window */
void displayWindow(struct Queue *q)
{
    printf("Current Window: ");

    for (int i = 0;
         i < q->count;
         i++)
    {
        int index;

        index =
            (q->front + i) %
            MAX_WINDOW;

        if (q->data[index].ack == 1)
        {
            printf("[%d-ACK] ",
                   q->data[index].seq);
        }
        else
        {
            printf("[%d] ",
                   q->data[index].seq);
        }
    }

    printf("\n");
}

int main()
{
    int sockfd;

    struct sockaddr_in receiver;

    struct Queue q;

    int n;
    int window;
    int loss;

    int nextFrame = 0;
    int base = 0;

    int frame;
    int ack;

    int transmissions = 0;
    int retransmissions = 0;

    struct timeval tv;

    init(&q);

    srand(time(NULL));

    printf("Enter number of frames: ");
    scanf("%d", &n);

    printf("Enter window size: ");
    scanf("%d", &window);

    printf("Enter frame loss probability (0-100): ");
    scanf("%d", &loss);

    if (n <= 0 || n > MAX_FRAMES)
    {
        printf("Invalid number of frames.\n");
        return 0;
    }

    if (window <= 0 ||
        window > MAX_WINDOW)
    {
        printf("Invalid window size.\n");
        return 0;
    }

    /* Create UDP socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    /* Configure receiver */
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

    /*
     * Create initial sender window.
     */
    while (nextFrame < n &&
           q.count < window)
    {
        struct Frame f;

        f.seq = nextFrame;
        f.ack = 0;

        enqueue(&q, f);

        nextFrame++;
    }

    printf("\n====================================\n");
    printf("        SELECTIVE REPEAT SENDER     \n");
    printf("====================================\n");

    displayWindow(&q);

    while (base < n)
    {
        int currentCount = q.count;

        /*
         * Send all unacknowledged frames
         * in the current window.
         */
        for (int i = 0;
             i < currentCount;
             i++)
        {
            int index;

            index =
                (q.front + i) %
                MAX_WINDOW;

            /*
             * Already acknowledged frame
             * does not need retransmission.
             */
            if (q.data[index].ack == 1)
                continue;

            frame = q.data[index].seq;

            printf("\nSender: Sending Frame %d.\n",
                   frame);

            /*
             * Simulate frame loss.
             */
            if ((rand() % 100) < loss)
            {
                printf("Sender: Frame %d LOST.\n",
                       frame);

                printf("Sender: No ACK for Frame %d.\n",
                       frame);

                printf("Sender: Frame %d will be retransmitted.\n",
                       frame);

                retransmissions++;

                continue;
            }

            /*
             * Send frame through UDP.
             */
            sendto(sockfd,
                   &frame,
                   sizeof(frame),
                   0,
                   (struct sockaddr *)&receiver,
                   sizeof(receiver));

            transmissions++;

            /*
             * Wait for ACK.
             */
            int result =
                recvfrom(sockfd,
                         &ack,
                         sizeof(ack),
                         0,
                         NULL,
                         NULL);

            if (result < 0)
            {
                printf("Sender: Timeout for Frame %d.\n",
                       frame);

                printf("Sender: Frame %d remains unacknowledged.\n",
                       frame);

                retransmissions++;
            }
            else if (ack == frame)
            {
                printf("Sender: ACK %d received.\n",
                       ack);

                /*
                 * Mark only this frame
                 * as acknowledged.
                 */
                q.data[index].ack = 1;

                printf("Sender: Frame %d marked ACKED.\n",
                       frame);
            }
        }

        printf("\n");
        displayWindow(&q);

        /*
         * Slide the window.
         *
         * Only consecutive ACKed frames
         * at the front are removed.
         */
        while (q.count > 0 &&
               q.data[q.front].ack == 1)
        {
            int oldFrame;

            oldFrame =
                q.data[q.front].seq;

            printf("Sender: Frame %d removed from window.\n",
                   oldFrame);

            dequeue(&q);

            base++;

            /*
             * Add a new frame to the
             * circular queue.
             */
            if (nextFrame < n)
            {
                struct Frame f;

                f.seq = nextFrame;
                f.ack = 0;

                enqueue(&q, f);

                printf("Sender: Frame %d added to window.\n",
                       nextFrame);

                nextFrame++;
            }
        }

        printf("\n");
        displayWindow(&q);
    }

    /*
     * Send termination signal.
     */
    frame = -1;

    sendto(sockfd,
           &frame,
           sizeof(frame),
           0,
           (struct sockaddr *)&receiver,
           sizeof(receiver));

    printf("\n====================================\n");
    printf("All frames transmitted successfully.\n");
    printf("Total transmissions : %d\n",
           transmissions);
    printf("Retransmissions     : %d\n",
           retransmissions);
    printf("====================================\n");

    close(sockfd);

    return 0;
}

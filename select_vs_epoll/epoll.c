#include <stdio.h>
#include <string.h>
#include <unistd.h> //for fork, sleep, getpid
#include <stdlib.h> //for exit
#include <errno.h>

#include <sys/time.h>

//socket
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//pthread
#include <pthread.h>

#include <sys/epoll.h>

#define MAXBUF          2048
#define NUM_SENDERS     10

#define handle_error_en(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)


struct thread_info {    /* Used as argument to thread_start() */
    pthread_t   thread_id;        /* ID returned by pthread_create() */
    int         thread_num;       /* Application-defined thread # */
    uint16_t    port;
};


static void* sender(void* arg)
{
    char msg[256];
    struct sockaddr_in servaddr = {0};
    int sockfd, num=1;
    struct thread_info *tinfo = arg;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(tinfo->port);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    while(1) {
        sleep(1+tinfo->thread_num);
        sprintf (msg, "Test message %d from sender %d", num, tinfo->thread_num);
        sendto(sockfd, (const char *)msg, strlen(msg), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
        num++;
    }
}


int main()
{
    int  i, r, max=0;
    ssize_t s;
    int fds[NUM_SENDERS];
    struct sockaddr_in servaddr = {0};
    char buffer[MAXBUF];
    int nfds;
    struct epoll_event events[NUM_SENDERS];
    int epfd = epoll_create(10);


    printf("Select vs Epoll\n");

    //open sockets
    for(i=0; i<NUM_SENDERS; i++)
    {

        fds[i] = socket(AF_INET, SOCK_DGRAM, 0);
        if (fds[i] == -1)
            handle_error("socket");
        memset(&servaddr, 0, sizeof (servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(2000+i);
        servaddr.sin_addr.s_addr = INADDR_ANY;
        r = bind(fds[i],(struct sockaddr*)&servaddr ,sizeof(servaddr));
        if (r == -1)
            handle_error("bind");

        struct epoll_event ev;
        ev.data.fd = fds[i];
        ev.events = EPOLLIN;
        epoll_ctl(epfd,EPOLL_CTL_ADD,ev.data.fd,&ev);
    }


    //start sender threads
    struct thread_info *tinfo = calloc(NUM_SENDERS, sizeof(struct thread_info));
    if (tinfo == NULL)
        handle_error("calloc");

    for(i=0; i<NUM_SENDERS; i++)
    {
        tinfo[i].thread_num = i;
        tinfo[i].port = 2000 + i;
        r = pthread_create(&tinfo[i].thread_id, NULL, &sender, &tinfo[i]);
        if (r != 0)
            handle_error_en(r, "pthread_create");
    }

    sleep(5);
    while(1){

        do {
            printf("round again\n");
            nfds = epoll_wait(epfd, events, NUM_SENDERS, -1);
        } while (errno == EINTR);


        for(i=0;i<NUM_SENDERS;i++) {
            memset(buffer,0,MAXBUF);
            s = recvfrom(fds[i], (char *)buffer, MAXBUF,  MSG_DONTWAIT, NULL, NULL);
            if (s > 0)
                printf("%s\n", buffer);
        }
    }

    //join with sender threads
    for (i = 0; i < NUM_SENDERS; i++) {
        r = pthread_join(tinfo[i].thread_id, NULL);
        if (r != 0)
            handle_error_en(r, "pthread_join");

        printf("Joined with thread %d\n", tinfo[i].thread_num);
    }

    free(tinfo);
    exit(EXIT_SUCCESS);
}

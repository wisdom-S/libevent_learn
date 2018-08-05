#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<event.h>
#include<event2/util.h>
#include<event2/bufferevent.h>
#include<event2/buffer.h>

void cmd_msg_cb(int fd, short events,void* arg )
{
    char msg[4096];
    int n;
    if((n = read(fd,msg,sizeof(msg))) < 0 )
    {
        perror("read fail");
        exit(1);
    }

    struct bufferevent* bev = (struct bufferevent*)arg;
    bufferevent_write(bev,msg,n);
}

void server_msg_cb(bufferevent* bev, void* arg)
{
    char msg[4096];
    int len = bufferevent_read(bev,msg,sizeof(msg));
    if(len <= 0)
    {
        perror("read fail");
        exit(1);
    }

    msg[len] = '\0';

    printf("recv %s from server\n",msg);
}

void event_cb(struct bufferevent* bev, short event, void* arg)
{
    if(event & BEV_EVENT_EOF)
        printf("connection closed\n");
    else if(event & BEV_EVENT_ERROR)
        printf("some other error\n");
    else if(event & BEV_EVENT_CONNECTED)
    {
        printf("the client has connected to server\n");
        return;
    }

    bufferevent_free(bev);

    struct event* ev = (struct event*)arg;
    event_free(ev);
}

int main(int argc, char** argv)
{
    if(argc<3)
    {
        printf("please input server sockaddr\n");
        return -1;
    }
  
    struct event_base* base = event_base_new();

    struct bufferevent* bev = bufferevent_socket_new(base,-1,BEV_OPT_CLOSE_ON_FREE);

    struct event* ev_cmd = event_new(base,STDIN_FILENO, EV_READ | EV_PERSIST,cmd_msg_cb,(void*)bev);

    event_add(ev_cmd,NULL);
   
    struct sockaddr_in server_addr;

    memset(&server_addr,0,sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    if(inet_aton(argv[1],&server_addr.sin_addr) == 0)
    {
        errno = EINVAL;
        return -1;
    }
  
    bufferevent_socket_connect(bev,(struct sockaddr*)&server_addr,sizeof(struct sockaddr));

    bufferevent_setcb(bev,server_msg_cb,NULL,event_cb,(void*)ev_cmd);
    bufferevent_enable(bev,EV_READ | EV_PERSIST);

    event_base_dispatch(base);

    printf("finished\n");
    
    return 0;
}



#include <iostream>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>

void listener_cb(evconnlistener* listener, evutil_socket_t fd,struct sockaddr* sock, int socklen,void* arg);
void socket_read_cb(bufferevent* bev, void* arg);
void event_cb(struct bufferevent* bev, short event, void* arg);

int main() {
    
    struct sockaddr_in sin;
    memset(&sin,0,sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr =  htonl(INADDR_ANY);
    sin.sin_port = htons(9999);

    struct event_base* base = event_base_new();

    evconnlistener* listener = evconnlistener_new_bind(base,listener_cb,base,LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,10,(struct sockaddr*)&sin,sizeof(struct sockaddr_in));

    event_base_dispatch(base);

    evconnlistener_free(listener);
    event_base_free(base);


    return 0;
}

void listener_cb(evconnlistener* listener, evutil_socket_t fd, struct sockaddr* sock, int socklen, void* arg)
{
    printf("accept a client %d\n",fd);

    struct event_base* base = (event_base*)arg;

    bufferevent* bev = bufferevent_socket_new(base,fd,BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev,socket_read_cb,NULL,event_cb,NULL);
    bufferevent_enable(bev,EV_READ | EV_PERSIST);
}

void socket_read_cb(bufferevent* bev, void* arg)
{
  char msg[4096];

  size_t len = bufferevent_read(bev,msg,sizeof(msg));

  msg[len] = '\0';
  printf("recv the client msg: %s\n",msg);

  char reply_msg[4096] = "I have received the msg: ";
  strcat(reply_msg+strlen(reply_msg),msg);

  bufferevent_write(bev,reply_msg,strlen(reply_msg));

  return;
  
}

void event_cb(struct bufferevent* bev, short event, void* arg)
{
    if(event & BEV_EVENT_EOF)
        printf("connection closed\n");
    else if(event & BEV_EVENT_ERROR)
        printf("some other error\n");

    bufferevent_free(bev);
}

#include "server.h"

// git push https://$GITHUB_TOKEN@github.com/marketideas/clientserver
//
S_CLASS::S_CLASS()
{
    server_fd=-1;
    clients.clear();
    listen_port=DEFAULT_LISTEN_PORT; // default listen port per requirements
}
S_CLASS::~S_CLASS()
{
    if (server_fd>=0)
        close(server_fd);
    server_fd=-1;
    remove_clients(true);  //terminate and free all remaining clients
}

std::string S_CLASS::uppercase(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
    [](unsigned char c) {
        return std::toupper(c);
    });
    return(s);
}

void S_CLASS::sendMessageToRoom(std::string roomname, std::string username, std::string msg, bool appendUser)
{
    std::string room=uppercase(roomname);
    std::string croom;

    std::lock_guard<std::recursive_mutex> lk(clientsMutex);
    //printf("message: room: |%s| user: %s msg:|%s|\n",roomname.c_str(),username.c_str(),msg.c_str());

    list_t::iterator iter;
    iter=clients.begin();
    while(iter!=clients.end())
    {
        C_CLASS *client=*iter;
        if (client!=NULL)
        {
            croom=uppercase(client->getRoom());
            if ((croom==room) && (room!=""))
            {
                if (appendUser)
                    client->clientSend(username+": "+msg);
                else
                    client->clientSend(msg);
            }
        }
        iter++;
    }
}

void S_CLASS::setSignal(int signo)  // WARN: called from signal handler not main thread.
{
    if (signo!=SIGHUP) // this one is common when run as a daemon, so just ignore it
    {
        shutdown=true;
    }
}

int32_t S_CLASS::run(int _listen_port)
{
    int32_t result;
    shutdown=false;
    listen_port=_listen_port;
    result=do_listen();
    return(result);
}

void S_CLASS::remove_clients(bool force_all)
{

    // we are only going to 'erase' 1 thread at a time to prevent problems
    // with erasing during an iterator.  Slower, yes, but safter depending
    // on differences between C++ versions, and speed does not matter here.

    std::lock_guard<std::recursive_mutex> lk(clientsMutex);

    list_t::iterator iter,current;
    C_CLASS *client_item;

    if (force_all) // tell them all to terminate
    {
        printf("removing clients: %s\n",force_all?"true":"false");

        for(
            iter=clients.begin();
            iter!=clients.end();
            iter++)
        {
            client_item=*iter;
            if (client_item!=NULL)
            {
                client_item->terminate(true);
            }
        }
    }
    else if (shutdown) // don't need to do this if not force_all and shutdown==true
    {
        return;
    }

    iter=clients.begin();
    while(iter!=clients.end())
    {
        client_item=*iter;
        current=iter;
        iter++;
        if (client_item!=NULL)
        {
            if (client_item->iscomplete())
            {
                delete(client_item);
                clients.erase(current);
                iter=clients.begin(); // restart so iterator is updated correctly
            }
        }
        else // we got a NULL pointer, and shouldn't, but handle anyway
        {
            clients.erase(current);
            iter=clients.begin();
        }
    }
}
int S_CLASS::do_listen()
{
    int result=-1;
    int x;

    int clientfd;
    struct sockaddr_in addr,client;

    int reuseaddr=1;
    int server_fd=socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK,IPPROTO_TCP);

    if (server_fd>=0)
    {
        setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&reuseaddr,sizeof(reuseaddr)); // not going to error check here because not critical

        bzero(&addr,sizeof(addr));
        addr.sin_family=AF_INET;
        addr.sin_addr.s_addr=htonl(INADDR_ANY);
        addr.sin_port=htons(listen_port);
        x=bind(server_fd,(SA*) &addr,sizeof(addr));
        if (x==0)
        {
            printf("chat server running. (listening on port %d)\n",listen_port);
            if (listen(server_fd,SOMAXCONN)==0)
            {
                socklen_t clen=sizeof(client);
                result=0;
                while(!shutdown)
                {
                    clientfd=accept(server_fd,(SA*)&client,&clen);
                    if (clientfd>=0)
                    {
                        std::lock_guard<std::recursive_mutex> lk(clientsMutex);

                        // start the client thread
                        // we will handle the inability to allocate a new client instance ourselves
                        C_CLASS *cthread=new (std::nothrow)  C_CLASS(*this);
                        if (cthread==NULL)
                        {
                            close(clientfd);
                        }
                        else
                        {
                            if (clients.size()<MAX_CONNECTIONS)
                            {
                                clients.push_back(cthread);
                                cthread->accept(clientfd);
                            }
                            else
                            {
                                close(clientfd);
                                delete cthread;
                            }
                        }
                    }
                    else
                    {
                        remove_clients(false);  //false is only remove completed threads
                        usleep(1000*10); // so we don't waste CPU cycles
                    }
                }
            }
        }
        else // bind failed
        {
            fprintf(stderr,"Unable to bind listen socket (need to be root?)\n");
        }
    }

    return(result);
}

extern "C" {

    char portval[MAX_CMD_INPUT+1];
    S_CLASS *baseApp=NULL; // I wouldn't do this (singleton), but because of the signal handling

    void signalHandler(int signo)
    {
        if (baseApp!=NULL)
            baseApp->setSignal(signo);
    }

    int read_cmd_options(int argc, char *argv[])
    {
        int result=0;
        size_t s;
        portval[0]=0;
        for (;;)
        {
            switch(getopt(argc,argv, "T:t:?h"))
            {
            case 't':
            case 'T':
                s=strlen(optarg);
                if ((s>0) && (s<MAX_CMD_INPUT))
                    strcpy(portval,optarg);
                else
                    result=-1;
                break;
            case '?':
            case 'h':
            default:
                printf("Usage: %s <options>\n",argv[0]);
                printf("  -t <portnum> - specify TCP port to listen on\n");
                result=1;
                break;
            case -1:
                break;

            }
            break;
        }
        return(result);
    }

    // use 'int' here so it matches any compiler/environment
    int main(int argc, char *argv[])
    {
        int resultcode=-1;
        baseApp=new S_CLASS();
        if (baseApp!=NULL)
        {
            signal(SIGINT,signalHandler);
            signal(SIGTERM,signalHandler);
            signal(SIGHUP,signalHandler);

            int x=read_cmd_options(argc,argv);


            if (x==1) // they wanted usage
                return 0;
            if (x==0)
            {
                int port=DEFAULT_LISTEN_PORT;
                bool ok=true;
                if (strlen(portval)>0)
                {
                    std::string s(portval);
                    int p;
                    try
                    {
                        x=stoi(s);
                        if (s.length()>5)
                            ok=false;
                        port=x;
                    }
                    catch(...)
                    {
                        ok=false;
                    }
                    if ((port==0) || (port>=65536)) // can't use port 0
                    {
                        ok=false;
                        port=0;
                    }
                }
                if (ok)
                {
                    resultcode=baseApp->run(port);
                    delete baseApp;
                    baseApp=NULL;
                }
                else
                    fprintf(stderr,"Invalid parameter\n");

            }
        }
        return(resultcode);
    }
}
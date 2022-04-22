#include "server.h"



C_CLASS::C_CLASS(S_CLASS &_server) : T_CLASS(), server(_server)
{
    client_fd=-1;
    input_len=0;
    input_buff[0]=0x00;
    lastmessage=0;  //FIXUP: not used, but would be for denial of service rate timing
    username="";
    roomname="";
    complete=false;
    terminated=false;
    thread_started=false;
    thethread=NULL;
}

C_CLASS::~C_CLASS()
{
    if ((roomname!="") && (username!=""))
        server.sendMessageToRoom(roomname,username,username+" has left",false);

    if (client_fd>=0)
    {
        close(client_fd);
        client_fd=-1;
    }
    if (thread_started && (thethread!=NULL))
    {
        join();
    }
}

std::string C_CLASS::getRoom()
{
    return(roomname);
}

void C_CLASS::clientSend(std::string msg)
{
    //printf("clientsend: |%s|\n",msg.c_str());
    if (client_fd>=0)
    {
        int x;
        char lf=0x0A;
        int len=msg.length();
        if (len>0)
        {
            x=write(client_fd,msg.c_str(),msg.length());
            if (x!=len)
                terminate();
            else
                x=write(client_fd,&lf,1);
        }
    }
}


// called only from thread code
void C_CLASS::parse_command( inbuf_t &buffer)
{
    if ((roomname.length()==0) || (username.length()==0))
    {
        std::string tok;
        std::vector<std::string> tokens;
        int ct=0;
        tokens.clear();
        char *context=NULL;
        char *ptr=(char *)&buffer[0];
        ptr=strtok_r(ptr," ",&context);
        while(ptr!=NULL)
        {
            // use thread safe version
            tok=ptr;

            if (ct<10) // just a quick check to make sure they don't send too many tokens for a join
            {
                tokens.push_back(tok);
                ct++;
            }
            ptr=strtok_r(NULL," ",&context);
        }
        bool err=true;
        if (ct==3) // join command MUST have 3 tokens
        {
            //make it case insensitive
            tok=server.uppercase(tokens[0]);

            if (tok=="JOIN")
            {
                if (tokens[1].length()<=20)
                    roomname=server.uppercase(tokens[1]);  // FIXUP: not specified in requirements, but I will make roomname insensitive
                if (tokens[2].length()<=20)
                    username=tokens[2];
                if ((roomname!="") && (username!=""))
                    err=false;
            }
            if (tok=="QQ")  // FIXUP: not in the requirements but want a quick way to quit
                terminate();
        }
        if (err)
        {
            roomname="";
            username="";
            clientSend("ERROR");
            terminate(true); // FIXUP: I don't like this terminate, but the requirements call for this. ("Error situations")
        }
        else
            server.sendMessageToRoom(roomname,username,username+" has joined",false);
    }
    else
        server.sendMessageToRoom(roomname,username,std::string((const char*)&buffer[0]),true);
}

// terminate can be called by any thread so protect with mutex
void C_CLASS::terminate(bool value)
{
    termlock.lock();
    terminated=value;
    termlock.unlock();
}

bool C_CLASS::iscomplete(void)
{
    bool tmp;
    termlock.lock();
    tmp=complete;
    termlock.unlock();
    return(tmp);
}

void C_CLASS::run()  // this is the method run in the thread
{
    uint8_t inbuff[2];
    int x;
    struct timeval to;
    fd_set rfds,wfds,efds;

    complete=false;

    terminate(false);  // allow the thread to run
    //printf("client thread: %d\n",client_fd);
    input_len=0;
    input_buff[0]=0x00;
    while((!terminated) && (client_fd>=0))
    {
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        FD_ZERO(&efds);

        FD_SET(client_fd,&rfds);
        FD_SET(client_fd,&wfds);
        FD_SET(client_fd,&efds);

        to.tv_sec=0;
        to.tv_usec=(100*1000); // 10ms
        x=select(client_fd+1,&rfds,NULL,&efds,&to);

        if (x<0)
        {
            terminate(true);
        }
        else if (x>0) // process the select (if 0, then there was a timeout)
        {
            if (FD_ISSET(client_fd,&efds))
            {
                terminate(true);
            }
            //else if (FD_ISSET(client_fd,&wfds))
            //{
            //    inbuff[0]=0x00; // our 'ping' is the NULL character
            //    x=write(client_fd,inbuff,1);
            // }
            else if (FD_ISSET(client_fd,&rfds))
            {
                x=read(client_fd,inbuff,1);
                if (x>0)
                {
                    char c=inbuff[0] & 0x7F; // if high bit set, reset to ASCII range
                    switch(c)
                    {
                    case 0x00:
                    case 0x0D: // do any character translation needed. (we ignore carriage return)
                        c=0x00;
                        break;
                    default:
                        break;
                    }
                    // ignore any characters if buffer is full
                    // don't insert NULLs or linefeeds (let parse handle those)
                    if ((c!=0x00) && (c!=0x0A) && (input_len<(sizeof(input_buff)-1)))
                    {
                        input_buff[input_len++]=c;
                        input_buff[input_len]=0x00; // keep the buffer zero terminated
                    }
                    if ((c==0x0A) || (input_len>=(MAX_CLIENT_MESSAGE-1)))
                    {
                        parse_command(input_buff);
                        input_len=0;
                        input_buff[0]=0x00;
                        // FIXUP - do some rate limiting here to see if they are sending messages too quickly
                        // denial of service, and close the connection.
                    }
                }
                else
                    terminate(true);
            }
        }
    }
    if (client_fd>=0)
    {
        close(client_fd);
        client_fd=-1;
    }
    complete=true;
}

int C_CLASS::accept(int _clientfd)
{
    int result=-1;
    if ((_clientfd>=0) & (client_fd<0))
    {
        result=0;
        client_fd=_clientfd;
        thread_started=true;
        start();
    }
    return(result);
}

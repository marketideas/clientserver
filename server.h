#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include <mutex>
#include <thread>
#include <list>
#include <vector>
#include <exception>
#include <string>
#include <algorithm>

// things that are not clear from the requirements
// 1) can we accept connections from the same IP address at the same time?
// 2) it is not clear if client can 'JOIN' multiple rooms....it seems likely that with 'JOIN' only working when not in a
//    room, that it is NOT possible without disconnecting, and reconnecting TCP

#undef NULL
#define NULL nullptr

#define S_CLASS chatServer
#define C_CLASS chatClient
#define R_CLASS chatRoom
#define T_CLASS chatThread

#define DEFAULT_LISTEN_PORT (1234)
#define MAX_CLIENT_MESSAGE (20000+1)
#define MAX_CMD_INPUT (64)
#define MAX_CONNECTIONS (2000)

// FIXUP: Windows doesn't have a GetTickCount function, so allow us to create one if ported
//#define GetTickCount GetTickCount

#define SA struct sockaddr

class R_CLASS {
public:
    R_CLASS(void);

};

class T_CLASS
{
public:
    //Starts thread.
    inline void start()
    {
        if (thethread==NULL)
        {
            thethread = new std::thread(T_CLASS::work, this);
        }
    }

    inline void join()
    {
        if (thethread!=NULL)
        {
            thethread->join();
            delete thethread;
            thethread = NULL;
        }
    }

protected:
    virtual void run() = 0;
    std::thread* thethread = NULL;

private:

    static void work(T_CLASS* t)
    {
        if (t!=NULL)
            t->run();
    }
};

class S_CLASS;
class C_CLASS : public T_CLASS
{
    typedef uint8_t inbuf_t[MAX_CLIENT_MESSAGE+1];

private:
    uint64_t lastmessage;  //FIXUP: unused, used for data rate control
    int client_fd;          // socket ID of client
    bool thread_started;
    int input_len;      // how long the current read data from the client
    inbuf_t input_buff; // buffer that holds the incoming raw data from client
    S_CLASS &server;    // the server class that owns us
    bool complete;      // thread is not executing
    bool terminated;   // shared thread variable TRUE to force thread to exit
    std::mutex termlock;    // mutex to protect 'doterminate' variable
    //C_CLASS() { throw [std::exception] };  // FIXUP - create some exception classes, etc
    //C_CLASS() {};
protected:
    std::string username;
    std::string roomname;
    void parse_command( inbuf_t &buff);
    virtual void run();

public:
    C_CLASS(S_CLASS &_server);
    virtual ~C_CLASS();

    virtual int accept(int _clientfd);
    void terminate(bool value=true);
    bool iscomplete(void);
    void clientSend(std::string msg);
    std::string getRoom();

};

class S_CLASS {
private:
    int server_fd;
    bool shutdown;
    uint16_t listen_port;
    std::mutex msgMutex;

protected:
    typedef std::list<C_CLASS *> list_t;
    list_t clients;
    void remove_clients(bool force_all);

public:
    S_CLASS(void);
    virtual ~S_CLASS(void);
    virtual void setSignal(int signo);
    virtual int do_listen();
    virtual int run(int _listen_port);
    std::string uppercase(std::string s);
    void sendMessageToRoom(std::string roomname, std::string username, std::string msg, bool appendUser);

};

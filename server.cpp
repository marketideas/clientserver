#include "server.h"

S_CLASS::S_CLASS()
{
    listen_port=DEFAULT_LISTEN_PORT; // default listen port per requirements
}
S_CLASS::~S_CLASS()
{
    // nothing to clean up here
}

int32_t S_CLASS::run(void)
{
    int32_t result=-1;

    return(result);
}

extern "C" {
    // use 'int' here so it matches any compiler/environment
    int main(int argc, char *argv[])
    {
        int resultcode=-1;

        S_CLASS *baseApp=new S_CLASS();
        if (baseApp!=NULL)
        {
            resultcode=baseApp->run();
        }

        return(resultcode);
    }
}
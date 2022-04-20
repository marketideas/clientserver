#include <stdio.h>
#include <inttypes.h>

#define S_CLASS chatServer
#define C_CLASS chatClient
#define R_CLASS chatRoom

#define DEFAULT_LISTEN_PORT (1234)

class R_CLASS {
public:
    R_CLASS(void);

};

class C_CLASS {
public:
    C_CLASS(void);
};


class S_CLASS {
private:
    uint16_t listen_port;
protected:

public:
    S_CLASS(void);
    virtual ~S_CLASS(void);
    virtual int run(void);
};

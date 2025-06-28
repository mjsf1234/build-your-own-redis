 #define max_msg_size 256

enum{
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_CLOSE = 2
};

struct Conn {
    int fd = -1;
    int state = 0;

    size_t rbuf_size = 0;
    uint8_t rbuf[4 + max_msg_size ];

    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + max_msg_size ];
};
#include <uv.h>
#include <assert.h>

#include "utils.h"

#define N_BACKLOG 64
#define SENDBUF_SIZE 1024
#define PORT 9090

typedef enum { INITIAL_ACK, WAIT_FOR_MSG, IN_MSG } processing_state;

typedef struct {
    processing_state state;
    char sendbuf[SENDBUF_SIZE];
    int32_t sendbuf_end;
    uv_tcp_t* client;
} peer_state_t;

void on_alloc_buffer(uv_handle_t* handle, size_t suggested_size,
                      uv_buf_t* buf)
{
    buf->base = (char*)xmalloc(suggested_size);
    buf->len = suggested_size;
}

void on_client_closed(uv_handle_t* handle) {
    uv_tcp_t* client = (uv_tcp_t*) handle;

    if (client->data) {
        free(client->data);
    }

    free(client);
}

void on_wrote_buf(uv_write_t* req, int32_t status)
{
    peer_state_t* peerstate;

    if (status) {
        fprintf(stderr, "Write error: %s\n", uv_strerror(status));
        exit(2);
    }

    peerstate = (peer_state_t*)req->data;

    if (peerstate->sendbuf_end >= 3 &&
        peerstate->sendbuf[peerstate->sendbuf_end - 3] == 'X' &&
        peerstate->sendbuf[peerstate->sendbuf_end - 2] == 'Y' &&
        peerstate->sendbuf[peerstate->sendbuf_end - 1] == 'Z') {
        free(peerstate);
        free(req);
        uv_stop(uv_default_loop());
        return;
    }

    peerstate->sendbuf_end = 0;
    free(req);
}

void on_peer_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf)
{
    int32_t rc;
    peer_state_t* peerstate;
    
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "Read error: %s\n", uv_strerror(nread));
        }

        uv_close((uv_handle_t*)client, on_client_closed);
    }
    else if (nread == 0) {
    }
    else {
        assert(buf->len >= nread);

        peerstate = (peer_state_t*)client->data;

        if (peerstate->state == INITIAL_ACK) {
            free(buf->base);
            return;
        }

        for (int32_t i = 0; i < nread; ++i) {
            switch (peerstate->state) {
            case INITIAL_ACK:
                assert(0 && "can't reach here");
                break;

            case WAIT_FOR_MSG:
                if (buf->base[i] == '^') {
                    peerstate->state = IN_MSG;
                }
                break;

            case IN_MSG:
                if (buf->base[i] == '$') {
                    peerstate->state = WAIT_FOR_MSG;
                } else {
                    assert(peerstate->sendbuf_end < SENDBUF_SIZE);
                    peerstate->sendbuf[peerstate->sendbuf_end++] = 
                                                               buf->base[i] + 1;
                }
                break;
            }
        }
        
        if (peerstate->sendbuf_end > 0) {
            uv_buf_t writebuf = uv_buf_init(peerstate->sendbuf, 
                                            peerstate->sendbuf_end);

            uv_write_t* writereq = (uv_write_t*) xmalloc(sizeof (*writereq));
            writereq->data = peerstate;
            
            if ((rc = uv_write(writereq, (uv_stream_t*) client, &writebuf, 1,
                               on_wrote_buf)) == -1) {
                fprintf(stderr, "uv_write failed: %s", uv_strerror(rc));
                exit(2);
            }
        }
    }
    
    free(buf->base);
}

void on_wrote_init_ack(uv_write_t* req, int32_t status)
{
    int32_t rc;
    peer_state_t* peerstate;

    if (status) {
        fprintf(stderr, "Write error: %s\n", uv_strerror(status));
        exit(2);
    }

    peerstate = (peer_state_t*)req->data;
    peerstate->state = WAIT_FOR_MSG;
    peerstate->sendbuf_end = 0;

    if ((rc = uv_read_start((uv_stream_t*)peerstate->client, on_alloc_buffer, 
                            on_peer_read)) == -1) {
        fprintf(stderr, "uv_read_start failed: %s", uv_strerror(rc));
        exit(2);
    }

    free(req);
}

void on_peer_connected(uv_stream_t* server_stream, int32_t status)
{
    int32_t rc, namelen;
    uv_tcp_t* client;
    struct sockaddr_storage peername;
    peer_state_t* peerstate;
    uv_write_t* req;

    if (status == -1) {
        fprintf(stderr, "Peer connection error: %s\n", uv_strerror(status));
        return;
    }

    client = (uv_tcp_t*) xmalloc(sizeof (*client));

    if ((rc = uv_tcp_init(uv_default_loop(), client)) == -1) {
        fprintf(stderr, "uv_tcp_init failed: %s", uv_strerror(rc));
        exit(2);
    }

    client->data = NULL;

    if (uv_accept(server_stream, (uv_stream_t *) client) == 0) {
        namelen = sizeof peername;
        if ((rc = uv_tcp_getpeername(client, (struct sockaddr *) &peername, 
                                     &namelen)) == -1) {
            fprintf(stderr, "uv_tcp_getpeername failed: %s", uv_strerror(rc));
            exit(2);
        }

        report_peer_connected((const struct sockaddr_in*)&peername, namelen);

        peerstate = (peer_state_t *) xmalloc(sizeof (*peerstate));
        peerstate->state = INITIAL_ACK;
        peerstate->sendbuf[0] = '*';
        peerstate->sendbuf_end = 1;
        peerstate->client = client;
        client->data = peerstate;

        uv_buf_t writebuf = uv_buf_init(peerstate->sendbuf, 
                                        peerstate->sendbuf_end);

        req = (uv_write_t *) xmalloc(sizeof (*req));
        req->data = peerstate;

        if ((rc = uv_write(req, (uv_stream_t *)client, &writebuf, 1, 
                           on_wrote_init_ack)) == -1) {
            fprintf(stderr, "uv_write failed: %s", uv_strerror(rc));
            exit(2);
        }
    }
    else {
        uv_close((uv_handle_t*)client, on_client_closed);
    }
}

int main(int32_t argc, char* argv[])
{
    int32_t rc; 
    uv_tcp_t server_stream;
    struct sockaddr_in server_address;

    if ((rc = uv_tcp_init(uv_default_loop(), &server_stream)) == -1) {
        fprintf(stderr, "uv_tcp_init failed: %s", uv_strerror(rc));
        exit(1);
    }

    if ((rc = uv_ip4_addr("0.0.0.0", PORT, &server_address)) == -1) {
        fprintf(stderr, "uv_ip4_addr failed: %s", uv_strerror(rc));
        exit(1);
    }

    if ((rc = uv_tcp_bind(&server_stream, 
                          (const struct sockaddr*)&server_address, 0)) == -1) {
        fprintf(stderr, "uv_tcp_bind failed: %s", uv_strerror(rc));
        exit(1);
    }

    if ((rc = uv_listen((uv_stream_t*)&server_stream, N_BACKLOG, 
                         on_peer_connected)) == -1) {
        fprintf(stderr, "uv_listen failed: %s", uv_strerror(rc));
        exit(1);
    }

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    return uv_loop_close(uv_default_loop());
}

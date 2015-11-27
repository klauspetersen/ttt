#include <iostream>
#include <glib.h>
#include <unistd.h>
#include <thread>
#include <libsigrok-internal.h>
#include <tmmintrin.h>
#include "ProducerConsumerQueue.h"

#define LOG_PREFIX "main"
#define MAX_DEVICES 3

using namespace std;

extern SR_PRIV struct sr_dev_driver saleae_logic16_driver_info;

volatile int throughput;
struct sr_dev_inst *sdiArr[MAX_DEVICES];

struct sr_context *sr_ctx = NULL;

char *byte2bin(uint8_t value, char *buf){
    uint8_t i;

    buf[8] = '\0';
    for (i = 0; i < 8; i++){
        if(value & 1){
            buf[7 - i] = '1';
        } else {
            buf[7 - i] = '0';
        }
        value = value >> 1;
    }
    return buf;
}

__m128i sse_trans_slice(__m128i x)
{
    union { unsigned short s[8]; __m128i m; } u;
    for (int i = 0; i < 8; ++i) {
        u.s[7-i]=_mm_movemask_epi8(x);
        x = _mm_slli_epi64(x,1);
    }
    return  u.m;
}

static void consumer_recv(){
    while(1){
        throughput = 0;
        sleep(1);
        cout << "Throughput:" << throughput << endl;
    }
}
static void sr_data_recv_cb(sr_packet_t *packet){
    char buf[9];

    for(int i=0; i< packet->size; i++){
        //printf("%s\n", byte2bin(((uint8_t*)packet->data)[i], buf));
        if(((uint8_t*)packet->data)[i] != 0) {
            printf("%d\n", ((uint8_t *) packet->data)[i]);
        }
    }
}

uint8_t arrVal[] = {
        0x00,
        0x01,
        0x02,
        0x03,
        0x04,
        0x05,
        0x06,
        0x07,
        0x08,
        0x09,
        0x0A,
        0x0B,
        0x0C,
        0x0D,
        0x0E,
        0x0F
};


uint8_t arrMask[] = {
        0x01,
        0x01,
        0x80,
        0x80,
        0x80,
        0x80,
        0x80,
        0x80,
        0x80,
        0x80,
        0x80,
        0x80,
        0x80,
        0x80,
        0x80,
        0x80
};
#if 0
static void data_convert(uint8_t *src, uint8_t *dst, uint32_t length){
    assert(length%128==0);

    __m128i x = _mm_loadu_si128((__m128i *)src);

    __m128i y = sse_trans_slice(x);


}
#endif

typedef union{
    __m128i x;
    uint8_t y[16];
} _mmi_mask;

int main()
{
    struct sr_dev_inst *sdi;
    struct sr_dev_driver *driver;
    struct sr_session *session;
    GMainLoop *main_loop;

    __m128i x = _mm_loadu_si128((__m128i *)arrVal);

    __m128i maskv = _mm_load_si128((__m128i *)arrMask);

    x = _mm_shuffle_epi8(x, maskv);

    __m128i y = sse_trans_slice(x);


    //std::thread consumer_thread(consumer_recv);

    cout << "trace_fast!" << endl;
    
    driver = &saleae_logic16_driver_info;

    if (sr_init(&sr_ctx) != SR_OK) {
        g_critical("Failed to initialize context.");
        return 0;
    }
    if(sr_session_new(sr_ctx, &session) != SR_OK){
        g_critical("Failed to initialize session.");
        return 0;
    }

    if (sr_driver_init(sr_ctx, driver) != SR_OK) {
        g_critical("Failed to initialize driver.");
        return 0;
    }

    GSList *devices = sr_driver_scan(driver, NULL);

    int i=0;
    for (GSList *l = devices; l; l = l->next) {
        sdiArr[i] = (struct sr_dev_inst *)l->data;
        sdiArr[i]->id = i;
        sdiArr[i]->cb = sr_data_recv_cb;
        if(sr_session_dev_add(session, sdiArr[i]) != SR_OK) {
            g_critical("Failed to add device to session.");
            return 0;
        }

        if(sr_dev_open(sdiArr[i]) != SR_OK){
            g_critical("Failed to open device");
            return 0;
        }
        i++;
    }

    main_loop = g_main_loop_new(NULL, FALSE);

    if(sr_session_start(session) != SR_OK){
        g_critical("Failed to start session.");
        return 0;
    }

    g_main_loop_run(main_loop);

    return 0;
}

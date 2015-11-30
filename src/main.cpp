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

__m128i sse_data_transpose(__m128i x){
    union { uint16_t s[8]; __m128i m; } u;
    for (int i = 0; i < 8; ++i) {
        u.s[7-i]=(uint16_t)(_mm_movemask_epi8(x));
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
    //char buf[9];

    for(int i=0; i< packet->size; i++){
        //printf("%s\n", byte2bin(((uint8_t*)packet->data)[i], buf));
        if(((uint8_t*)packet->data)[i] != 0) {
            printf("%d\n", ((uint8_t *) packet->data)[i]);
        }
    }
}


static void data_convert_copy(uint8_t *src, uint8_t *dst, uint32_t length){
    //static const __m128i m1 = _mm_set_epi8(0x0,0x2,0x4,0x6,0x8,0xA,0xC,0xE,0x1,0x3,0x5,0x7,0x9,0xB,0xD,0xF);
    static const __m128i m2 = _mm_set_epi8(0xF,0xD,0xB,0x9,0x7,0x5,0x3,0x1,0xE,0xC,0xA,0x8,0x6,0x4,0x2,0x0);
    __m128i x, y;
    assert(length%16==0);
    assert(src != NULL);
    assert(dst != NULL);

    while(length > 0) {
        /* Load unaligned */
        x = _mm_loadu_si128((__m128i *) src);
        /* Transpose */
        y = sse_data_transpose(x);
        /* Deinterlive */
        y = _mm_shuffle_epi8(y, m2);
        /* Store in dst */
        _mm_storeu_si128((__m128i *) dst, y);
        /* Inc by 128 bit */
        dst += 16;
        src += 16;
        length -= 16;
    }
}

int main()
{
    struct sr_dev_driver *driver;
    struct sr_session *session;
    GMainLoop *main_loop;

    thread t1(consumer_recv);

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

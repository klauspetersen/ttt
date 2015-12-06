#include <iostream>
#include <glib.h>
#include <unistd.h>
#include <thread>
#include "sigrok_wrapper.h"
#include <tmmintrin.h>
#include <assert.h>

#define LOG_PREFIX "main"

using namespace std;

volatile int throughput;

static void consumer_recv(){
    while(1){
        throughput = 0;
        sleep(1);
        cout << "Throughput:" << throughput << endl;
    }
}

void loop(){
    struct sr_context *ctx;
    sigrok_init(&ctx);






}

int main()
{
    thread t0(loop);
    thread t1(consumer_recv);

    for(;;);
}




#if 0
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
#endif
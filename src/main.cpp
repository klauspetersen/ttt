#include <iostream>
#include <glib.h>
#include <unistd.h>
#include <thread>
#include "libsigrok.h"
#include "libsigrok-internal.h"
#include "ProducerConsumerQueue.h"

#define LOG_PREFIX "main"
#define MAX_DEVICES 3

using namespace std;

extern SR_PRIV struct sr_dev_driver saleae_logic16_driver_info;

volatile int throughput;
struct sr_dev_inst *sdiArr[MAX_DEVICES];

//folly::ProducerConsumerQueue<folly::fbstring> queue;
struct sr_context *sr_ctx = NULL;
static void consumer_recv(){
    while(1){
        throughput = 0;
        sleep(1);
        cout << "Throughput:" << throughput << endl;
    }
}
#if 0
void datafeed_in(const struct sr_dev_inst *sdi, const struct sr_datafeed_packet *packet, void *cb_data){
    /* If the first packet to come in isn't a header, don't even try. */
    struct sr_datafeed_logic *logic;
    uint8_t data;

    (void)cb_data;
    (void)sdi;

    if (packet->type != SR_DF_HEADER){
        logic = (struct sr_datafeed_logic *)packet->payload;
   
        cout << "Data" << endl;
        for(int i=0; i<logic->length; i++){
            data = ((uint8_t *)(logic->data))[i];
            if(data != 0){
                //cout << "value is " << unsigned(data) << endl;
            }
        }
    } 
}
#endif


int main()
{
    struct sr_dev_inst *sdi;
    struct sr_dev_driver *driver;
    struct sr_session *session;
    GMainLoop *main_loop;

    std::thread consumer_thread(consumer_recv);  

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
        //sdiArr[i]->callback = NULL;
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

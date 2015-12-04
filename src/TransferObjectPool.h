//
// Created by kape on 12/3/15.
//

#ifndef TTT_TRANSFEROBJECTPOOL_H
#define TTT_TRANSFEROBJECTPOOL_H

#include <list>
#include <vector>
#include <mutex>
#include "sigrok_wrapper.h"


class TransferObjectPool {
public:
    TransferObjectPool(uint32_t cnt, uint32_t sizeOfPacket);
    void free(sr_warp_transfer_t *ptr);
    sr_warp_transfer_t* alloc();
    unsigned long size();
private:
    std::mutex mtx;
    std::list<sr_warp_transfer_t *> freeObjects;
    std::vector<sr_warp_transfer_t> objects;
};


#endif //TTT_TRANSFEROBJECTPOOL_H

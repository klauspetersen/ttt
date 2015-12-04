//
// Created by kape on 12/3/15.
//

#include "TransferObjectPool.h"

using namespace std;

TransferObjectPool::TransferObjectPool(uint32_t cnt, uint32_t sizeOfPacket) {
    /* Preallocate */
    for(int i = 0; i < cnt; i++){
        objects.push_back(sr_warp_transfer_t());
    }

    /* Initialize transfer objects */
    for(int i = 0; i < cnt; i++){
        objects.at(i).transfer = libusb_alloc_transfer(0);
        objects.at(i).packet.data = new uint8_t[sizeOfPacket];
        objects.at(i).packet.size = sizeOfPacket;
        objects.at(i).packet.ref = &objects.at(i);
        free(&objects.at(i));
    }
}

void TransferObjectPool::free(sr_warp_transfer_t *ptr) {
    /* Aquire lock */
    lock_guard<mutex> lock(mtx);
    /* Add ptr to free list */
    freeObjects.push_front(ptr);
}

sr_warp_transfer_t* TransferObjectPool::alloc() {
    /* Aquire lock */
    lock_guard<mutex> lock(mtx);
    /* Get next free transfer object */
    auto ret = freeObjects.front();
    /* Remove object from free list */
    freeObjects.pop_front();
    return ret;
}

unsigned long TransferObjectPool::size() {
    return objects.size();
}

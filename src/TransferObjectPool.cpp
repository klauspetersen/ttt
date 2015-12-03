//
// Created by kape on 12/3/15.
//

#include "TransferObjectPool.h"

TransferObjectPool::TransferObjectPool() {

    objects.reserve(100);

}

void TransferObjectPool::free(sr_warp_transfer_t *ptr) {
    /* Add ptr to free list */
    freeObjects.push_front(ptr);
}

sr_warp_transfer_t* TransferObjectPool::alloc() {
    sr_warp_transfer_t* ret;
    /* Get next free transfer object */
    ret = freeObjects.front();
    /* Remove object from free list */
    freeObjects.pop_front();

    return ret;
}

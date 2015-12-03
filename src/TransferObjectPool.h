//
// Created by kape on 12/3/15.
//

#ifndef TTT_TRANSFEROBJECTPOOL_H
#define TTT_TRANSFEROBJECTPOOL_H

#include <list>
#include <vector>
#include "sigrok_wrapper.h"


class TransferObjectPool {
public:
    TransferObjectPool();
    void free(sr_warp_transfer_t *ptr);
    sr_warp_transfer_t* alloc();
private:
    std::list<sr_warp_transfer_t *> freeObjects;
    std::vector<sr_warp_transfer_t> objects;
};


#endif //TTT_TRANSFEROBJECTPOOL_H

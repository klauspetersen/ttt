//
// Created by kape on 12/4/15.
//

#include "catch.hpp"
#include "TransferObjectPool.h"

#define TRANSFER_OBJ_BUF_SIZE 160256

SCENARIO( "TransferObjectPool allocations", "[vector]" ) {

    GIVEN( "A transfer object pool" ) {
        TransferObjectPool pool(10, TRANSFER_OBJ_BUF_SIZE);

        REQUIRE( pool.size() == 10 );

        WHEN( "the size is increased" ) {
            sr_warp_transfer_t* obj = pool.alloc();

            THEN( "the size and capacity change" ) {
                REQUIRE( obj != NULL );
            }
        }
    }
}


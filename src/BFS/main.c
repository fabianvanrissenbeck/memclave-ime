/*
* BFS with multiple tasklets
*
*/
#include <stdio.h>

#include <alloc.h>
#include <barrier.h>
#include <defs.h>
#include <mram.h>
#include <mutex.h>
#include <perfcounter.h>

//#define SK_LOG_ENABLED 1
#define TASKLETS 16
#include "dpu-utils.h"
#include "support/common.h"
#include "support/log.h"

__host uint64_t tl_cycles[TASKLETS];

// Shared state in WRAM
__attribute__((aligned(8)))
static volatile struct {
    volatile uint32_t arrive[TASKLETS];
    volatile uint32_t release_epoch;
} gbar;

// Call once at start (before first use)
static inline void mybarrier_init(void) {
    if (me() == 0) {
        for (uint32_t i = 0; i < TASKLETS; i++) gbar.arrive[i] = 0;
        gbar.release_epoch = 0;
    }
}

static inline void mybarrier_wait(void) {
    const uint32_t tid = me();
    // Every barrier use increments the epoch by 1
    const uint32_t want = gbar.release_epoch + 1;

    // Signal arrival (write *own* slot only)
    gbar.arrive[tid] = want;

    if (tid == 0) {
        // Wait for all tasklets to reach this epoch
        for (uint32_t i = 0; i < TASKLETS; i++) {
            while (gbar.arrive[i] != want) { /* spin */ }
        }
        gbar.release_epoch = want;
    } else {
        while (gbar.release_epoch != want) { /* spin */ }
    }
}

#define TEST_OFFSET 0x1000   // pick any MRAM offset ≥ 0 and < 64 MiB-8
#define ARG_OFFSET 0x3000

int main1(void) {
    uint64_t val;
    // read the broadcast value
    mram_read((__mram_ptr void const*)TEST_OFFSET, &val, sizeof(val));
    // add our DPU’s own ID
    val += me();
    // write it back in the same place
    mram_write(&val, (__mram_ptr void*)TEST_OFFSET, sizeof(val));
    return 0;
}

volatile uint32_t t_spin1 = 0xbb;
volatile uint32_t t_spin0 = 0xaa;
// main
int main() {

    if(me() == 0) {
	mybarrier_init();
        mem_reset(); // Reset the heap
    	sk_log_init();
	perfcounter_config(COUNT_CYCLES, true);
    }
    for (int i = 0; i<100; i++);
    //mybarrier_wait();

    // Load parameters
    uint32_t params_m = (uint32_t) DPU_MRAM_HEAP_POINTER;
    struct DPUParams* params_w = (struct DPUParams*) mem_alloc(ROUND_UP_TO_MULTIPLE_OF_8(sizeof(struct DPUParams)));
    mram_read((__mram_ptr void const*)ARG_OFFSET, params_w, ROUND_UP_TO_MULTIPLE_OF_8(sizeof(struct DPUParams)));
    mybarrier_wait();
    uint32_t s = perfcounter_get();


    // Extract parameters
    uint32_t numGlobalNodes = params_w->numNodes;
    uint32_t startNodeIdx = params_w->dpuStartNodeIdx;
    uint32_t numNodes = params_w->dpuNumNodes;
    uint32_t nodePtrsOffset = params_w->dpuNodePtrsOffset;
    uint32_t level = params_w->level;
    uint32_t nodePtrs_m = params_w->dpuNodePtrs_m;
    uint32_t neighborIdxs_m = params_w->dpuNeighborIdxs_m;
    uint32_t nodeLevel_m = params_w->dpuNodeLevel_m;
    uint32_t visited_m = params_w->dpuVisited_m;
    uint32_t currentFrontier_m = params_w->dpuCurrentFrontier_m;
    uint32_t nextFrontier_m = params_w->dpuNextFrontier_m;
    uint32_t nextFrontierPriv_m = params_w->dpuNextFrontierPriv_m;

    // global bitmap geometry (you already check numGlobalNodes % 64 == 0 above)
    const uint32_t globalWords = numGlobalNodes / 64;
    const uint32_t priv_stride_bytes = globalWords * sizeof(uint64_t);
    const uint32_t next_priv_base = nextFrontierPriv_m + me() * priv_stride_bytes;

    /*sk_log_write_idx(0, numNodes);
    sk_log_write_idx(1, numNodes);
    sk_log_write_idx(2, numGlobalNodes);
    sk_log_write_idx(3, startNodeIdx);
    sk_log_write_idx(4, nodePtrsOffset);
    sk_log_write_idx(5, level);*/

    if(numNodes > 0) {

        // Sanity check
        if(me() == 0) {
            if(numGlobalNodes%64 != 0) {
                //PRINT_ERROR("The number of nodes in the graph is not a multiple of 64!");
            }
            if(startNodeIdx%64 != 0 || numNodes%64 != 0) {
                //PRINT_ERROR("The number of nodes assigned to the DPU is not aligned to or a multiple of 64!");
            }
        }

        // Allocate WRAM cache for each tasklet to use throughout
        uint64_t* cache_w = mem_alloc(2 * sizeof(uint64_t));
	// Zero this tasklet's private next-frontier shard
        //for (uint32_t w = 0; w < globalWords; ++w) {
        //    store8B(0, next_priv_base, w, cache_w);
        //}

        uint32_t firstPtr = load4B(nodePtrs_m, 0, cache_w);

        // Update current frontier and visited list based on the next frontier from the previous iteration
        for(uint32_t nodeTileIdx = me(); nodeTileIdx < numGlobalNodes/64; nodeTileIdx += TASKLETS) {

            // Get the next frontier tile from MRAM
            uint64_t nextFrontierTile = load8B(nextFrontier_m, nodeTileIdx, cache_w);

            // Process next frontier tile if it is not empty 
            if(nextFrontierTile) {

                // Mark everything that was previously added to the next frontier as visited
                uint64_t visitedTile = load8B(visited_m, nodeTileIdx, cache_w);
                visitedTile |= nextFrontierTile;
                store8B(visitedTile, visited_m, nodeTileIdx, cache_w);

                // Clear the next frontier
                store8B(0, nextFrontier_m, nodeTileIdx, cache_w);

            }

            // Extract the current frontier from the previous next frontier and update node levels
            uint32_t startTileIdx = startNodeIdx/64;
            uint32_t numTiles = numNodes/64;
            if(startTileIdx <= nodeTileIdx && nodeTileIdx < startTileIdx + numTiles) {

                // Update current frontier
                store8B(nextFrontierTile, currentFrontier_m, nodeTileIdx - startTileIdx, cache_w);

                // Update node levels
                if(nextFrontierTile) {
                    for(uint32_t node = nodeTileIdx*64; node < (nodeTileIdx + 1)*64; ++node) {
                        if(isSet(nextFrontierTile, node%64)) {
                            store4B(level, nodeLevel_m, node - startNodeIdx, cache_w); // No false sharing so no need for locks
                        }
                    }
                }
            }

        }
        // Wait until all tasklets have updated the current frontier
        mybarrier_wait();

        // Identify tasklet's nodes
        uint32_t numNodesPerTasklet = (numNodes + TASKLETS - 1)/TASKLETS;
        uint32_t taskletNodesStart = me()*numNodesPerTasklet;
        uint32_t taskletNumNodes;
        if(taskletNodesStart > numNodes) {
            taskletNumNodes = 0;
        } else if(taskletNodesStart + numNodesPerTasklet > numNodes) {
            taskletNumNodes = numNodes - taskletNodesStart;
        } else {
            taskletNumNodes = numNodesPerTasklet;
        }

        // Visit neighbors of the current frontier
        for(uint32_t node = taskletNodesStart; node < taskletNodesStart + taskletNumNodes; ++node) {
            uint32_t nodeTileIdx = node/64;
            uint64_t currentFrontierTile = load8B(currentFrontier_m, nodeTileIdx, cache_w); // TODO: Optimize: load tile then loop over nodes in the tile
            if(isSet(currentFrontierTile, node%64)) { // If the node is in the current frontier
                // Visit its neighbors
                uint32_t nodePtr = load4B(nodePtrs_m, node, cache_w) - nodePtrsOffset;
                uint32_t nextNodePtr = load4B(nodePtrs_m, node + 1, cache_w) - nodePtrsOffset; // TODO: Optimize: might be in the same 8B as nodePtr
                for(uint32_t i = nodePtr; i < nextNodePtr; ++i) {
                    uint32_t neighbor = load4B(neighborIdxs_m, i, cache_w); // TODO: Optimize: sequential access to neighbors can use sequential reader
                    uint32_t neighborTileIdx = neighbor/64;
                    uint64_t visitedTile = load8B(visited_m, neighborTileIdx, cache_w);
                    if(!isSet(visitedTile, neighbor%64)) { // Neighbor not previously visited
                        // Add neighbor to next frontier
                        uint64_t privWord = load8B(next_priv_base, neighborTileIdx, cache_w);
                        setBit(privWord, neighbor % 64);
                        store8B(privWord, next_priv_base, neighborTileIdx, cache_w);

                    }
                }
            }
        }
	mybarrier_wait();
	// Reduce per-tasklet shards into the global nextFrontier_m
        // One writer per global 64-bit word: w is striped by tasklet id
        for (uint32_t w = me(); w < globalWords; w += TASKLETS) {
            uint64_t acc = 0;
            // OR all TASKLETS' shards at word w
            for (uint32_t t = 0; t < TASKLETS; ++t) {
                const uint32_t base_t = nextFrontierPriv_m + t * priv_stride_bytes;
		uint64_t v = load8B(base_t, w, cache_w);
                acc |= v;
		if (v) store8B(0, base_t, w, cache_w);
            }
            store8B(acc, nextFrontier_m, w, cache_w);
        }

    }

#if 1
        mybarrier_wait();
        uint32_t e = perfcounter_get();
        uint64_t my = e - s;
        tl_cycles[me()] = my;
        mybarrier_wait();
    if (me() == 0) {
	uint64_t mx = 0;
	for (int t = 0; t < TASKLETS; t++)
	    if (tl_cycles[t] > mx) mx = tl_cycles[t];

        // Layout: 8×8B = 64B
        // [0]=magic, [1]=whole_kernel_cycles_max, [2]=s, [3]=e, [4]=(tasklets<<32)|layers,
        // [5]=0 (spare), [6]=0 (spare), [7]=1 (done)
        sk_log_write_idx(0, 0xffffULL);                    // "SKLOGV1"
        sk_log_write_idx(1, mx);
        sk_log_write_idx(2, (uint64_t)s);
        sk_log_write_idx(7, 1ULL);
    }
#endif

    return 0;
}

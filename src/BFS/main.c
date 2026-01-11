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

#define T size_t
#define NR_TASKLETS 16
#include "dpu-utils.h"
#include "support/common.h"
#include "support/log.h"
#include "support/mc_sync.h"

__host uint64_t tl_cycles[NR_TASKLETS];

#define ARG_OFFSET 0x3000

int main() {

    if (me() == 0) {
        mybarrier_init();
        mem_reset();
        sk_log_init();
    }

    // Load parameters from MRAM
    struct DPUParams* params_w =
        (struct DPUParams*)mem_alloc(ROUND_UP_TO_MULTIPLE_OF_8(sizeof(struct DPUParams)));
    mram_read((__mram_ptr void const*)ARG_OFFSET,
              params_w,
              ROUND_UP_TO_MULTIPLE_OF_8(sizeof(struct DPUParams)));

    mybarrier_wait();

    // Extract params
    const uint32_t numGlobalNodes  = params_w->numNodes;
    const uint32_t startNodeIdx    = params_w->dpuStartNodeIdx;
    const uint32_t numNodes        = params_w->dpuNumNodes;
    const uint32_t nodePtrsOffset  = params_w->dpuNodePtrsOffset;
    const uint32_t level           = params_w->level;

    const uint32_t nodePtrs_m      = params_w->dpuNodePtrs_m;
    const uint32_t neighborIdxs_m  = params_w->dpuNeighborIdxs_m;
    const uint32_t nodeLevel_m     = params_w->dpuNodeLevel_m;
    const uint32_t visited_m       = params_w->dpuVisited_m;
    const uint32_t currentFrontier_m = params_w->dpuCurrentFrontier_m;
    const uint32_t nextFrontier_m  = params_w->dpuNextFrontier_m;
    //const uint32_t nextFrontierPriv_m = params_w->dpuNextFrontierPriv_m;

    // global bitmap geometry
    const uint32_t globalWords = numGlobalNodes / 64;
    const uint32_t priv_stride_bytes = globalWords * sizeof(uint64_t);
    const uint32_t nextFrontierPriv_m = nextFrontier_m + priv_stride_bytes;
    const uint32_t next_priv_base = nextFrontierPriv_m + me() * priv_stride_bytes;

    // WRAM cache
    uint64_t* cache_w = (uint64_t*)mem_alloc(2 * sizeof(uint64_t));

    // ---- Clear this tasklet's private shard every invocation (fixes garbage on iter 0) ----
    for (uint32_t w = 0; w < globalWords; ++w) {
        store8B(0, next_priv_base, w, cache_w);
    }
    mybarrier_wait();

    for (uint32_t tile = me(); tile < globalWords; tile += NR_TASKLETS) {

        uint64_t nf = load8B(nextFrontier_m, tile, cache_w);

        if (nf) {
            uint64_t vis = load8B(visited_m, tile, cache_w);
            vis |= nf;
            store8B(vis, visited_m, tile, cache_w);
            store8B(0, nextFrontier_m, tile, cache_w);  // clear input
        } else {
            // still clear input to be safe (and keep output clean on dpus with 0 nodes)
            store8B(0, nextFrontier_m, tile, cache_w);
        }

        // Local partition update (safe even if numNodes==0 because numTiles==0)
        const uint32_t startTileIdx = startNodeIdx / 64;
        const uint32_t numTilesLocal = numNodes / 64;

        if (startTileIdx <= tile && tile < startTileIdx + numTilesLocal) {

            // current frontier is stored locally (per-DPU)
            store8B(nf, currentFrontier_m, tile - startTileIdx, cache_w);

            // update node levels for nodes in this tile
            if (nf) {
                for (uint32_t node = tile*64; node < (tile+1)*64; ++node) {
                    if (isSet(nf, node % 64)) {
                        store4B(level, nodeLevel_m, node - startNodeIdx, cache_w);
                    }
                }
            }
        }
    }

    mybarrier_wait();

    if (numNodes > 0) {
        const uint32_t numNodesPerTasklet = (numNodes + NR_TASKLETS - 1) / NR_TASKLETS;
        const uint32_t taskletNodesStart  = me() * numNodesPerTasklet;
        uint32_t taskletNumNodes;

        if (taskletNodesStart >= numNodes) {
            taskletNumNodes = 0;
        } else if (taskletNodesStart + numNodesPerTasklet > numNodes) {
            taskletNumNodes = numNodes - taskletNodesStart;
        } else {
            taskletNumNodes = numNodesPerTasklet;
        }

        for (uint32_t node = taskletNodesStart; node < taskletNodesStart + taskletNumNodes; ++node) {

            const uint32_t tile = node / 64;
            uint64_t cf = load8B(currentFrontier_m, tile, cache_w);
            if (!isSet(cf, node % 64)) continue;

            uint32_t ptr = load4B(nodePtrs_m, node, cache_w) - nodePtrsOffset;
            uint32_t nxt = load4B(nodePtrs_m, node + 1, cache_w) - nodePtrsOffset;

            for (uint32_t i = ptr; i < nxt; ++i) {
                uint32_t nb = load4B(neighborIdxs_m, i, cache_w);
                uint32_t nbTile = nb / 64;

                uint64_t vis = load8B(visited_m, nbTile, cache_w);
                if (!isSet(vis, nb % 64)) {
                    uint64_t w = load8B(next_priv_base, nbTile, cache_w);
                    setBit(w, nb % 64);
                    store8B(w, next_priv_base, nbTile, cache_w);
                }
            }
        }
    }

    mybarrier_wait();

    for (uint32_t w = me(); w < globalWords; w += NR_TASKLETS) {
        uint64_t acc = 0;
        for (uint32_t t = 0; t < NR_TASKLETS; ++t) {
            const uint32_t base_t = nextFrontierPriv_m + t * priv_stride_bytes;
            acc |= load8B(base_t, w, cache_w);
        }
        store8B(acc, nextFrontier_m, w, cache_w);
    }

    mybarrier_wait();
    return 0;
}

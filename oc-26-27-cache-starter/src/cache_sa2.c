/* STUDENT FILE: Part 2 - Two-way set-associative cache.
 * Implement only this mode here; cache.c supplies initialization/dispatch.
 * Keep helpers static and in this file. Do not call another student module.
 * Record CPU and lookup events through stats_record(). Memory transfers
 * record their own events automatically; never record those twice.
 */
#include "cache_impl.h"
#include "memory.h"
#include "stats.h"

uint8_t sa2_read(Cache *cache, uint32_t address)
{
    /* TODO: read one byte using this mode's mapping/replacement rules.
     * Emit one CPU_READ event and the appropriate lookup events.
     */
    int way;
    uint8_t offset, set;
    uint32_t tag, base, base_miss;
    uint64_t max_clock = 0x0;
    L1Line line, miss_line;

    offset = address % BLOCK_SIZE;
    set = (address / BLOCK_SIZE) % (L1_LINES / 2);
    tag = address / (BLOCK_SIZE * L1_LINES / 2);
    base = tag * BLOCK_SIZE * L1_LINES * 2 + set * BLOCK_SIZE; 

    //l1[2 * set + way]
    // Tratar de erros

    for (way = 1; way >= 0; way--) {
        line = cache->l1[2 * set + way];
        if (line.tag == tag) { return line.data[offset]; }// hit
        
        else if (max_clock < line.last_used) {
            max_clock = line.last_used;
            miss_line = line;
        }
        else if (!line.valid) { miss_line = line;}
    }

    if (miss_line.dirty) {   //miss
        memory_write_block(miss_line.tag * BLOCK_SIZE * L1_LINES * 2 + set * BLOCK_SIZE, miss_line.data);  //não é assim
        line.dirty = 0;
    }

    memory_read_block(base, miss_line.data); 
    miss_line.tag = tag;
    //adicionar clock time

    return 0;
}

void sa2_write(Cache *cache, uint32_t address, uint8_t value)
{
    /* TODO: write-back/write-allocate for this mode.
     * Emit one CPU_WRITE event and the appropriate lookup events.
     */
    
    int way;
    uint8_t offset, set;
    uint32_t tag, base;
    uint64_t max_clock = 0x0;
    L1Line line, miss_line;

    offset = address % BLOCK_SIZE;
    set = (address / BLOCK_SIZE) % (L1_LINES / 2);
    tag = address / (BLOCK_SIZE * L1_LINES / 2);
    base = tag * BLOCK_SIZE * L1_LINES + set * BLOCK_SIZE; 

    //l1[2 * set + way]
    // Tratar de erros

    for (way = 1; way >= 0; way--) {
        line = cache->l1[2 * set + way];
        
        if (line.tag == tag) { 
            line.data[offset] = value;
            line.dirty = 1;
            return value; 
        }// hit
        
        else if (max_clock < line.last_used) {
            max_clock = line.last_used;
            miss_line = line;
        }
        else if (line.valid) { miss_line = line;}
    }

    if (miss_line.dirty) {   //miss
        memory_write_block(miss_line.tag * BLOCK_SIZE * L1_LINES * 2 + set * BLOCK_SIZE, line.data);
        line.dirty = 0;
    }
    miss_line.data[offset] = value;
    miss_line.tag = tag;
    miss_line.dirty = 1;
    miss_line.tag = tag;
    //adicionar clock time

    return 0;
}

void sa2_flush(Cache *cache)
{
    /* TODO: flush this mode without invalidation or recency changes.
     * Emit no CPU/lookup events. Memory writes record their own events.
     */

    uint8_t offset, index, set, way;
    uint32_t tag, base;
    L1Line line;


    for (set; set < L1_LINES / 2; set++ ) {
        for (way; way <= 1; way++) {
            line = cache->l1[2 * set + way];

            if (line.dirty && line.valid) {
                base = line.tag * BLOCK_SIZE * L1_LINES * 2 + set * BLOCK_SIZE;
                memory_write_block(base , line.data);
                line.dirty = 0;
            }
        }
    }

}

/* STUDENT FILE: Part 1 - Direct-mapped cache.
 * Implement only this mode here; cache.c supplies initialization/dispatch.
 * Keep helpers static and in this file. Do not call another student module.
 * Record CPU and lookup events through stats_record(). Memory transfers
 * record their own events automatically; never record those twice.
 */
#include "cache_impl.h"
#include "memory.h"
#include "stats.h"

uint8_t dm_read(Cache *cache, uint32_t address)
{
    /* TODO: read one byte using this mode's mapping/replacement rules.
     * Emit one CPU_READ event and the appropriate lookup events.
     */
    uint8_t offset, index;
    uint32_t tag, base, base_miss;
    L1Line line;

    offset = address % BLOCK_SIZE;
    index = address / BLOCK_SIZE % L1_LINES;
    tag = address / (BLOCK_SIZE * L1_LINES);

    base = tag * BLOCK_SIZE * L1_LINES + index * BLOCK_SIZE; 

    line = cache->l1[index];

    base_miss = line.tag * BLOCK_SIZE * L1_LINES + index * BLOCK_SIZE;


    // Tratar de erros

    if (line.tag == tag) { return line.data[offset]; }// hit

    if (line.dirty) {
        memory_write_block(base_miss, line.data);
        line.dirty = 0;
    } 
    line.tag = tag;

    memory_read_block(base, line.data);
    return line.data[offset];  //N sei se é para por 0
}

void dm_write(Cache *cache, uint32_t address, uint8_t value)
{
    /* TODO: write-back/write-allocate for this mode.
     * Emit one CPU_WRITE event and the appropriate lookup events.
     */

    uint8_t offset, index;
    uint32_t tag, base, base_miss;
    L1Line line;

    offset = address % BLOCK_SIZE;
    index = address / BLOCK_SIZE % L1_LINES;
    tag = address / (BLOCK_SIZE * L1_LINES);
    base = tag * BLOCK_SIZE * L1_LINES + index * BLOCK_SIZE; 

    line = cache->l1[index];

    base_miss = line.tag * BLOCK_SIZE * L1_LINES + index * BLOCK_SIZE;


    // Tratar de erros
    
    if (line.tag == tag) { 
        line.data[offset] = value;
        line.dirty = 1;
        return value; 
    }// hit

    else {
        if (line.dirty) memory_write_block(line.tag * BLOCK_SIZE * L1_LINES + index * BLOCK_SIZE, line.data);
        line.data[offset] = value;
        line.tag = tag;
        line.dirty = 1;
    }
    return value;


}

void dm_flush(Cache *cache)
{
    /* TODO: flush this mode without invalidation or recency changes.
     * Emit no CPU/lookup events. Memory writes record their own events.
     */

    uint8_t offset, index;
    uint32_t tag, base;
    L1Line line;


    for (int index; index < L1_LINES; index++ ) {
        line = cache->l1[index];

        if (line.dirty && line.valid) {
            base = line.tag * BLOCK_SIZE * L1_LINES + index * BLOCK_SIZE;
            memory_write_block(base , line.data);
            line.dirty = 0;
        }
    }
}

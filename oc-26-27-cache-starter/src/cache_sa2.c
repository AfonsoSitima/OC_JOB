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
    uint32_t tag, base;
    L1Line *line, *miss_line;

    offset = address % BLOCK_SIZE;
    set = (address / BLOCK_SIZE) % (L1_LINES / 2);
    tag = address / (BLOCK_SIZE * L1_LINES / 2);
    base = tag * BLOCK_SIZE * (L1_LINES / 2) + set * BLOCK_SIZE;    
    
    stats_record(EVENT_CPU_READ);


    for (way = 0; way < 2; way++) {
        line = &cache->l1[2 * set + way];
        if (line->tag == tag && line->valid) { // hit
            
            stats_record(EVENT_L1_HIT);
            //atualizar last_used
            line->last_used = ++cache->clock;
            return line->data[offset]; 
        }
    }

    stats_record(EVENT_L1_MISS);
    //l1 miss
    for (way = 0; way < 2; way++) {
        miss_line = &cache->l1[2 * set + way]; 
        if(!miss_line->valid){
            //linha livre - colocar imediatamente
            memory_read_block(base, miss_line->data);
            miss_line->valid = true;
            miss_line->dirty = 0;
            miss_line->tag = tag;
            miss_line->last_used = ++cache->clock;
            return miss_line->data[offset];
        }
    }

    //MISS - caso não haja linhas invalidas -> procurar a que tem menor LRU dentro do set
    miss_line = &cache->l1[2 * set];
    for (way = 1; way < 2; way++){ //loop caso o n de ways aumente 
        line = &cache->l1[2 * set + way];
    
        if (miss_line->last_used > line->last_used) {
            miss_line = line;
        }
    }    
    if (miss_line->dirty) {  
        //write-back da linha expulsa
        memory_write_block((miss_line->tag * (L1_LINES / 2) + set) * BLOCK_SIZE, miss_line->data);  
        miss_line->dirty = 0;
    }

    //atualizar com nova info
    memory_read_block(base, miss_line->data); 
    miss_line->tag = tag;
    miss_line->valid = true;
    miss_line->last_used = ++cache->clock;
    return miss_line->data[offset];
    
    
}

void sa2_write(Cache *cache, uint32_t address, uint8_t value)
{
    /* TODO: write-back/write-allocate for this mode.
     * Emit one CPU_WRITE event and the appropriate lookup events.
     */
    
 int way;
    uint8_t offset, set;
    uint32_t tag, base;
    L1Line *line, *miss_line;

    offset = address % BLOCK_SIZE;
    set = (address / BLOCK_SIZE) % (L1_LINES / 2);
    tag = address / (BLOCK_SIZE * L1_LINES / 2);
    base = tag * BLOCK_SIZE * (L1_LINES / 2) + set * BLOCK_SIZE;    
    
    stats_record(EVENT_CPU_WRITE);


    for (way = 0; way < 2; way++) {
        line = &cache->l1[2 * set + way];
        if (line->tag == tag && line->valid) { // hit
            
            stats_record(EVENT_L1_HIT);
            //atualizar last_used
            line->last_used = ++cache->clock;
            line->data[offset] = value; 
            line->dirty = true;
            return;
        }
    }

    stats_record(EVENT_L1_MISS);

    for (way = 0; way < 2; way++) {
        miss_line = &cache->l1[2 * set + way]; 
        if(!miss_line->valid){
            //linha livre - colocar imediatamente
            memory_read_block(base, miss_line->data);
            miss_line->valid = true;
            miss_line->tag = tag;
            miss_line->last_used = ++cache->clock;
            miss_line->data[offset] = value;
            miss_line->dirty = true;
            return;
        }
    }

    //MISS - caso não haja linhas invalidas -> procurar a que tem menor LRU dentro do set
    miss_line = &cache->l1[2 * set];
    for (way = 1; way < 2; way++){
        line = &cache->l1[2 * set + way];
    
        if (miss_line->last_used > line->last_used) {
            miss_line = line;
        }
    }    
    if (miss_line->dirty) {  
        //write-back da linha expulsa
        memory_write_block((miss_line->tag * (L1_LINES / 2) + set) * BLOCK_SIZE, miss_line->data);  
        miss_line->dirty = 0;
    }

    //atualizar com nova info
    memory_read_block(base, miss_line->data); 
    miss_line->tag = tag;
    miss_line->valid = true;
    miss_line->last_used = ++cache->clock;
    miss_line->data[offset] = value;
    miss_line->dirty = true;
    
    return;
}

void sa2_flush(Cache *cache)
{
    /* TODO: flush this mode without invalidation or recency changes.
     * Emit no CPU/lookup events. Memory writes record their own events.
     */

    uint8_t set, way;
    uint32_t base;
    L1Line *line;


    for (set = 0; set < L1_LINES / 2; set++ ) {
        for (way = 0; way <= 1; way++) {
            line = &cache->l1[2 * set + way];

            if (line->dirty && line->valid) {
                base = line->tag * BLOCK_SIZE * (L1_LINES / 2) + set * BLOCK_SIZE;    
                memory_write_block(base , line->data);
                line->dirty = 0;

            }
        }
    }

}

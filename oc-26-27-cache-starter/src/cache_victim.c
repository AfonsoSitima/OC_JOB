/* STUDENT FILE: Part 3 - Direct-mapped L1 plus victim cache.
 * Implement only this mode here; cache.c supplies initialization/dispatch.
 * Keep helpers static and in this file. Do not call another student module.
 * Record CPU and lookup events through stats_record(). Memory transfers
 * record their own events automatically; never record those twice.
 */
#include "cache_impl.h"
#include "memory.h"
#include "stats.h"
#include <string.h>

uint8_t victim_read(Cache *cache, uint32_t address)
{
    /* TODO: read one byte using this mode's mapping/replacement rules.
     * Emit one CPU_READ event and the appropriate lookup events.
     */
    uint8_t offset, index;
    uint32_t tag, base, base_miss, blockNum;
    bool vic_hit = false;
    offset = address % BLOCK_SIZE;
    index = address / BLOCK_SIZE % L1_LINES;
    tag = address / (BLOCK_SIZE * L1_LINES);
    blockNum = address / BLOCK_SIZE;

    base = tag * BLOCK_SIZE * L1_LINES + index * BLOCK_SIZE; 
    L1Line *line = &cache->l1[index];
    base_miss = line->tag * BLOCK_SIZE * L1_LINES + index * BLOCK_SIZE;

    if (line->tag == tag && line->valid) { 
        return line->data[offset]; 
    } else {
        for (int i = 0; i < VICTIM_BLOCKS; i++){
            if (cache->victim[i].block_number == blockNum && cache->victim[i].valid){
                if (line->valid){
                    //trocar o da victim com o da L1
                    //guardar vicEntry
                    VictimEntry aux = cache->victim[i];

                    //entry de L1 que vai para Victim
                    VictimEntry *v_new = &cache->victim[i];
                    v_new->valid = true;
                    v_new->dirty = line->dirty;
                    v_new->block_number = line->tag * L1_LINES + index; //block number com base na L1
                    memcpy(v_new->data, line->data, BLOCK_SIZE);
                    v_new->last_used = ++cache->clock; 

                    //troca
                    L1Line *l1_new = &cache->l1[index]; //aponta logo para a entry onde se vai mudar
                    l1_new->valid = true;
                    l1_new->dirty = aux.dirty;
                    l1_new->tag = tag;
                    memcpy(l1_new->data, aux.data, BLOCK_SIZE);
                    
                    return l1_new->data[offset];
                    
                }else {
                    //caso o sítio da L1 esteja vazio ponho o bloco da victim lá
                    L1Line *l1_new = &cache->l1[index];
                    l1_new->valid = true;
                    l1_new->dirty = cache->victim[i].dirty;
                    memcpy(l1_new->data, cache->victim[i].data, BLOCK_SIZE);
                    l1_new->tag = tag;

                    cache->victim[i].valid = false; //so pode estar 1 sitio
                    return l1_new->data[offset];
                }
            }
        }
        //victim miss - ir buscar a memo para por em L1
        if(line->valid){
            //caso em que na L1 já há algo, portanto tenho de retirar e meter esse algo na victim
            
            
            for (int j = 0; j < VICTIM_BLOCKS; j++){
                if(!cache->victim[j].valid){
                    //entrada livre para guardar o que saiu de L1
                    VictimEntry *v_new = &cache->victim[j];
                    memcpy(v_new->data, line->data, BLOCK_SIZE);
                    v_new->valid = true;
                    v_new->dirty = line->dirty;
                    v_new->block_number = line->tag * L1_LINES + index;
                    v_new->last_used = ++cache->clock;

                    memory_read_block(base, line->data);
                    line->valid = true;
                    line->dirty = 0;
                    line->tag = tag;
                    return line->data[offset];
                }
                //caso todas estejam válidas usa-se a com menor LRU
            }
            //desempate por LRU
            int minLRU = 0; //first index
            for (int j = 1; j < VICTIM_BLOCKS; j++){
                if (cache->victim[j].last_used < cache->victim[minLRU].last_used){
                    minLRU = j;
                }
            }
            if(cache->victim[minLRU].dirty){
                memory_write_block(cache->victim[minLRU].block_number * BLOCK_SIZE, cache->victim[minLRU].data);
            }
            //atualiza o victim no index minLRU
            VictimEntry *v_new = &cache->victim[minLRU];
            memcpy(v_new->data, line->data, BLOCK_SIZE);
            v_new->valid = true;
            v_new->dirty = line->dirty;
            v_new->block_number = line->tag * L1_LINES + index;
            v_new->last_used = ++cache->clock;


            
            memory_read_block(base, line->data);
            line->valid = true;
            line->dirty = 0;
            line->tag = tag;
            
            return line->data[offset];
        
        } else{
            memory_read_block(base, line->data);
            line->valid = true;
            line->dirty = 0;
            line->tag = tag;
            return line->data[offset];
        }
        
    

    return 0;
}
}

void victim_write(Cache *cache, uint32_t address, uint8_t value){
    /* TODO: write-back/write-allocate for this mode.
     * Emit one CPU_WRITE event and the appropriate lookup events.
     */
    uint8_t offset, index;
    uint32_t tag, base, base_miss, blockNum;
    bool vic_hit = false;
    offset = address % BLOCK_SIZE;
    index = address / BLOCK_SIZE % L1_LINES;
    tag = address / (BLOCK_SIZE * L1_LINES);
    blockNum = address / BLOCK_SIZE;

    base = tag * BLOCK_SIZE * L1_LINES + index * BLOCK_SIZE; 
    L1Line *line = &cache->l1[index];
    base_miss = line->tag * BLOCK_SIZE * L1_LINES + index * BLOCK_SIZE;

    if (line->tag == tag && line->valid) { 
        line->data[offset] = value;
        line->dirty = true; 
        return;
    } else {
        for (int i = 0; i < VICTIM_BLOCKS; i++){
            if (cache->victim[i].block_number == blockNum && cache->victim[i].valid){
                if (line->valid){
                    //trocar o da victim com o da L1
                    //guardar vicEntry
                    VictimEntry aux = cache->victim[i];

                    //entry de L1 que vai para Victim
                    VictimEntry *v_new = &cache->victim[i];
                    v_new->valid = true;
                    v_new->dirty = line->dirty;
                    v_new->block_number = line->tag * L1_LINES + index; //block number com base na L1
                    memcpy(v_new->data, line->data, BLOCK_SIZE);
                    v_new->last_used = ++cache->clock; 

                    //troca
                    L1Line *l1_new = &cache->l1[index]; //aponta logo para a entry onde se vai mudar
                    l1_new->valid = true;
                    l1_new->dirty = aux.dirty;
                    l1_new->tag = tag;
                    memcpy(l1_new->data, aux.data, BLOCK_SIZE);
                    
                    l1_new->data[offset] = value;
                    line->dirty = true;
                    return;
                    
                }else {
                    //caso o sítio da L1 esteja vazio ponho o bloco da victim lá
                    L1Line *l1_new = &cache->l1[index];
                    l1_new->valid = true;
                    l1_new->dirty = cache->victim[i].dirty;
                    memcpy(l1_new->data, cache->victim[i].data, BLOCK_SIZE);
                    l1_new->tag = tag;

                    cache->victim[i].valid = false; //so pode estar 1 sitio
                    l1_new->data[offset] = value;
                    line->dirty = true;
                    return;
                }
            }
        }
            //victim miss - ir buscar a memo para por em L1
        if(line->valid){
            //caso em que na L1 já há algo, portanto tenho de retirar e meter esse algo na victim
            
            
            for (int j = 0; j < VICTIM_BLOCKS; j++){
                if(!cache->victim[j].valid){
                    //entrada livre para guardar o que saiu de L1
                    VictimEntry *v_new = &cache->victim[j];
                    memcpy(v_new->data, line->data, BLOCK_SIZE);
                    v_new->valid = true;
                    v_new->dirty = line->dirty;
                    v_new->block_number = line->tag * L1_LINES + index;
                    v_new->last_used = ++cache->clock;

                    memory_read_block(base, line->data);
                    line->valid = true;
                    line->dirty = 0;
                    line->tag = tag;
                    line->data[offset] = value;
                    line->dirty = true;
                    return;
                }
                //caso todas estejam válidas usa-se a com menor LRU
            }
            //desempate por LRU
            int minLRU = 0; //first index
            for (int j = 1; j < VICTIM_BLOCKS; j++){
                if (cache->victim[j].last_used < cache->victim[minLRU].last_used){
                    minLRU = j;
                }
            }
            if(cache->victim[minLRU].dirty){
                memory_write_block(cache->victim[minLRU].block_number * BLOCK_SIZE, cache->victim[minLRU].data);
            }
            //atualiza o victim no index minLRU
            VictimEntry *v_new = &cache->victim[minLRU];
            memcpy(v_new->data, line->data, BLOCK_SIZE);
            v_new->valid = true;
            v_new->dirty = line->dirty;
            v_new->block_number = line->tag * L1_LINES + index;
            v_new->last_used = ++cache->clock;


            
            memory_read_block(base, line->data);
            line->valid = true;
            line->dirty = 0;
            line->tag = tag;
            
            line->data[offset] = value;
            line->dirty = true;
            return;
        
        } else{
            memory_read_block(base, line->data);
            line->valid = true;
            line->dirty = 0;
            line->tag = tag;
            line->data[offset] = value;
            line->dirty = true;
            return;
        }
    }
}

void victim_flush(Cache *cache)
{
    /* TODO: flush this mode without invalidation or recency changes.
     * Emit no CPU/lookup events. Memory writes record their own events.
     */
    uint8_t offset, index;
    uint32_t tag, base;
    L1Line *line;
    VictimEntry *vEntry;

    for (index; index < L1_LINES; index++){
        line = &cache->l1[index];

        if(line->dirty && line->valid) {
            base = line->tag * BLOCK_SIZE * L1_LINES + index * BLOCK_SIZE;
            memory_write_block(base , line->data);
            line->dirty = 0;
        }
    }

    for (index = 0; index < VICTIM_BLOCKS; index++){
        vEntry = &cache->victim[index];

        if(vEntry->dirty && vEntry->valid){
            base = vEntry->block_number * BLOCK_SIZE;

            memory_write_block(base, vEntry->data);
            vEntry->dirty = 0;
        }
    }

}

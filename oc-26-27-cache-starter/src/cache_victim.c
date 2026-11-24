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
    uint32_t tag, base, blockNum;

    offset = address % BLOCK_SIZE;
    index = address / BLOCK_SIZE % L1_LINES;
    tag = address / (BLOCK_SIZE * L1_LINES);
    blockNum = address / BLOCK_SIZE;

    base = tag * BLOCK_SIZE * L1_LINES + index * BLOCK_SIZE; 
    L1Line *line = &cache->l1[index];

    stats_record(EVENT_CPU_READ);

    if (line->tag == tag && line->valid) {  //CASO DE HIT L1
        stats_record(EVENT_L1_HIT);
        return line->data[offset]; 
    } else {
        stats_record(EVENT_L1_MISS);
        for (unsigned int i = 0; i < VICTIM_BLOCKS; i++){
            if (cache->victim[i].block_number == blockNum && cache->victim[i].valid){
                stats_record(EVENT_VICTIM_HIT);
                if (line->valid){ //ocupada

                    //SWAP victim com o da L1
                    //guardar vicEntry
                    VictimEntry aux = cache->victim[i];

                    //entry de L1 que vai para Victim
                    VictimEntry *v_new = &cache->victim[i];
                    v_new->valid = true;
                    v_new->dirty = line->dirty;
                    v_new->block_number = line->tag * L1_LINES + index; //block number com base na L1
                    memcpy(v_new->data, line->data, BLOCK_SIZE);
                    v_new->last_used = ++cache->clock; 

                    //troca L1 line para a info da entry da victim
                    L1Line *l1_new = &cache->l1[index]; //aponta logo para a entry onde se vai mudar
                    l1_new->valid = true;
                    l1_new->dirty = aux.dirty;
                    l1_new->tag = tag;
                    memcpy(l1_new->data, aux.data, BLOCK_SIZE);
                    
                    return l1_new->data[offset];
                    
                }else { 
                    //caso o sítio da L1 esteja vazio colocar bloco da victim 
                    L1Line *l1_new = &cache->l1[index];
                    l1_new->valid = true;
                    l1_new->dirty = cache->victim[i].dirty;
                    memcpy(l1_new->data, cache->victim[i].data, BLOCK_SIZE);
                    l1_new->tag = tag;

                    cache->victim[i].valid = false; //so pode estar em 1 sitio
                    return l1_new->data[offset];
                }
            }
        }

        stats_record(EVENT_VICTIM_MISS);

        //victim miss - ir buscar a memo para por em L1
        if(line->valid){
            //caso em que na L1 já há algo, portanto tenho de retirar e meter esse algo na victim
            
            for (unsigned int j = 0; j < VICTIM_BLOCKS; j++){
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
            for (unsigned int j = 1; j < VICTIM_BLOCKS; j++){
                if (cache->victim[j].last_used < cache->victim[minLRU].last_used){
                    minLRU = j;
                }
            }
            if(cache->victim[minLRU].dirty){
                //antes de expulsar da victim escrever na memória
                memory_write_block(cache->victim[minLRU].block_number * BLOCK_SIZE, cache->victim[minLRU].data);
                                
            }
            //atualiza o victim no index minLRU 
            VictimEntry *v_new = &cache->victim[minLRU];
            memcpy(v_new->data, line->data, BLOCK_SIZE);
            v_new->valid = true;
            v_new->dirty = line->dirty;
            v_new->block_number = line->tag * L1_LINES + index;
            v_new->last_used = ++cache->clock;


            //ler bloco novo para entrada de L1 
            memory_read_block(base, line->data);
            line->valid = true;
            line->dirty = 0;
            line->tag = tag;
            
            return line->data[offset];
        
        } else{
            //caso entrada de L1 estivesse invalida - apenas colocar
            memory_read_block(base, line->data);
            line->valid = true;
            line->dirty = 0;
            line->tag = tag;

            return line->data[offset];
        }
        
    

}
}

void victim_write(Cache *cache, uint32_t address, uint8_t value){
    /* TODO: write-back/write-allocate for this mode.
     * Emit one CPU_WRITE event and the appropriate lookup events.
     */
    uint8_t offset, index;
    uint32_t tag, base, blockNum;
    
    offset = address % BLOCK_SIZE;
    index = address / BLOCK_SIZE % L1_LINES;
    tag = address / (BLOCK_SIZE * L1_LINES);
    blockNum = address / BLOCK_SIZE;

    base = tag * BLOCK_SIZE * L1_LINES + index * BLOCK_SIZE; 
    L1Line *line = &cache->l1[index];

    stats_record(EVENT_CPU_WRITE);

    if (line->tag == tag && line->valid) { //CASO HIT L1
        line->data[offset] = value;
        line->dirty = true; 

        stats_record(EVENT_L1_HIT);

        return;
    } else {
        stats_record(EVENT_L1_MISS);
        for (unsigned int i = 0; i < VICTIM_BLOCKS; i++){
            if (cache->victim[i].block_number == blockNum && cache->victim[i].valid){
                stats_record(EVENT_VICTIM_HIT);
                if (line->valid){
                    //SWAP victim com o da L1
                    //guardar vicEntry
                    VictimEntry aux = cache->victim[i];

                    //entry de L1 que vai para Victim
                    VictimEntry *v_new = &cache->victim[i];
                    v_new->valid = true;
                    v_new->dirty = line->dirty;
                    v_new->block_number = line->tag * L1_LINES + index; //block number com base na L1
                    memcpy(v_new->data, line->data, BLOCK_SIZE);
                    v_new->last_used = ++cache->clock; 

                    //troca L1 line para a info da entry da victim
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

        stats_record(EVENT_VICTIM_MISS);

        //victim miss - ir buscar a memo para por em L1
        if(line->valid){
            //caso em que na L1 já há algo, portanto tenho de retirar e meter esse algo na victim
            
            
            for (unsigned int j = 0; j < VICTIM_BLOCKS; j++){
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
            for (unsigned int j = 1; j < VICTIM_BLOCKS; j++){
                if (cache->victim[j].last_used < cache->victim[minLRU].last_used){
                    minLRU = j;
                }
            }
            if(cache->victim[minLRU].dirty){
                //antes de expulsar da victim escrever na memória
                memory_write_block(cache->victim[minLRU].block_number * BLOCK_SIZE, cache->victim[minLRU].data);
                
            }
            //atualiza o victim no index minLRU
            VictimEntry *v_new = &cache->victim[minLRU];
            memcpy(v_new->data, line->data, BLOCK_SIZE);
            v_new->valid = true;
            v_new->dirty = line->dirty;
            v_new->block_number = line->tag * L1_LINES + index;
            v_new->last_used = ++cache->clock;


            //ler bloco novo para entrada de L1 
            memory_read_block(base, line->data);
            line->valid = true;
            line->dirty = 0;
            line->tag = tag;
            line->data[offset] = value;
            line->dirty = true;

            return;
        
        } else{
            //caso entrada de L1 estivesse invalida - apenas colocar
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
    uint8_t index;
    uint32_t base;
    L1Line *line;
    VictimEntry *vEntry;

    for (index = 0; index < L1_LINES; index++){
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

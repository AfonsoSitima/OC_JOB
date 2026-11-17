/* STUDENT FILE: Part 4. Do not print to stdout. */
#include "config.h"
#include "stats.h"

static Stats counters;

void stats_reset(void)
{
    /* TODO: reset every performance counter and simulated time. */
    counters.reads = 0;
    counters.writes = 0;
    counters.l1_hits = 0;
    counters.l1_misses = 0;
    counters.victim_hits = 0;
    counters.victim_misses = 0;
    counters.memory_reads = 0;
    counters.memory_writes = 0;
    counters.cycles = 0;
}

void stats_record(Event event)
{
    /* TODO: account for exactly one event using the costs in config.h. */
    switch (event)
    {
    case EVENT_CPU_READ:
        /* code */
        counters.reads++;
        //no cycles 
        break;
    case EVENT_CPU_WRITE:
        counters.writes++;
        //no cycles 
        break;
    case EVENT_L1_HIT:
        counters.l1_hits++;
        counters.cycles += L1_CYCLES;
        //compute cycles
        break;
    case EVENT_L1_MISS:
        counters.l1_misses++;
        counters.cycles += L1_CYCLES;
        break;
    case EVENT_VICTIM_HIT:
        counters.victim_hits++;
        counters.cycles += VICTIM_CYCLES;
        break;
    case EVENT_VICTIM_MISS:
        counters.victim_misses++;
        counters.cycles += VICTIM_CYCLES;
        break;
    case EVENT_MEMORY_READ:
        counters.memory_reads++;
        counters.cycles += MEMORY_READ_CYCLES;
        break;
    case EVENT_MEMORY_WRITE:
        counters.memory_writes++;
        counters.cycles += MEMORY_WRITE_CYCLES;
        break;
    default:
        break;
    }
}

Stats stats_get(void)
{
    /* Supplied accessor. The student code above maintains counters. */
    return counters;
}

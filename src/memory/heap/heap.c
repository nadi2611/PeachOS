#include "heap.h"
#include "kernel.h"
#include "status.h"
#include "memory/memory.h"
#include <stdbool.h>

static int heap_validate_table(void* ptr, void* end, struct heap_table* table)
{
    int res = 0;

    size_t table_size = (size_t)(end - ptr);
    size_t total_blocks = table_size / PEACHOS_HEAP_BLOCK_SIZE;
    if (table->total != total_blocks)
    {
        res = -EINVARG;
        goto out;
    }

out:
    return res;
}

static bool heap_validate_alignment(void* ptr)
{
    return ((unsigned int)ptr % PEACHOS_HEAP_BLOCK_SIZE) == 0;
}

int heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* table)
{
    int res = 0;
    
    if (!heap_validate_alignment(ptr) || !heap_validate_alignment(end))
    {
        res = -EINVARG;
        goto out;
    }

    memset(heap, 0, sizeof(struct heap));
    heap->saddr = ptr;
    heap->table = table;

    res = heap_validate_table(ptr, end, table);
    if (res < 0)
    {
        goto out;
    }

    size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * table->total;
    memset(table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, table_size);

out:
    return res;
}

static uint32_t heap_align_value_to_upper(uint32_t val)
{
    if((val % PEACHOS_HEAP_BLOCK_SIZE) == 0)
    {
        return val;
    }

    val = (val - (val % PEACHOS_HEAP_BLOCK_SIZE));
    val += PEACHOS_HEAP_BLOCK_SIZE;
    return val;
}

static int heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry)
{
    return entry & 0x0f;
}
int heap_get_start_block(struct heap* heap, uint32_t total_blocks)
{
    struct heap_table* table = heap->table;
    int bc = 0;
    int bs = -1;

    for (size_t i = 0; i < table->total; i++)
    {
        if (heap_get_entry_type(table->entries[i]) != HEAP_BLOCK_TABLE_ENTRY_FREE)
        {
            bc = 0;
            bs = -1;
            continue;
        }

        // If this is the first block
        if (bs == -1)
        {
            bs = i;
        }
        bc++;

        if (bc == total_blocks)
        {
            break;
        }
    }

    if (bs == -1)
    {
        return -ENOMEM;
    }

    return bs;
}
void* heap_malloc_blocks(struct heap* heap, uint32_t total_blocks)
{
    void* address = 0;

    int start_block = heap_get_start_block(heap, total_blocks);
    if (start_block < 0)
    {
        goto out;
    }

    address = heap_block_to_address(heap, start_block);

    // Mark blocks as taken
    heap_mark_blocks_taken(heap, start_block, total_blocks);


out:
    return address;



}
void* heap_malloc(struct heap* heap, size_t size)
{
    size_t aligned_size = heap_align_value_to_upper(size);
    uint32_t total_blocks = aligned_size / PEACHOS_HEAP_BLOCK_SIZE;


    return heap_malloc_blocks(heap, total_blocks);
}

void* heap_free(struct heap* heap, void* ptr)
{
    return 0;
}
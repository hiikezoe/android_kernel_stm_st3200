/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2008-2011 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/* needed to detect kernel version specific code */
#include <linux/version.h>

#include <linux/semaphore.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <asm/atomic.h>
#include <linux/vmalloc.h>
#include "ump_kernel_common.h"
#include "ump_kernel_memory_backend.h"

#include <linux/bpa2.h>


#define MAX_GFX_PARTITION 4

static const unsigned char* partnames[MAX_GFX_PARTITION] =
{
        "gfx-memory-0", "gfx-memory-1", "BPA2_Region1", "BPA2_Region0"
};

typedef struct bpa2_allocator
{
    struct semaphore mutex;

    struct {
        struct bpa2_part *bpa2;
        unsigned long base;
        unsigned long end;
    } partition[MAX_GFX_PARTITION];

    int partNumber;
} bpa2_allocator;


static void bpa2_allocator_shutdown(ump_memory_backend * backend);
static int bpa2_allocator_allocate(void* ctx, ump_dd_mem * mem);
static void bpa2_allocator_release(void * ctx, ump_dd_mem * handle);

/*
 * Create dedicated memory backend
 */
ump_memory_backend * ump_bpa2_memory_backend_create(void)
{
    ump_memory_backend * backend;
	bpa2_allocator * allocator;

	DBG_MSG(2, ("Creating BPA2 UMP memory backend\n"));

	backend = kzalloc(sizeof(ump_memory_backend), GFP_KERNEL);
	if (NULL != backend)
	{
		allocator = kmalloc(sizeof(bpa2_allocator), GFP_KERNEL);
		if (NULL != allocator)
		{
		    int i;

		    backend->ctx = allocator;
		    backend->allocate = bpa2_allocator_allocate;
		    backend->release = bpa2_allocator_release;
		    backend->shutdown = bpa2_allocator_shutdown;
		    backend->stat = NULL;
		    backend->pre_allocate_physical_check = NULL;
		    backend->adjust_to_mali_phys = NULL;

		    allocator->partNumber = 0;
            sema_init(&allocator->mutex, 1);

		    /*
		     * WorkAround to blitter limitation
		     * Split GFX memory in  several partition not crossing 64MB boundary.
		     * And thus ensure we don't cross a 64MB boundary.
		     */
		    for(i = 0; i < MAX_GFX_PARTITION; i++)
		    {
		        allocator->partition[allocator->partNumber].bpa2 = bpa2_find_part(partnames[i]);
		        if (NULL != allocator->partition[allocator->partNumber].bpa2)
		        {
		            long size;

		            bpa2_memory(allocator->partition[allocator->partNumber].bpa2,
		                    &allocator->partition[allocator->partNumber].base,
		                    &size);

		            allocator->partition[allocator->partNumber].end = allocator->partition[allocator->partNumber].base + size;

		            DBG_MSG(2, (" %s => [%x : %x[ (%dMB)\n", partnames[i],
		                    allocator->partition[allocator->partNumber].base,
		                    allocator->partition[allocator->partNumber].end,
		                    size / 1024 / 1024));

		            allocator->partNumber++;
		        }
		    }

		    if(allocator->partNumber == 0)
		    {
	            MSG_ERR(("Failed to find BPA2 partitions for UMP\n"));
	            kfree(allocator);
		    }

            return backend;
		}
		kfree(backend);
	}

	return NULL;
}



/*
 * Destroy specified dedicated memory backend
 */
static void bpa2_allocator_shutdown(ump_memory_backend * backend)
{
	bpa2_allocator * allocator;

	BUG_ON(!backend);
	BUG_ON(!backend->ctx);

	allocator = (bpa2_allocator*)backend->ctx;

	kfree(allocator);
	kfree(backend);
}


#define MB64    (64 * 1024 * 1024)

static int bpa2_allocator_allocate(void* ctx, ump_dd_mem * mem)
{
	bpa2_allocator * allocator = (bpa2_allocator*)ctx;
	int pageNumber;
	int part;

	BUG_ON(!ctx);
	BUG_ON(!mem);

	mem->block_array = (ump_dd_physical_block*)vmalloc(sizeof(ump_dd_physical_block));
	if (NULL == mem->block_array)
	{
		MSG_ERR(("Failed to allocate block array\n"));
		return 0;
	}

	pageNumber = (mem->size_bytes + (PAGE_SIZE -1)) / PAGE_SIZE;
    mem->block_array[0].size = pageNumber * PAGE_SIZE;

    if (down_interruptible(&allocator->mutex))
    {
        MSG_ERR(("Could not get mutex to do block_allocate\n"));
        return 0;
    }

    for(part = 0; part < allocator->partNumber; part++)
    {
        unsigned long addrs[100];
        int index;

        for(index = 0; index < sizeof(addrs) / sizeof(addrs[0]); index++)
        {
            mem->block_array[0].addr = bpa2_alloc_pages(allocator->partition[part].bpa2, pageNumber, 1, GFP_KERNEL);

            if(mem->block_array[0].addr == 0) // No more space in this partition, try other one
                break;

            if(mem->block_array[0].addr / MB64 != (mem->block_array[0].addr + mem->size_bytes) / MB64)
            {
                // We cross 64MB memory, free chunck and try to allocate a chunk from start to upper boundary
                int sizetoboundary = (mem->block_array[0].addr / MB64 + 1) * MB64 - mem->block_array[0].addr;

                bpa2_free_pages(allocator->partition[part].bpa2, mem->block_array[0].addr);
                mem->block_array[0].addr = 0;

                addrs[index] = bpa2_alloc_pages(allocator->partition[part].bpa2,
                        (sizetoboundary + (PAGE_SIZE -1)) / PAGE_SIZE, 1, GFP_KERNEL);

#if 0
                DBG_MSG(2, ("UMP'BPA2:: We cross 64MB memory, addr: [%x..%x[ sizetoboundary: %x\n",
                        mem->block_array[0].addr,
                        mem->block_array[0].addr + mem->block_array[0].size,
                        sizetoboundary));
                DBG_MSG(2, ("UMP'BPA2::     fake:  [%x..%x[\n", addrs[index], addrs[index]+sizetoboundary));
#endif
            }
            else
                break; // We have found a good chunk, exit
        }

        for(; index-- > 0; )
            bpa2_free_pages(allocator->partition[part].bpa2, addrs[index]);

        if(mem->block_array[0].addr != 0)
            break;
    }

    up(&allocator->mutex);

    if(mem->block_array[0].addr == 0)
    {
        MSG_ERR(("Could not find BPA2 memory for the allocation for %d\n", mem->size_bytes));
        vfree(mem->block_array);
        mem->block_array = NULL;
        return 0;
    }

    mem->nr_blocks = 1;
    //mem->is_cached=0;

    DBG_MSG(2, ("UMP'BPA2 +++ [%x : %x[ (%d)\n",
            mem->block_array[0].addr,
            mem->block_array[0].addr + mem->block_array[0].size,
            mem->block_array[0].size));

	return 1;
}



static void bpa2_allocator_release(void * ctx, ump_dd_mem * handle)
{
    bpa2_allocator * allocator = (bpa2_allocator*)ctx;
    int part;

	BUG_ON(!ctx);
	BUG_ON(!handle);

    DBG_MSG(2, ("UMP'BPA2 --- %x + %d\n", handle->block_array[0].addr, handle->block_array[0].size));

    if (down_interruptible(&allocator->mutex))
    {
        MSG_ERR(("Allocator release: Failed to get mutex - memory leak\n"));
        return;
    }

    // Found good partition and free !!
    for(part = 0; part < allocator->partNumber; part++)
        if(allocator->partition[part].base <= handle->block_array[0].addr &&
                handle->block_array[0].addr < allocator->partition[part].end)
        {
            bpa2_free_pages(allocator->partition[part].bpa2, handle->block_array[0].addr);
            break;
        }

    up(&allocator->mutex);

    vfree(handle->block_array);
    handle->block_array = NULL;
}

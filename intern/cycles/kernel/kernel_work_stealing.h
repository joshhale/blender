/*
 * Copyright 2011-2015 Blender Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __KERNEL_WORK_STEALING_H__
#define __KERNEL_WORK_STEALING_H__

CCL_NAMESPACE_BEGIN

/*
 * Utility functions for work stealing
 */

#ifdef __KERNEL_OPENCL__
#  pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#endif

/* Returns true if there is work */
ccl_device bool get_next_work(KernelGlobals *kg,
                              uint thread_index,
                              ccl_private uint *global_work_index)
{
	uint total_work_size = kernel_split_params.w
	                     * kernel_split_params.h
	                     * kernel_split_params.num_samples;

	/* With a small amount of work there may be more threads than work due to
	 * rounding up of global size, stop such threads immediately. */
	if(thread_index >= total_work_size) {
		return false;
	}

	/* Increase atomic work index counter in pool. */
	uint pool = thread_index / WORK_POOL_SIZE;
	uint work_index = atomic_fetch_and_inc_uint32(&kernel_split_params.work_pools[pool]);

	/* Map per-pool work index to a global work index. */
	uint global_size = ccl_global_size(0) * ccl_global_size(1);
	kernel_assert(global_size % WORK_POOL_SIZE == 0);
	kernel_assert(thread_index < global_size);

	*global_work_index = (work_index / WORK_POOL_SIZE) * global_size
	                   + (pool * WORK_POOL_SIZE)
	                   + (work_index % WORK_POOL_SIZE);

	/* Test if all work for this pool is done. */
	return (*global_work_index < total_work_size);
}

/* Map global work index to pixel X/Y and sample. */
ccl_device_inline void get_work_pixel(KernelGlobals *kg,
                                      uint global_work_index,
                                      ccl_private uint *x,
                                      ccl_private uint *y,
                                      ccl_private uint *sample)
{
	uint tile_pixels = kernel_split_params.w * kernel_split_params.h;
	uint sample_offset = global_work_index / tile_pixels;
	uint pixel_offset = global_work_index - sample_offset * tile_pixels;
	uint y_offset = pixel_offset / kernel_split_params.w;
	uint x_offset = pixel_offset - y_offset * kernel_split_params.w;

	*x = kernel_split_params.x + x_offset;
	*y = kernel_split_params.y + y_offset;
	*sample = kernel_split_params.start_sample + sample_offset;
}

CCL_NAMESPACE_END

#endif  /* __KERNEL_WORK_STEALING_H__ */

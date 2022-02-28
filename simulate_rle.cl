unsigned char get_value_rle(__local unsigned char *target_cache, __local unsigned short *target_rle_offsets, int x, int y, int gid){
	int yoffset = (int)target_rle_offsets[y];
	int i = 0;
	int streak = 0;
	// unsigned int small_cache;
	
	// TODO: read memory in 32-bit chunks (4 chars at a time), as this is memory bank size.
	// implement idea: make char array of 4 length, and populate at beginning of function.
	// Should be able to index as below, but without the yoffset.
	// Once we need the next set of 4 chars, we can fetch those values, and reset the i value / whatever offset we use?
	// small_cache = *((__local unsigned int *)target_cache + yoffset / 4);
	
	//if(gid == 0){
	//	printf("%d %d\n", (__local unsigned int *)target_cache + yoffset / 4, target_cache + yoffset);
	//	printf("%x %u %u %u\n", target_cache[yoffset], target_cache[yoffset + 1], target_cache[yoffset + 2], target_cache[yoffset + 3]);
	//	printf("%x %u %u %u\n", small_cache & 0xFFFF, (small_cache & 0x00F0) >> 4, (small_cache & 0x0F00) >> 8, (small_cache & 0xF000) >> 12);
	//}
	
	if(x == 0){
		return target_cache[yoffset + 1];
	}
	while(x >= 0){
		streak = (int)target_cache[yoffset + i];
		// If we reach the end of the row, then value must be from previous.
		if(streak == 0){
			return target_cache[yoffset + i - 1];
		}
		x -= streak;
		// i points to next streak value.
		i += 2;
	}
	return target_cache[yoffset + i - 1];
}

__kernel void fire(float radius, float damage_per_pellet, int num_pellets, float hs_mult,
    int randomness, int stepsize, int resolution, int iterations, int target_rle_length,
    __constant unsigned char *target_rle, __constant unsigned short *target_rle_offsets, __global float *result_buf, __global float *rand_buf){
    
	// Cache for random numbers
	// While one (a) is being read from, other (b) fetches new random numbers asynchronously.
	__local float rcache1[16];
	__local float rcache2[16];
	
	// Cache for target (rle), and rle offsets. 
	// Assigned to 24kB, and 4kB (should be safe enough)
	__local unsigned char target_cache[4400];
	__local unsigned short offset_cache[2048];

	
	int gid = get_global_id(0);
	int group_id = get_group_id(0);
	int group_size = get_local_size(0);
	int lid = get_local_id(0);

	int target_index = gid * stepsize;
											
	int targetx = target_index % resolution;
	int targety = (target_index / resolution) * stepsize;

	if(lid < 64){
		// Fill cache with rle target 
		for(int i = 0; i < target_rle_length; i+= 64){
			int j = lid + i;
			if(j < target_rle_length){
				target_cache[j] = target_rle[j];
			}
		}
	}

	if(lid < 32){
		// Fill offset cache too
		for(int i = 0; i < resolution; i+= 32){
			int j = lid + i;
			if(j < resolution){
				offset_cache[j] = target_rle_offsets[j];
			}
		}
	}
  int a_offset = iterations * num_pellets;

  float damage = 0;
  unsigned char hit;
	
	int j = 0;
	int num_floats = 16;
	
	while(j < a_offset){
		int cache_index = j % num_floats;
		if(cache_index == 0){
			if(lid < num_floats){
				rcache1[lid] = rand_buf[j + lid];
				rcache2[lid] = rand_buf[j + lid + a_offset];
			}
			barrier(CLK_LOCAL_MEM_FENCE);
		}
		
		float rand1 = rcache1[cache_index];
		float rand2 = rcache2[cache_index];
		
		float ang = 6.28318 * rand1;
		float rad = radius * rand2;
		int x = (int)(rad * cos(ang) + targetx);
		int y = (int)(rad * sin(ang) + targety);
		hit = 0;
		if( ( (x >= 0) && (x < resolution) ) && ( (y >= 0) && (y < resolution) )){
			hit = get_value_rle(target_cache, offset_cache, x, y, gid);

			if(hit > 0){
				switch(hit)
				{
					case 1:
						damage += damage_per_pellet;
						break;
					case 2:
						damage += (hs_mult * damage_per_pellet);
						break;
					default:
						damage += (0.9 * damage_per_pellet);
				}
			}
		}
	j++;
	}
	damage = damage / iterations;
	result_buf[gid] = damage;
}
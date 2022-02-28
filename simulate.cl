__kernel void fire(float radius, float damage_per_pellet, int num_pellets, float hs_mult,
    int randomness, int stepsize, int resolution, int iterations,
    __constant unsigned char *target_buf, __global float *result_buf, __global float *rand_buf){
    
	// TODO: not portable lolol
	__local float rcache1[16];
	__local float rcache2[16];
	
    int gid = get_global_id(0);
    int group_id = get_group_id(0);
    int group_size = get_local_size(0);
    int lid = get_local_id(0);    

    int target_index = gid * stepsize;
                        
    int targetx = target_index % resolution;
    int targety = (target_index / resolution) * stepsize;
                        
    int a_offset = iterations * num_pellets;
                        
    float damage = 0;
    unsigned char hit;
	
	int j = 0;
	int num_floats = 16;
	
    while(j < a_offset){
		int cache_index = j % num_floats;
		if( cache_index == 0){
			if(lid < num_floats){
				rcache1[lid] = rand_buf[j + lid];
				rcache2[lid] = rand_buf[j + lid + a_offset];
			}
		}
		barrier(CLK_GLOBAL_MEM_FENCE);
		float rand1 = rcache1[cache_index];
		float rand2 = rcache2[cache_index];
		
		float ang = 6.28318 * rand1;
		float rad = radius * rand2;
		int x = (int)(rad * cos(ang) + targetx);
		int y = (int)(rad * sin(ang) + targety);
		hit = 0;
		if( ( (x >= 0) && (x < resolution) ) && ( (y >= 0) && (y < resolution) )){
			hit = target_buf[x + y * resolution];
			if(hit != 0){
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
    barrier(CLK_GLOBAL_MEM_FENCE);
    result_buf[gid] = damage;
}
__kernel void fire(double radius, float damage_per_pellet, int num_pellets, float hs_mult,
    int randomness, int stepsize, int resolution, int iterations,
    __constant unsigned char *target_buf, __global double *result_buf, __global double *rand_buf){
    
	__local double group_seed;
	
    int gid = get_global_id(0);
    int group_id = get_group_id(0);
    int group_size = get_local_size(0);
    int lid = get_local_id(0);    
	
	int maxIndex = 2 * num_pellets * iterations * group_size * randomness;
	
	if(lid == 0){
		group_seed = rand_buf[group_id % maxIndex];
	}
	int rando = (int)(group_seed * group_size);
	
    barrier(CLK_LOCAL_MEM_FENCE);

    int target_index = gid * stepsize;
                        
    int targetx = target_index % resolution;
    int targety = (target_index / resolution) * stepsize;
                        
    int a_offset = 2 * group_size * iterations * num_pellets;
    int r_offset = (group_id % randomness) * a_offset;
                        
    double damage = 0;
    unsigned char hit;
    for(int j=0; j < iterations; j++){
		if(lid == 0){
			// hooooo boi.
			int new_index = ((int)(group_seed * maxIndex) + group_id + j) % maxIndex;
			group_seed = rand_buf[new_index];
		}
		barrier(CLK_LOCAL_MEM_FENCE);
		rando = (int)(group_seed * group_size);
		int joffset = j * 2 * num_pellets * group_size + r_offset;
		int rand_index = (lid + rando) % group_size;
		rand_index = 0;
		
		for(int i=0; i < num_pellets; i++){
            int ioffset = 2 * i * group_size + rand_index + joffset;
			double rand1 = rand_buf[ioffset];
			double rand2 = rand_buf[ioffset + group_size];
			
            double ang = 6.2831853071795864769 * rand1;
            double rad = radius * rand2;
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
        }
    }
    damage = damage / iterations;
    barrier(CLK_GLOBAL_MEM_FENCE);
    result_buf[gid] = damage;
}
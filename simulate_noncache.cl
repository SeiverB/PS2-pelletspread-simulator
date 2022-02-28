__kernel void fire(float radius, float damage_per_pellet, int num_pellets, float hs_mult,
    int randomness, int stepsize, int resolution, int iterations,
    __constant unsigned char *target_buf, __global float *result_buf, __global float *rand_buf){
    
	__local float rcache[16];
	
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
	
    for(int j=0; j < iterations; j++){
		for(int i=0; i < num_pellets; i++){
			int a = j * num_pellets + i;
			float rand1 = rand_buf[a];
			float rand2 = rand_buf[a + a_offset];
			
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
        }
    }
    damage = damage / iterations;
    barrier(CLK_GLOBAL_MEM_FENCE);
    result_buf[gid] = damage;
}
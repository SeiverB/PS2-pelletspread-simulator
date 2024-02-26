# Planetside 2 Pellet-Spread Simulator for in-game shotguns

This program serves to simulate the spread of shotgun pellets overtop of accurate hitboxes in order to determine the optimal aim-point, and expected maximum damage for a given range.
Since the pellets are randomly distributed about a radius, and there are different damage modifiers according to the hitbox hit, it is not trivial to determine without simulation.

This program is able to simulate thousands of shots for a given aim-point, which is then averaged, and written to an image heatmap, which gives players an idea of the approximate damage that will be done by aiming at a particular pixel.
The relevant stats from each in-game shotgun are accounted for, including, pellet count, damage per pellet, and the pelletspread radius

Currently, this program is optimized for running on my AMD Radeon 5700XT using OpenCL. Included are several implementations of the OpenCL Kernel using different compression methods in order to save shared memory for each workgroup.
If a statistical model were to be used for the pellet trajectories instead of simulating thousands of shots, this program could be sped up immensely.

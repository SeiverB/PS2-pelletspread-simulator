#include <iostream>
#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include <string>
#include <vector>
#include <ctime>
#include <math.h>
#include <bitmap_image.hpp>
#include <tinycolormap.hpp>
#include <random>

using namespace std;

// OpenCL
#define GROUP_SIZE 128
#define MAX_SOURCE_SIZE (0x100000)

// Target properties
#define TARGET_MISS (0x000000)
#define TARGET_HS (0x0000FF)
#define TARGET_LEGS (0x00FFFF)
#define TARGET_BODY (0xFFFFFF)
#define MODEL_HEIGHT 1.506

// BMP properties:
#define DATA_OFFSET_OFFSET 0x000A
#define WIDTH_OFFSET 0x0012
#define HEIGHT_OFFSET 0x0016
#define BITS_PER_PIXEL_OFFSET 0x001C
#define HEADER_SIZE 14
#define INFO_HEADER_SIZE 40
#define NO_COMPRESION 0
#define MAX_NUMBER_OF_COLORS 0
#define ALL_COLORS_REQUIRED 0

// 0 is rle, 1 for others
#define KERNEL 0

#define PI 3.14159265358979323846

#define MAXRAND 4294967295

typedef unsigned int int32;
typedef short int16;
typedef unsigned char byte;

mt19937 gen32;

// Generate random float from 0 to 1.
float randZeroToOne() {
    float r = ((float)gen32() / (float)MAXRAND);
    return r;
}

const char* getErrorString(cl_int error)
{
    switch (error) {
        // run-time and JIT compiler errors
    case 0: return "CL_SUCCESS";
    case -1: return "CL_DEVICE_NOT_FOUND";
    case -2: return "CL_DEVICE_NOT_AVAILABLE";
    case -3: return "CL_COMPILER_NOT_AVAILABLE";
    case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case -5: return "CL_OUT_OF_RESOURCES";
    case -6: return "CL_OUT_OF_HOST_MEMORY";
    case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case -8: return "CL_MEM_COPY_OVERLAP";
    case -9: return "CL_IMAGE_FORMAT_MISMATCH";
    case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case -11: return "CL_BUILD_PROGRAM_FAILURE";
    case -12: return "CL_MAP_FAILURE";
    case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
    case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
    case -15: return "CL_COMPILE_PROGRAM_FAILURE";
    case -16: return "CL_LINKER_NOT_AVAILABLE";
    case -17: return "CL_LINK_PROGRAM_FAILURE";
    case -18: return "CL_DEVICE_PARTITION_FAILED";
    case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

        // compile-time errors
    case -30: return "CL_INVALID_VALUE";
    case -31: return "CL_INVALID_DEVICE_TYPE";
    case -32: return "CL_INVALID_PLATFORM";
    case -33: return "CL_INVALID_DEVICE";
    case -34: return "CL_INVALID_CONTEXT";
    case -35: return "CL_INVALID_QUEUE_PROPERTIES";
    case -36: return "CL_INVALID_COMMAND_QUEUE";
    case -37: return "CL_INVALID_HOST_PTR";
    case -38: return "CL_INVALID_MEM_OBJECT";
    case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case -40: return "CL_INVALID_IMAGE_SIZE";
    case -41: return "CL_INVALID_SAMPLER";
    case -42: return "CL_INVALID_BINARY";
    case -43: return "CL_INVALID_BUILD_OPTIONS";
    case -44: return "CL_INVALID_PROGRAM";
    case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
    case -46: return "CL_INVALID_KERNEL_NAME";
    case -47: return "CL_INVALID_KERNEL_DEFINITION";
    case -48: return "CL_INVALID_KERNEL";
    case -49: return "CL_INVALID_ARG_INDEX";
    case -50: return "CL_INVALID_ARG_VALUE";
    case -51: return "CL_INVALID_ARG_SIZE";
    case -52: return "CL_INVALID_KERNEL_ARGS";
    case -53: return "CL_INVALID_WORK_DIMENSION";
    case -54: return "CL_INVALID_WORK_GROUP_SIZE";
    case -55: return "CL_INVALID_WORK_ITEM_SIZE";
    case -56: return "CL_INVALID_GLOBAL_OFFSET";
    case -57: return "CL_INVALID_EVENT_WAIT_LIST";
    case -58: return "CL_INVALID_EVENT";
    case -59: return "CL_INVALID_OPERATION";
    case -60: return "CL_INVALID_GL_OBJECT";
    case -61: return "CL_INVALID_BUFFER_SIZE";
    case -62: return "CL_INVALID_MIP_LEVEL";
    case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
    case -64: return "CL_INVALID_PROPERTY";
    case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
    case -66: return "CL_INVALID_COMPILER_OPTIONS";
    case -67: return "CL_INVALID_LINKER_OPTIONS";
    case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

        // extension errors
    case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
    case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
    case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
    case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
    case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
    case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
    default: return "Unknown OpenCL error";
    }
}


int saveHeatmap(const char* fileName, float* values, int size, const char* format, int maxvalue) {
    bitmap_image image(size, size);
    image.clear();

    // probably not the most efficient solution, but it is keeping me from losing my hair
    int index = 0;
    for (unsigned int x = 0; x < image.height(); x++){
        for (unsigned int y = 0; y < image.width(); y++) {
            float value = values[index] / maxvalue;
            tinycolormap::Color mapped = tinycolormap::GetColor(value, tinycolormap::ColormapType::Magma);
            unsigned char r = (unsigned char)(mapped.r() * 255);
            unsigned char g = (unsigned char)(mapped.g() * 255);
            unsigned char b = (unsigned char)(mapped.b() * 255);
            image.set_pixel(y, x, r, g, b);
            index += 1;
        }
    }
    image.save_image(fileName);
    return 0;
}

// Bitmap open function, courtesy of Alejandro Rodriguez: https://elcharolin.wordpress.com/2018/11/28/read-and-write-bmp-files-in-c-c/
int LoadBMP(const char *fileName, byte **pixels) {
    FILE* imageFile = fopen(fileName, "rb");
    int32 width = 0;
    int32 height = 0;
    int32 dataOffset = 0;
    int32 bytesPerPixel = 0;
    fseek(imageFile, DATA_OFFSET_OFFSET, SEEK_SET);
    fread(&dataOffset, 4, 1, imageFile);
    fseek(imageFile, WIDTH_OFFSET, SEEK_SET);
    fread(&width, 4, 1, imageFile);
    fseek(imageFile, HEIGHT_OFFSET, SEEK_SET);
    fread(&height, 4, 1, imageFile);
    int16 bitsPerPixel;
    fseek(imageFile, BITS_PER_PIXEL_OFFSET, SEEK_SET);
    fread(&bitsPerPixel, 2, 1, imageFile);
    bytesPerPixel = ((int32)bitsPerPixel) / 8;

    int paddedRowSize = (int)(4 * ceil((float)(width) / 4.0f)) * (bytesPerPixel);
    int unpaddedRowSize = (width) * (bytesPerPixel);
    int totalSize = unpaddedRowSize * (height);
    *pixels = (byte*)malloc(totalSize);
    int i = 0;
    byte* currentRowPointer = *pixels + ((height - 1) * unpaddedRowSize);
    for (int32 i = 0; i < height; i++)
    {
        fseek(imageFile, dataOffset + (i * paddedRowSize), SEEK_SET);
        fread(currentRowPointer, 1, unpaddedRowSize, imageFile);
        currentRowPointer -= unpaddedRowSize;
    }
    fclose(imageFile);

    // Flip bit order, courtesy of: https://stackoverflow.com/questions/9296059/read-pixel-value-in-bmp-file
    for (i = 0; i < totalSize; i += 3) {
        byte tmp = (*pixels)[i];
        (*pixels)[i] = (*pixels)[i + 2];
        (*pixels)[i + 2] = tmp;
    }
    return width;
}


class Target {
public:
    int m_targetdim, m_targetlen, m_target_rle_length;
    unsigned char* m_target;
    unsigned char* m_target_rle;
    unsigned short* m_target_rle_offsets;

    // Target must be square
    // loadedtarget is pointer to raw pixels loaded from bmp. targetdim is the resolution of x or y (square img) eg: 512
    Target(byte* loadedtarget, int targetdim) {
        
        m_targetdim = targetdim;
        m_targetlen = targetdim * targetdim;
        m_target = (unsigned char*) malloc(sizeof(unsigned char) * (m_targetlen));
        // allocate compressed rle array to worst case so there cannot be data leaks
        m_target_rle = (unsigned char*)malloc(sizeof(unsigned char) * (m_targetlen));
        // takes row index --> offset in bytes (where row is actually located within m_target_rle)
        m_target_rle_offsets = (unsigned short*)malloc(sizeof(unsigned short) * (m_targetdim));

        // Convert from RGB bytes to body region type (char).
        for (int i = 0; i < m_targetlen; i++) {
            // loadedtarget's length is 3x m_targetlen as it holds 3 bytes per pixel
            int l_index = 3 * i;
            unsigned int rgb = (loadedtarget[l_index]) << 8;    // Red value
            rgb = (rgb + loadedtarget[l_index + 1]) << 8;       // Green value
            rgb = (rgb + loadedtarget[l_index + 2]);            // Blue value

            switch (rgb) {
            case TARGET_MISS:
                m_target[i] = 0;
                break;
            case TARGET_BODY:
                m_target[i] = 1;
                break;
            case TARGET_HS:
                m_target[i] = 2;
                break;
            case TARGET_LEGS:
                m_target[i] = 3;
                break;
            default:
                cout << "\nInvalid colour found while converting to body regions...\n";
                exit(1);
            }
        }

        // Compress body region type into RLE rows.
        unsigned char dtype = 255;
        unsigned char streak = 1;
        this->m_target_rle_offsets[0] = 0;
        // a is the current position within RLE array (byte offset)
        unsigned short a = 0;
        for (int i = 0; i <= m_targetlen; i++) {
            int j = i % m_targetdim;
            // If we reach the end of a row, write last streak, and value, plus the stoprow value
            // Save byte offset to seperate list
            // assign type to next value, reset streak counter.
            if (j == 0) {
                // Only write if our last streak is not null
                if (dtype != 255) {
                    this->m_target_rle[a] = streak;
                    this->m_target_rle[a + 1] = dtype;
                    this->m_target_rle[a + 2] = 0;
                    a += 3;
                    // save byte offset
                    this->m_target_rle_offsets[i / m_targetdim] = a;
                    // If at end of list, add additional 0.
                    if (i == m_targetlen) {
                        this->m_target_rle[a] = 0;
                    }
                }
                dtype = this->m_target[i];
                streak = 1;
            }
            // if we have reached maximum streak value, or streak is broken, write current streak/type to m_target_rle
            // assign dtype to new value.
            else if (streak == 255 || this->m_target[i] != dtype) {
                this->m_target_rle[a] = streak;
                this->m_target_rle[a + 1] = dtype;
                a += 2;
                dtype = this->m_target[i];
                streak = 1;
            }
            // Continue Streak
            else {
                streak++;
            }
        
        }
        printf("RLE Compressed target size: %d\n", a);
        m_target_rle_length = a;
        // debug, print out formatted rle.
        int j = 0;
        for (int i = 0; i < m_targetdim; i++) {
            printf("%d: ", i);
            streak = 1;
            dtype = 255;
            while (true) {
                streak = this->m_target_rle[j];
                dtype = this->m_target_rle[j + 1];
                if (streak == 0) {
                    printf("[%d]\n", this->m_target_rle_offsets[i], (unsigned char)m_target_rle[this->m_target_rle_offsets[i]], (unsigned char)m_target_rle[this->m_target_rle_offsets[i] + 2]);
                    break;
                }
                else {
                    printf("%u %u ", streak, dtype);
                }
                j += 2;
            }
            if (dtype == 0) {
                break;
            }
            j += 1;
        }
    }
    unsigned char& operator[](int i) {
        if (i >= m_targetlen) {
            cout << "Index out of bounds" << endl;
            exit(1);
        }
        return m_target[i];
    }

};

class Weapon {
public:
    char* name;
    int max_damage, min_damage, max_damage_range, min_damage_range, num_pellets;
    float* acc;
    float* base_stats;
    float hs_mult, pellet_spread;

public:
    Weapon(char* weapon_name, float* weapon_base_stats, float* acc_tuple, int* damage_model) {
        name = _strdup(weapon_name);
        // TODO: this shouldn't be hardcoded...
        acc = (float*)malloc(8 * sizeof(float));
        base_stats = (float*)malloc(4 * sizeof(float));

        if ((acc == 0) || (base_stats == 0)) {
            cout << "Failed to allocate memory for Weapon";
            exit(1);
        }
        // TODO: again, kinda hardcoded.
        memcpy(acc, acc_tuple, 8 * sizeof(float));
        memcpy(base_stats, weapon_base_stats, 4 * sizeof(float));

        max_damage = damage_model[0];
        min_damage = damage_model[1];
        max_damage_range = damage_model[2];
        min_damage_range = damage_model[3];
        hs_mult = base_stats[1];
        num_pellets = (int)base_stats[2];
        pellet_spread = base_stats[3];

    }
};


class Simulation {
private:
    Target* m_target;
    int m_stepsize, m_iterations, m_randomness;
    char * m_output_type;
    cl_context m_context;
    cl_program m_program;
    cl_platform_id m_default_platform;
    cl_command_queue m_queue;
    cl_device_id m_device;
    cl_kernel m_kernel_fire;

public:
    Simulation(Target* target, const char * kernelpath, int stepsize, int iterations, int randomness, const char * output_type) {
        m_target = target;
        m_stepsize = stepsize;
        m_iterations = iterations;
        m_randomness = randomness;

        char * kernel_path = _strdup(kernelpath);
        m_output_type = _strdup(output_type);

        // Load kernel source code into m_kernel
        FILE* fp;
        char* m_kernel;
        size_t m_kernel_size;

        fp = fopen(kernel_path, "r");
        if (!fp) {
            cout << "\nKernel Source Not Found\n";
            exit(1);
        }
        m_kernel = (char*)malloc(MAX_SOURCE_SIZE);
        m_kernel_size = fread(m_kernel, 1, MAX_SOURCE_SIZE, fp);
        fclose(fp);

        // opencl example code adapted from: https://github.com/Dakkers/OpenCL-examples/blob/master/example00/main.cpp#L5
    
        cl_platform_id all_platforms[4];
        cl_uint num_platforms = 0;

        cl_int success = clGetPlatformIDs(2, all_platforms, &num_platforms);
        
        if (success != CL_SUCCESS) {
            cout << " Failed to allocate resources for OpenCL implementation.\n";
            exit(1);
        }

        if (num_platforms == 0) {
            cout << " No platforms found. Check OpenCL installation!\n";
            exit(1);
        }

        m_default_platform = all_platforms[0];
        char platform_name[128];

        clGetPlatformInfo(m_default_platform, CL_PLATFORM_NAME, 128, platform_name, NULL);

        cout << "Using platform: " << platform_name << "\n";

        // get default device (CPUs, GPUs) of the default platform

        cl_device_id devices[3];
        success = clGetDeviceIDs(m_default_platform, CL_DEVICE_TYPE_GPU, 3, devices, NULL);

        if (success != CL_SUCCESS) {
            cout << " An error occured while finding an OpenCL device.\n";
            exit(1);
        }
        
        m_device = devices[0];
        char device_name[128];
        char device_vendor[128];

        clGetDeviceInfo(m_device, CL_DEVICE_NAME, 128, (void*)device_name, NULL);
        clGetDeviceInfo(m_device, CL_DEVICE_VENDOR, 128, (void*)device_vendor, NULL);

        cout << "Using device: " << device_name << " From vendor: " << device_vendor << "\n";

        // a context is like a "runtime link" to the device and platform;
        // i.e. communication is possible
        const cl_context_properties properties[3] = {CL_CONTEXT_PLATFORM, (cl_context_properties)m_default_platform, 0};
        cl_int errorcode = 0;
        m_context = clCreateContext(properties, 1, devices, NULL, NULL, &errorcode);
        if (errorcode != CL_SUCCESS) {
            cout << "Failed to create OpenCL Context\n";
            exit(1);
        }

        // create the program that we want to execute on the device
        const char* programs[1] = { m_kernel };
        const size_t lengths[1] = { m_kernel_size };
        m_program = clCreateProgramWithSource(m_context, 1, programs, lengths, &errorcode);
        
        if (errorcode != CL_SUCCESS) {
            cout << "Failed to create OpenCL Program\n";
            exit(1);
        }

        success = clBuildProgram(m_program, NULL, NULL, NULL, NULL, NULL);
        if (success != CL_SUCCESS) {
            cout << "Failed to build OpenCL Program: " << getErrorString(success) << endl;
            exit(1);
        }

        m_kernel_fire = clCreateKernel(m_program, "fire", &errorcode);
        if (errorcode != CL_SUCCESS) {
            cout << "Failed to create Kernel from OpenCL Program\n";
            exit(1);
        }


        m_queue = clCreateCommandQueueWithProperties(m_context, m_device, NULL, &errorcode);
        if (errorcode != CL_SUCCESS) {
            cout << "Failed to create OpenCL Command Queue\n";
            exit(1);
        }

    }
    void simulate_weapon(Weapon* weapon, int distance_start, int distance_stop) {
        Weapon wep = *weapon;
        printf("Starting simulation...\n");
        // int num_rand = 2 * wep.num_pellets * m_iterations * GROUP_SIZE * m_randomness;
        int num_rand = 2 * wep.num_pellets * m_iterations * m_randomness;
        float* h_rand;
        printf("Allocating memory for random numbers...\n");
        h_rand = (float*)malloc(sizeof(float)*num_rand);

        if (h_rand == 0) {
            cout << "Could not allocate enough space for random-number array\n";
            exit(1);
        }
        printf("Generating random numbers...\n");
        for (int i = 0; i < num_rand; i++) {
            h_rand[i] = randZeroToOne();
        }
        cl_int errorcode = 0;
        int d_rand_size = num_rand * sizeof(float);
        printf("Moving random number matrix to device memory...\n");
        cl_mem d_rand = clCreateBuffer(m_context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, d_rand_size, (void*)h_rand, &errorcode);
        if (errorcode != CL_SUCCESS) {
            cout << "Failed to create random number buffer on device.\n";
            exit(1);
        }
        printf("Moving target to device memory...\n");
        #if KERNEL == 0
            int d_target_size = (*m_target).m_target_rle_length * sizeof(unsigned char);
            cl_mem d_target = clCreateBuffer(m_context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, d_target_size, (*m_target).m_target_rle, &errorcode);
        #else
            int d_target_size = (*m_target).m_targetlen * sizeof(unsigned char);
            cl_mem d_target = clCreateBuffer(m_context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, d_target_size, (*m_target).m_target, &errorcode);
        #endif

        if (errorcode != CL_SUCCESS) {
            cout << "Failed to create RLE target buffer on device.\n";
            exit(1);
        }

        int d_rle_offsets_size = (*m_target).m_targetdim * sizeof(unsigned short);
        cl_mem d_rle_offsets = clCreateBuffer(m_context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, d_rle_offsets_size, this->m_target->m_target_rle_offsets, &errorcode);
        
        if (errorcode != CL_SUCCESS) {
            cout << "Failed to create RLE target offset buffer on device.\n";
            exit(1);
        }

        // Create buffer on device for result
        printf("Creating results buffer on device...\n");
        int d_result_size = (*m_target).m_targetlen * sizeof(float);
        cl_mem d_result = clCreateBuffer(m_context, CL_MEM_WRITE_ONLY, d_result_size, NULL, NULL);
        
        // Allocate memory on host for result
        printf("Allocating memory on host for result...\n");
        float* h_result;
        h_result = (float*)malloc(d_result_size * sizeof(float));

        size_t global_work_size[] = { (size_t)(*m_target).m_targetlen };
        size_t local_work_size[] = { GROUP_SIZE };
        cl_event event;
        

        float damage_per_pellet;
        float m = (float)(wep.max_damage - wep.min_damage) / (float)(wep.max_damage_range - wep.min_damage_range);
        int num_pellets = wep.num_pellets;
        float hs_mult = wep.hs_mult;
        int randomness = this->m_randomness;
        int stepsize = this->m_stepsize;
        int resolution = this->m_target->m_targetdim;
        int iterations = this->m_iterations;
        int target_rle_length = this->m_target->m_target_rle_length;

        for (int distance = distance_start; distance < distance_stop; distance++) {
            printf("Simulating Weapon at distance: %d\n", distance);
            // Pellet spread radius in m
            float radius = tan(wep.pellet_spread * (PI / 180) * distance);
            // Convert pellet spread to pixels
            radius = ((float)resolution / MODEL_HEIGHT) * radius;

            if (distance <= wep.max_damage_range) {
                damage_per_pellet = (float)wep.max_damage;
            }
            else if (distance <= wep.min_damage_range) {
                damage_per_pellet = (float)wep.max_damage + m * (distance - wep.max_damage_range);
            }
            else {
                damage_per_pellet = (float)wep.min_damage;
            }

            errorcode = clSetKernelArg(m_kernel_fire, 0, sizeof(radius), &radius);
            if (errorcode != CL_SUCCESS) {
                cout << "Error While setting Kernel Arg0: " << getErrorString(errorcode) << endl;
            }
            errorcode = clSetKernelArg(m_kernel_fire, 1, sizeof(damage_per_pellet), &damage_per_pellet);
            if (errorcode != CL_SUCCESS) {
                cout << "Error While setting Kernel Arg1: " << getErrorString(errorcode) << endl;
            }
            errorcode = clSetKernelArg(m_kernel_fire, 2, sizeof(num_pellets), &num_pellets);
            if (errorcode != CL_SUCCESS) {
                cout << "Error While setting Kernel Arg2: " << getErrorString(errorcode) << endl;
            }
            errorcode = clSetKernelArg(m_kernel_fire, 3, sizeof(hs_mult), &hs_mult);
            if (errorcode != CL_SUCCESS) {
                cout << "Error While setting Kernel Arg3: " << getErrorString(errorcode) << endl;
            }
            errorcode = clSetKernelArg(m_kernel_fire, 4, sizeof(randomness), &randomness);
            if (errorcode != CL_SUCCESS) {
                cout << "Error While setting Kernel Arg4: " << getErrorString(errorcode) << endl;
            }
            errorcode = clSetKernelArg(m_kernel_fire, 5, sizeof(stepsize), &stepsize);
            if (errorcode != CL_SUCCESS) {
                cout << "Error While setting Kernel Arg5: " << getErrorString(errorcode) << endl;
            }
            errorcode = clSetKernelArg(m_kernel_fire, 6, sizeof(resolution), &resolution);
            if (errorcode != CL_SUCCESS) {
                cout << "Error While setting Kernel Arg6: " << getErrorString(errorcode) << endl;
            }
            errorcode = clSetKernelArg(m_kernel_fire, 7, sizeof(iterations), &iterations);
            if (errorcode != CL_SUCCESS) {
                cout << "Error While setting Kernel Arg7: " << getErrorString(errorcode) << endl;
            }
#if KERNEL == 0
                errorcode = clSetKernelArg(m_kernel_fire, 8, sizeof(target_rle_length), &target_rle_length);
                if (errorcode != CL_SUCCESS) {
                    cout << "Error While setting Kernel Arg8: " << getErrorString(errorcode) << endl;
                }
                errorcode = clSetKernelArg(m_kernel_fire, 9, sizeof(d_target), &d_target);
                if (errorcode != CL_SUCCESS) {
                    cout << "Error While setting Kernel Arg9: " << getErrorString(errorcode) << endl;
                }
                errorcode = clSetKernelArg(m_kernel_fire, 10, sizeof(d_rle_offsets), &d_rle_offsets);
                if (errorcode != CL_SUCCESS) {
                    cout << "Error While setting Kernel Arg10: " << getErrorString(errorcode) << endl;
                }
                errorcode = clSetKernelArg(m_kernel_fire, 11, sizeof(d_result), &d_result);
                if (errorcode != CL_SUCCESS) {
                    cout << "Error While setting Kernel Arg11: " << getErrorString(errorcode) << endl;
                }
                errorcode = clSetKernelArg(m_kernel_fire, 12, sizeof(d_rand), &d_rand);
                if (errorcode != CL_SUCCESS) {
                    cout << "Error While setting Kernel Arg12: " << getErrorString(errorcode) << endl;
                }
#else
                errorcode = clSetKernelArg(m_kernel_fire, 8, sizeof(d_target), &d_target);
                if (errorcode != CL_SUCCESS) {
                    cout << "Error While setting Kernel Arg8: " << getErrorString(errorcode) << endl;
                }
                errorcode = clSetKernelArg(m_kernel_fire, 9, sizeof(d_result), &d_result);
                if (errorcode != CL_SUCCESS) {
                    cout << "Error While setting Kernel Arg9: " << getErrorString(errorcode) << endl;
                }
                errorcode = clSetKernelArg(m_kernel_fire, 10, sizeof(d_rand), &d_rand);
                if (errorcode != CL_SUCCESS) {
                    cout << "Error While setting Kernel Arg10: " << getErrorString(errorcode) << endl;
                }
#endif  

                for (int i = 0; i < 1; i++) {
                    time_t itime = time(NULL);

                    errorcode = clEnqueueNDRangeKernel(m_queue, m_kernel_fire, 1, NULL, global_work_size, local_work_size, NULL, NULL, &event);
                    if (errorcode != CL_SUCCESS) {
                        cout << "Kernel Launch failed, with error: " << getErrorString(errorcode) << endl;
                        exit(1);
                    }

                    cl_event event_list[1] = { event };
                    clWaitForEvents(1, event_list);

                    clReleaseEvent(event);

                    printf("Time Elapsed: %f \n", (float)(time(NULL) - itime));
                }
                printf("Getting results from device memory...\n");
                errorcode = clEnqueueReadBuffer(m_queue, d_result, CL_TRUE, 0, d_result_size, h_result, 0, NULL, NULL);
                if (errorcode != CL_SUCCESS) {
                    cout << "Failed to get results: " << getErrorString(errorcode) << endl;
                    exit(1);
                }
             

            float* max = max_element(h_result, &h_result[m_target->m_targetlen]);

            char outputstr[128];
            sprintf(outputstr, "%s, %d %.0f %d.bmp", wep.name, distance, *max, iterations);
            printf("Saving result to file...\n");
            saveHeatmap(outputstr, h_result, resolution, "heatmap", 1000);
        }
        
        errorcode = clReleaseMemObject(d_target);
        errorcode = clReleaseMemObject(d_result);
        errorcode = clReleaseMemObject(d_rand);
        if (errorcode != CL_SUCCESS) {
            cout << "Could not free memory objects";
            exit(1);
        }
        printf("Calling clFinish...\n");
        errorcode = clFinish(m_queue);
        if (errorcode != CL_SUCCESS) {
            cout << "Could not release finish: " << getErrorString(errorcode) << endl;
            exit(1);
        }
        errorcode = clReleaseContext(m_context);
        printf("Realeasing opencl context...\n");
        if (errorcode != CL_SUCCESS) {
            cout << "Could not release context: " << getErrorString(errorcode) << endl;
            exit(1);
        }

    }
};

Weapon** getWeapons(int num_weapons) {
    Weapon** weapons;
    weapons = (Weapon**)malloc(sizeof(Weapon*) * num_weapons);
    if (weapons == 0) {
        cout << "Could not allocate memory when creating weapon\n";
        exit(1);
    }
    float bj_basestats[4] = { 71, 1.5, 11, 3.5 };
    float bj_acc_tuple[8] = {1.f, 1.5f, 1.5f, 2.f, 0.1f, 0.35f, 0.1f, 0.9f};
    int bj_damage_model[4] = { 125, 50, 8, 18 };
    char bj_name[] = "Tas-16 Blackjack";
    Weapon* blackjack = new Weapon(bj_name, bj_basestats, bj_acc_tuple, bj_damage_model);
    weapons[0] = blackjack;

    return weapons;
}

int main() {
    char basepath[] = "C:/Users/syver/Desktop/planetside_thing";
    char kernelname[] = "simulate.cl";

    // Target parameters
    char targetname[] = "target_v2.bmp";

    // Add full path to target name
    char targetpath[128];
    sprintf(targetpath, "%s/%s", basepath, targetname);

    //char kernelpath[128];
    //sprintf(kernelpath, "%s/%s", basepath, kernelname);
    char kernelpath[] = "C:/Users/syver/Desktop/planetside_thing/c_version/PS2Shotgun_cpp/Project1/simulate_rle.cl";

    byte* pixels;
    int width = LoadBMP(targetpath, &pixels);

    Target target = Target(pixels, width);

    // Simulation parameters
    int iterations = 10000;
    int stepsize = 1;
    int randomness = 1; 
    char output_type[] = "heatmap";

    Simulation simulation = Simulation(&target, kernelpath, stepsize, iterations, randomness, output_type);

    Weapon** weapons = getWeapons(1);
    Weapon a = *weapons[0];

    simulation.simulate_weapon(weapons[0], 6, 7);

    return 0;
 }

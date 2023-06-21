#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>


#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILLITY 1

#include <CL/opencl.hpp>

static const int DATA_SIZE = 1000;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <xclbin>" << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<cl::Device> devices;
    cl::Device device;
    std::vector<cl::Platform> platforms;
    bool found_device = false;

    cl::Platform::get(&platforms);

    for (size_t i = 0; (i < platforms.size()) & (found_device == false); i++) {
        cl::Platform platform = platforms[i];
        std::string platform_name = platform.getInfo<CL_PLATFORM_NAME>();
        if (platform_name == "Xilinx") {
            devices.clear();
            platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices);
            if (devices.size()) {
                device = devices[0];
                found_device = true;
                break;
            }
        }
    }

    if (found_device == false) {
        std::cout << "Error: Unable to find Target Device" << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        return EXIT_FAILURE;
    }

    cl::Context context(device);
    char* xclbin_filename = argv[1];
    std::cout << "Loading: " << xclbin_filename << "\n";
    std::ifstream bin_file(xclbin_filename, std::ifstream::binary);
    bin_file.seekg(0, bin_file.end);
    unsigned nb = bin_file.tellg();
    bin_file.seekg(0, bin_file.beg);
    char* buf = new char[nb];
    bin_file.read(buf, nb);

    cl::Program::Binaries bins;
    bins.push_back({buf, nb});
    devices.resize(1);
    cl::Program program(context, devices, bins);

    size_t size_in_bytes = DATA_SIZE * sizeof(int);

    cl::Buffer buffer_in(context, CL_MEM_READ_ONLY, size_in_bytes);
    cl::Buffer buffer_out(context, CL_MEM_WRITE_ONLY, size_in_bytes);

    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE);
    int* ptr_in = (int *)q.enqueueMapBuffer(buffer_in, CL_TRUE, CL_MAP_WRITE, 0, size_in_bytes);
    int* ptr_out = (int *)q.enqueueMapBuffer(buffer_out, CL_TRUE, CL_MAP_READ, 0, size_in_bytes);

    for (int i = 0; i < DATA_SIZE; i++) {
        ptr_in[i] = i;
    }

    unsigned int sec;
    int nsec;
    double d_sec;

    std::timespec start_time, end_time;
    clock_gettime(CLOCK_REALTIME, &start_time);

    cl::Kernel kernel(program, "kernel_simple");
    int narg = 0;
    kernel.setArg(narg++, buffer_in);
    kernel.setArg(narg++, buffer_out);
    kernel.setArg(narg++, DATA_SIZE);

    q.enqueueMigrateMemObjects({buffer_in}, 0);
    q.enqueueTask(kernel);
    q.enqueueMigrateMemObjects({buffer_out}, CL_MIGRATE_MEM_OBJECT_HOST);
    q.finish();

    clock_gettime(CLOCK_REALTIME, &end_time);
    sec = end_time.tv_sec - start_time.tv_sec;
    nsec = end_time.tv_nsec - start_time.tv_nsec;
    d_sec = (double)sec + (double)nsec / (1000 * 1000 * 1000);

    for (int i = 0; i < DATA_SIZE; i++) {
        std::cout << "input[" << i << "] : " << ptr_in[i] << ", result[" << i << "] : " << ptr_out[i] << std::endl;
    }
    
    q.enqueueUnmapMemObject(buffer_in, ptr_in);
    q.enqueueUnmapMemObject(buffer_out, ptr_out);
    q.finish();

    std::cout << "time : " << d_sec << std::endl;
    
    return 0;
}

#define DATA_SIZE 1000

extern "C" {
    void kernel_multi(const int* input, int* output, int size) {
        Multi: for (int i = 0; i < size; i++) {
            output[i] = input[i] * 2;
        }
        
    }

    void kernel_add(const int* input, int* output, int size) {
        Add: for (int i = 0; i < size; i++) {
            output[i] = input[i] + 10;
        }
    }
 
    void kernel_simple(const int* input, int* output, int size) {
        int tmp[DATA_SIZE];
        kernel_add(input, tmp, size);
        kernel_multi(tmp, output, size);
    }
}

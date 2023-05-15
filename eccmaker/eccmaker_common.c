#include <stdio.h>
#include "eccmaker_common.h"


/*
 * Generate decode matrix from encode matrix and erasure list
 * 
 * Note: This function is not thread safe because it uses a static variable to allocate memory only once
 * 
 *
 */

int gf_gen_decode_matrix_simple(
                    const u8 * encode_matrix,
                    const u8 * frag_err_list,
                    u8 * decode_matrix, // output matrix, modified by function
                    u8 * decode_index,  // output matrix, modified by function
                    int nerrs,
                    int k,
                    int m)
{
    // These are preserved between function calls so they must all be static
    static int first_run = 1;
    static u8 *temp_matrix = NULL;
    static u8 *invert_matrix = NULL;
    if (first_run == 1) { // first run, so allocate the memory just for the first time
        first_run = 0;
        temp_matrix = malloc(m * k); // allocated only once
        invert_matrix = malloc(m * k); // allocated only once
        puts("============================================ Allocating memory for temp matrix and invert matrix");
    }
    if (temp_matrix == NULL || invert_matrix == NULL) {
        printf("Error! Failed to allocate temp or invert matrix\n");
        exit(-1);
    }





    u8 frag_in_err[MMAX];
    int i, j, r;
    int nsrcerrs = 0;
    u8 s;

    memset(frag_in_err, 0, sizeof(frag_in_err));

    // Order the fragments in erasure for easier sorting
    for (i = 0; i < nerrs; i++) {
        if (frag_err_list[i] < k)
            nsrcerrs++;
        frag_in_err[frag_err_list[i]] = 1;
    }

    // Construct temp_matrix (matrix that encoded remaining frags) by removing erased rows
    for (i = 0, r = 0; i < k; i++, r++) {
        while (frag_in_err[r])
            r++;
        for (j = 0; j < k; j++)
            temp_matrix[k * i + j] = encode_matrix[k * r + j];
        decode_index[i] = r;
    }

    // Invert matrix to get recovery matrix
    if (gf_invert_matrix(temp_matrix, invert_matrix, k) < 0)
        return -1;

    // Get decode matrix with only wanted recovery rows
    for (i = 0; i < nerrs; i++) {
        if (frag_err_list[i] < k)	// A src err
            for (j = 0; j < k; j++)
                decode_matrix[k * i + j] =
                    invert_matrix[k * frag_err_list[i] + j];
    }

    // For non-src (parity) erasures need to multiply encode matrix * invert
    for (int p = 0; p < nerrs; p++) {
        if (frag_err_list[p] >= k) {	// A parity err
            for (i = 0; i < k; i++) {
                s = 0;
                for (j = 0; j < k; j++)
                    s ^= gf_mul(invert_matrix[j * k + i],
                            encode_matrix[k * frag_err_list[p] + j]);
                decode_matrix[k * p + i] = s;
            }
        }
    }
    return 0;
}


const unsigned char generate_byte(const unsigned char upper_bound){ // generates numbers in the range [0, upper_bound] - all inclusive!
    // I have thoroughly tested this function and informally proved that it is correct to my satisfaction.
    // this function will reject up to half of all generated numbers
    // this is a bit inefficient, but is ok when speed doesn't matter
    // you can fairly easily extend this to more than 255 by interpreting multiple bytes as 32 or 64 bit integer
    // you can also make a batch version, which would be faster
    const int n = 256; // the number of numbers in our input space
    assert(upper_bound < n); // the biggest number we can generate, given our input range
    const int range = upper_bound + 1; // the number of numbers in our output space
    //printf("range:%u\n", range);
    unsigned char array[1];
    const int nearest_multiple = (n - (n % range));
    assert(nearest_multiple >= n / 2);
    //printf("nearest multiple: %d\n", nearest_multiple);
    while (1){
        int r = getrandom(array, 1, 0); // generate one random byte
        assert(r == 1); // verify that we got one byte back
        unsigned char x = array[0];
        if (x < nearest_multiple){
            //printf("generated: %u\n", x);
            return x % range;
        }
    }
}


void choose_without_replacement(
    // I've thoroughly tested this function and ensured that it works.
    // this function is limited to arrays of up to 255 elements due to generate_byte, see below
    #define ELEMENT_TYPE u8
    // the #define macro is to make this function generic, so feel free to replace u8 with some other type.
    const ELEMENT_TYPE* input_array,
    ELEMENT_TYPE* output_array, // caller must allocate this, but does not need to initialize it
    int input_array_length, // number of elements in input and output array
    int output_array_length, // number of elements in input and output array
    int m, // k + p
    int elements_to_pick // number of elements to randomly pick from the input array
){
    assert(input_array_length == output_array_length);
    assert(elements_to_pick <= input_array_length);
    assert(elements_to_pick < 256); // limitation due to generate_byte
    // first, copy input array into output array
    memcpy(output_array, input_array, sizeof(ELEMENT_TYPE) * input_array_length);
    // next, pick random elements and swap them to the front of the output array
    // this is basically a Knuth shuffle
    for (int i = 0; i < elements_to_pick; i++) {
        // initially we choose between 0 and n-1
        // next we choose between 1 and n-1, and so on
        unsigned char rand_num = generate_byte(m - 1 - i);
        int random_index = i + rand_num;
        // swap the ith element with the randomly chosen element
        ELEMENT_TYPE ith_element = output_array[i];
        output_array[i] = output_array[random_index];
        output_array[random_index] = ith_element;
    }
    return;
}


void print_array(
    const char* name,
    const u8* array,
    int num_elements
){
    printf("%s: [", name);
    if (num_elements == 0){
        printf("]\n");
        return;
    }
    for (int i = 0; i < num_elements-1; i++){
        printf("%u, ", array[i]);
    }
    printf("%u", array[num_elements-1]);
    printf("]\n");
}

void print_matrix(
    const char* name,
    const u8** matrix,
    int rows,
    int cols
){
    printf("=== Begin Matrix %s ===\n", name);
    for (int i = 0; i < rows; i++){
        printf("Row %d", i);
        print_array("", matrix[i], cols);
    }
    printf("=== End Matrix ===\n");
}

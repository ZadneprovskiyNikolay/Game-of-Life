#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FILE_SIZE_OFFSET 0x0002
#define DATA_OFFSET_OFFSET 0x000A
#define WIDTH_OFFSET 0x0012
#define HEIGHT_OFFSET 0x0016
#define BITS_PER_PIXEL_OFFSET 0x001C
#define HEADER_SIZE 14
#define INFO_HEADER_SIZE 40
#define NO_COMPRESION 0
#define MAX_NUMBER_OF_COLORS 0
#define ALL_COLORS_REQUIRED 0

typedef unsigned int uint16;
typedef unsigned long uint32;
typedef unsigned char byte;

typedef unsigned int uint16;
typedef unsigned long uint32;
typedef unsigned char byte;

struct CmdArgs { 
    char* input_file_name;
    char* output_directory;
    int max_iters, interval;
};

byte* metadata;
int metadata_size;

inline void strcpy_alloc(char** strp, char* src) {
    int string_len  = strlen(src);
    *strp = (char*) malloc((string_len + 1) * sizeof(char));
    strcpy(*strp, src);
}

// Returns 1 for valid arguments, 0 otherwise.
int parse_cmdargs(int argc, char* argv[], struct CmdArgs* cmdargs) {     
    if (argc < 3) {
        printf("You didn't provide enough arguments.\n");
        return 0;
    }
    
    for (int i = 1; i < argc; i++) {
        // Input file name.
        if (*(argv[i] + 2) == 'i')
            strcpy_alloc(&(cmdargs->input_file_name), argv[i] + 8);

        // Output directory.
        else if (*(argv[i] + 2) == 'o')
            strcpy_alloc(&(cmdargs->output_directory), argv[i] + 9);

        // Maximum ages.
        else if (*(argv[i] + 2) == 'm') {
            cmdargs->max_iters = strtol(argv[i] + 11, NULL, 10);
            if (cmdargs->max_iters == 0) {
                exit(EXIT_SUCCESS);
            }
        }

        // Picture updating frequency in seconds.
        else if (*(argv[i] + 2) == 'd') {  
            cmdargs->interval = strtol(argv[i] + 12, NULL, 10);
        } 
    }

    if (cmdargs->input_file_name == NULL || cmdargs->output_directory == NULL)
        return 0;

    return 1;
}



void read_image(const char* file_name, byte** pixels, uint32* width, uint32* height) {
    //Open the file for reading in binary mode
    FILE* image_file = fopen(file_name, "rb");
    if (image_file == NULL) {
        printf("Could not open image.");
        exit(EXIT_FAILURE);
    }

    // Read file size.
    uint32 file_size;
    fseek(image_file, FILE_SIZE_OFFSET, SEEK_SET);
    fread(&file_size, 4, 1, image_file);
    // Read data offset.
    uint32 data_offset;
    fseek(image_file, DATA_OFFSET_OFFSET, SEEK_SET);
    fread(&data_offset, 4, 1, image_file);
    // Read width.
    fseek(image_file, WIDTH_OFFSET, SEEK_SET);
    fread(width, 4, 1, image_file);
    // Read height.
    fseek(image_file, HEIGHT_OFFSET, SEEK_SET);
    fread(height, 4, 1, image_file);
    // Read bits per pixel.
    short bits_per_pixel;
    fseek(image_file, BITS_PER_PIXEL_OFFSET, SEEK_SET);
    fread(&bits_per_pixel, 2, 1, image_file);

    if (bits_per_pixel != 1) {
        printf("Pitcture is not nomochrome (bits per pixel: %d)\n", bits_per_pixel);            
        exit(EXIT_FAILURE);
    }

    printf("Intial picture:\n");
    printf("width: %d, height: %d\n\n", *width, *height);

    int padded_row_size = (*width + (32 - *width % 32)) / 8.0f;
    int unpadded_row_size = (*width + (8 - *width % 8)) / 8.0f;

    int total_size = unpadded_row_size * (*height);
    *pixels = (byte*) malloc(total_size);

    int i = 0;    
    byte* cur_rowp = *pixels; // Points to the last row of pixel array.
    byte* padded_row = (byte*) malloc(padded_row_size);
    fseek(image_file, data_offset, SEEK_SET);
    for (i = 0; i < *height; i++) {    
        fread(padded_row, padded_row_size, 1, image_file);
        memcpy(cur_rowp, padded_row, unpadded_row_size);     
        cur_rowp += unpadded_row_size;
    }

    // Copy meta information.
    metadata_size = data_offset;
    metadata = (byte*) malloc(data_offset);
    fseek(image_file, 0, SEEK_SET);
    fread(metadata, data_offset, 1, image_file);

    fclose(image_file);
}

inline byte get_color(byte* field, int row_idx, int col_idx, int width);

void write_image(const char* file_name, const char* dir_name, byte* field, uint32 width, uint32 height) {
    // Create directory if it doesn't exist.
    struct stat st = {0};
    if (stat(dir_name, &st) == -1) {
        if (mkdir(dir_name)) {            
            printf("Couldn't create directory: %s\n", strerror(errno));
            return;
        }
    }    

    // Create file path.
    char* file_path = (char*) calloc(sizeof(dir_name) + sizeof(file_name) + 2, 1);
    memcpy(file_path, dir_name, strlen(dir_name));
    file_path[strlen(dir_name)] = '\\';
    memcpy(file_path + strlen(dir_name) + 1, file_name, strlen(file_name));

    FILE* image_file = fopen(file_path, "wb+");
    if (image_file == NULL) {
        printf("Could not create output file.\n");
        exit(EXIT_FAILURE);
    }

    int padded_row_size = (width + (32 - width % 32)) / 8.0f;
    int unpadded_row_size = (width + (8 - width % 8)) / 8.0f;
    uint32 file_size = metadata_size + width * padded_row_size;
    
    // Rewrite metadata: file_size, width, height.
    memcpy(metadata + FILE_SIZE_OFFSET, &file_size, 4);
    memcpy(metadata + WIDTH_OFFSET, &width, 4);
    memcpy(metadata + HEIGHT_OFFSET, &height, 4);
    fwrite(metadata, metadata_size, 1, image_file);

    // Write pixel data.
    const int bit_padding = unpadded_row_size * 8 - width;
    const int padding_size = padded_row_size - unpadded_row_size;
    byte* unpadded_row = (byte*) malloc(unpadded_row_size);
    byte* padding = (byte*) calloc(padding_size, 1);    
    for (int row_idx = 0; row_idx < height; row_idx++) {

        // Clear unpadded row.
        for (int byte_idx = 0; byte_idx < unpadded_row_size; byte_idx++) 
            unpadded_row[byte_idx] = 0;

        // Create row.        
        for (int col_idx = 0; col_idx < width; col_idx++) {            
            int bit = get_color(field, row_idx, col_idx, width);
            int byte_idx = col_idx / 8;
            unpadded_row[byte_idx] += (bit << (7 - col_idx % 8));
        }

        // Write row.
        fwrite(unpadded_row, unpadded_row_size, 1, image_file);
        fwrite(padding, padding_size, 1, image_file);        
    }

    fclose(image_file);
}

short get_color_bitmap(byte* pixels, int bit_row, int bit_col, int width, int height, int bit_padding) {
    int byte_width = (width + bit_padding) / 8;
    int byte_idx = (height - (bit_row + 1)) * byte_width + (bit_col / 8);
    short res;
    if ((byte_idx + 1) % width == 0)       
        res = (pixels[byte_idx] >> (width - 1 - bit_col + bit_padding) ) & 0x01;
    else {
        int shift = 7 - (bit_col % 8);
        res = ( pixels[byte_idx] >> shift ) & 0x01;
    }
    return res;
}

inline byte get_color(byte* field, int row_idx, int col_idx, int width) {
    return *(field + row_idx * width + col_idx);
}

inline void set_color(byte color, byte* field, int row_idx, int col_idx, int width) {
    *(field + row_idx * width + col_idx) = color;
}

int main(int argc, char* argv[]) {    
    // Parse cmd arguments.
    struct CmdArgs cmdargs = { NULL, NULL, 0, .interval = 1 };
    if (!parse_cmdargs(argc, argv, &cmdargs)) {
        printf("Incorrect argumets.\n");
        exit(EXIT_FAILURE);
    }    

    // Read image bits.
    uint32 width, height;
    byte* pixels; // Picture pixels are stored bottom up.
    read_image(cmdargs.input_file_name, &pixels, &width, &height);
    const int bytes_per_row = (width + (8 - width % 8)) / 8.0f;    
    const int bit_padding = bytes_per_row * 8 - width;

    // Write bits into 2d array.
    byte* field = (byte*) malloc(height * width);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {            
            byte color = get_color_bitmap(pixels, i, j, width, height, bit_padding);
            set_color(color, field, width - 1 - i, j, width);
        }
    }   
    free(pixels);   

    // Create file name.        
    char file_format[5] = { '.', 'b', 'm', 'p', '\0' };
    char* file_name = (char*) "output";
    char* file_name_full = (char*) malloc(6 + 5);
    memcpy(file_name_full, file_name, 6);
    memcpy(file_name_full + 6, file_format, 5);
    write_image(file_name_full, cmdargs.output_directory, field, width, height);    

    return 0;
}
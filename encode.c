#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include "encode.h"
#include "types.h"

#define MAGIC_STRING "#*"   // Define a proper magic string
#define MAX_FILE_NAME 256
#define MAX_EXTN_SIZE 8

/* Read file size using fseek/ftell */
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint32_t width = 0, height = 0;

    // Seek to 18th byte (width is stored at offset 18 in little-endian BMP)
    fseek(fptr_image, 18, SEEK_SET);

    // Read the width (an int)
    fread(&width, sizeof(uint32_t), 1, fptr_image);

    // Read the height (an int)
    fread(&height, sizeof(uint32_t), 1, fptr_image);

    // Return image capacity (3 bytes per pixel for 24-bit BMP)
    uint64_t bytes_capacity = (uint64_t)width * (uint64_t)height * 3ULL;
    return (uint)bytes_capacity;
}

uint get_file_size(FILE *fptr)
{
    uint size = 0;
    long cur_pos = ftell(fptr);
    if (cur_pos == -1L)
        cur_pos = 0;

    // Move to end, tell, then restore
    fseek(fptr, 0, SEEK_END);
    long sz = ftell(fptr);
    if (sz == -1L)
        size = 0;
    else
        size = (uint)sz;

    fseek(fptr, cur_pos, SEEK_SET);
    return size;
}

/* Validate and read arguments */
Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    printf("INFO: Checking source image extension\n");
    if (strcmp(strstr(argv[2], ".bmp"), ".bmp") != 0)
    {
        printf("ERROR: Source image file must be .bmp\n");
        return e_failure;
    }
    printf("SUCCESS: Valid extension\n");
    encInfo->src_image_fname = argv[2];

    printf("INFO: Checking for secret message file\n");
    if (argv[3] != NULL)
        encInfo->secret_fname = argv[3];
    else
    {
        printf("ERROR: Secret file not provided\n");
        return e_failure;
    }
    printf("SUCCESS: Secret message found\n");

    // Extract secret file extension
    printf("INFO: Extracting secret file extension\n");
    const char *secret_fname = encInfo->secret_fname;
    const char *dot = strrchr(secret_fname, '.');
    
    // Check if a dot was found and if it's not the very first character (like ".bashrc")
    if (dot != NULL && dot != secret_fname)
    {
        // Copy the extension (starting after the dot)
        strncpy(encInfo->extn_secret_file, dot + 1, sizeof(encInfo->extn_secret_file) - 1); 
        encInfo->extn_secret_file[sizeof(encInfo->extn_secret_file) - 1] = '\0';
        printf("SUCCESS: Secret file extension verified as .%s\n", encInfo->extn_secret_file);
    }
    else
    {
        // If no dot or dot is the first character, use an empty string as the extension.
        encInfo->extn_secret_file[0] = '\0';
        printf("INFO: Secret file has no extension or is a dotfile (using empty extension).\n");
        // Removed the previous ERROR and return e_failure. This allows files without extensions.
    }

    // Output stego file (Handles optional argument: if argv[4] is NULL, uses "steg.bmp")
    if (argv[4] != NULL)
        encInfo->stego_image_fname = argv[4];
    else
        encInfo->stego_image_fname = "steg.bmp";

    return e_success;
}

Status open_files(EncodeInfo *encInfo)
{
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "rb");
    if (encInfo->fptr_src_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->src_image_fname);
        return e_failure;
    }

    encInfo->fptr_secret = fopen(encInfo->secret_fname, "rb");
    if (encInfo->fptr_secret == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->secret_fname);
        fclose(encInfo->fptr_src_image);
        return e_failure;
    }

    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "wb");
    if (encInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->stego_image_fname);
        fclose(encInfo->fptr_src_image);
        fclose(encInfo->fptr_secret);
        return e_failure;
    }

    return e_success;
}

Status check_capacity(EncodeInfo *encInfo)
{
    if (!encInfo || !encInfo->fptr_src_image || !encInfo->fptr_secret)
        return e_failure;

    encInfo->image_capacity = get_image_size_for_bmp(encInfo->fptr_src_image);
    encInfo->size_secret_file = get_file_size(encInfo->fptr_secret);

    const char *secret = encInfo->secret_fname;
    const char *dot = strrchr(secret, '.');
    if (dot && dot != secret) // Check for dot and ensure it's not a dotfile
    {
        // Using strncpy to safely copy extension into the struct's buffer
        strncpy(encInfo->extn_secret_file, dot + 1, sizeof(encInfo->extn_secret_file) - 1);
        encInfo->extn_secret_file[sizeof(encInfo->extn_secret_file) - 1] = '\0';
    }
    else
    {
        // No extension, set to empty string
        encInfo->extn_secret_file[0] = '\0';
    }

    uint magic_bits = strlen(MAGIC_STRING) * 8;
    // Calculate required bits based on the *actual* determined extension length
    uint extn_len = strlen(encInfo->extn_secret_file); 
    
    // Required bits: Magic (16) + Extn Size (32) + Extn Data (Extn Len * 8) + File Size (32) + File Data (File Size * 8)
    uint64_t required_bits = (uint64_t)magic_bits + 32 + (uint64_t)(extn_len * 8) + 32 + (uint64_t)encInfo->size_secret_file * 8;

    // Check if image capacity (in bytes) is sufficient for required bits (in bytes)
    // Note: Capacity is in bytes (3 bytes per pixel * W * H), required_bits is in bits.
    // The image capacity is the *total number of LSBs available*, which is equal to the byte count.
    if (encInfo->image_capacity >= (required_bits / 8.0) + 1) // Required bits / 8 + 1 for safety margin
        return e_success;

    fprintf(stderr, "ERROR: Insufficient image capacity. Image capacity (bytes): %u, Required (bytes): %lu\n",
            encInfo->image_capacity, (unsigned long)(required_bits / 8.0) + 1);
    return e_failure;
}

Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image)
{
    if (!fptr_src_image || !fptr_dest_image)
        return e_failure;

    rewind(fptr_src_image);
    unsigned char header[54];
    size_t read = fread(header, 1, 54, fptr_src_image);
    if (read != 54)
    {
        fprintf(stderr, "ERROR: Unable to read BMP header\n");
        return e_failure;
    }

    size_t written = fwrite(header, 1, 54, fptr_dest_image);
    if (written != 54)
    {
        fprintf(stderr, "ERROR: Unable to write BMP header to dest\n");
        return e_failure;
    }

    return e_success;
}

Status encode_byte_to_lsb(char data, char *image_buffer)
{
    if (!image_buffer)
        return e_failure;

    for (int i = 0; i < 8; ++i)
    {
        image_buffer[i] &= 0xFE;
        char bit = (data >> (7 - i)) & 0x01;
        image_buffer[i] |= bit;
    }
    return e_success;
}

Status encode_size_to_lsb(int size, char *imageBuffer)
{
    if (!imageBuffer)
        return e_failure;

    for (int i = 0; i < 32; ++i)
    {
        imageBuffer[i] &= 0xFE;
        char bit = (size >> (31 - i)) & 0x01;
        imageBuffer[i] |= bit;
    }
    return e_success;
}

Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)
{
    if (!magic_string || !encInfo)
        return e_failure;

    size_t len = strlen(magic_string);
    char imageBuffer[8];

    for (size_t i = 0; i < len; ++i)
    {
        fread(imageBuffer, 1, 8, encInfo->fptr_src_image);
        encode_byte_to_lsb(magic_string[i], imageBuffer);
        fwrite(imageBuffer, 1, 8, encInfo->fptr_stego_image);
    }

    return e_success;
}

Status encode_secret_file_extn_size(int size, EncodeInfo *encInfo)
{
    char imageBuffer[32];
    fread(imageBuffer, 1, 32, encInfo->fptr_src_image);
    encode_size_to_lsb(size, imageBuffer);
    fwrite(imageBuffer, 1, 32, encInfo->fptr_stego_image);
    return e_success;
}

Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)
{
    size_t len = strlen(file_extn);
    char imageBuffer[8];

    for (size_t i = 0; i < len; ++i)
    {
        fread(imageBuffer, 1, 8, encInfo->fptr_src_image);
        encode_byte_to_lsb(file_extn[i], imageBuffer);
        fwrite(imageBuffer, 1, 8, encInfo->fptr_stego_image);
    }

    return e_success;
}

Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
    char imageBuffer[32];
    fread(imageBuffer, 1, 32, encInfo->fptr_src_image);
    encode_size_to_lsb((int)file_size, imageBuffer);
    fwrite(imageBuffer, 1, 32, encInfo->fptr_stego_image);
    return e_success;
}

Status encode_secret_file_data(EncodeInfo *encInfo)
{
    uint data_size = encInfo->size_secret_file;
    // Removed malloc/free for simplicity and efficiency, reading directly from file
    
    char secret_byte;
    char imageBuffer[8];
    rewind(encInfo->fptr_secret); // Ensure we start from the beginning of the secret file

    for (uint i = 0; i < data_size; ++i)
    {
        // Read 1 byte from secret file
        if (fread(&secret_byte, 1, 1, encInfo->fptr_secret) != 1)
        {
            fprintf(stderr, "ERROR: Could not read secret byte %u.\n", i);
            return e_failure;
        }
        
        // Read 8 bytes from source image
        if (fread(imageBuffer, 1, 8, encInfo->fptr_src_image) != 8)
        {
            fprintf(stderr, "ERROR: Could not read 8 bytes from source image for byte %u.\n", i);
            return e_failure;
        }
        
        // Encode the secret byte into LSBs of the 8 image bytes
        encode_byte_to_lsb(secret_byte, imageBuffer);
        
        // Write the 8 stego image bytes to the stego file
        if (fwrite(imageBuffer, 1, 8, encInfo->fptr_stego_image) != 8)
        {
            fprintf(stderr, "ERROR: Could not write 8 stego bytes for byte %u.\n", i);
            return e_failure;
        }
    }

    return e_success;
}

Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest)
{
    int ch;
    while ((ch = fgetc(fptr_src)) != EOF)
        fputc(ch, fptr_dest);
    return e_success;
}

Status do_encoding(EncodeInfo *encInfo)
{
    if (open_files(encInfo) != e_success)
    {
        fprintf(stderr, "ERROR: Opening files failed\n");
        return e_failure;
    }

    printf("Files opened successfully.\n");

    if (check_capacity(encInfo) != e_success)
    {
        // check_capacity prints the detailed error
        return e_failure;
    }

    printf("Image has enough capacity.\n");

    if (copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image) != e_success)
    {
        return e_failure;
    }
    printf("BMP header copied.\n");

    if (encode_magic_string(MAGIC_STRING, encInfo) != e_success)
    {
        return e_failure;
    }
    printf("Magic string encoded.\n");

    int extn_size = strlen(encInfo->extn_secret_file);
    if (encode_secret_file_extn_size(extn_size, encInfo) != e_success)
    {
        return e_failure;
    }
    printf("Extension size encoded: %d\n", extn_size);

    if (extn_size > 0)
    {
        if (encode_secret_file_extn(encInfo->extn_secret_file, encInfo) != e_success)
        {
            return e_failure;
        }
        printf("Extension encoded: %s\n", encInfo->extn_secret_file);
    }

    if (encode_secret_file_size(encInfo->size_secret_file, encInfo) != e_success)
    {
        return e_failure;
    }
    printf("Secret file size encoded: %ld bytes\n", encInfo->size_secret_file);

    if (encode_secret_file_data(encInfo) != e_success)
    {
        return e_failure;
    }
    printf("Secret file data encoded.\n");

    if (copy_remaining_img_data(encInfo->fptr_src_image, encInfo->fptr_stego_image) != e_success)
    {
        return e_failure;
    }
    printf("Remaining image data copied.\n");

    fclose(encInfo->fptr_src_image);
    fclose(encInfo->fptr_secret);
    fclose(encInfo->fptr_stego_image);

    return e_success;
}
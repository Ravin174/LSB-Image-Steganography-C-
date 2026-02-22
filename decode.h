#ifndef DECODE_H
#define DECODE_H
#include <stdio.h>
#include "types.h"
#include "common.h" // Added to ensure MAGIC_STRING is available if needed

/* Structure to store decoding information */
typedef struct _DecodeInfo
{
    /* File names */
    char stego_image_fname[50];
    char output_fname[50];

    /* File pointers */
    FILE *fptr_stego_image;
    FILE *fptr_output;

    /* Decoded data */
    char extn_secret_file[10]; // Increased size for flexibility
    uint size_secret_file;
    char magic_string[10];

} DecodeInfo;

/* Function declarations */

/* Read and validate decode arguments */
Status_d read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo);

/* Open files for decoding */
Status_d open_decode_files(DecodeInfo *decInfo);

/* Perform the decoding process */
Status_d do_decoding(DecodeInfo *decInfo);

/* Decode operations */
Status_d decode_magic_string(DecodeInfo *decInfo);
Status_d decode_secret_file_extn_size(DecodeInfo *decInfo, int *extn_size);
Status_d decode_secret_file_extn(DecodeInfo *decInfo, char *extn, int extn_size);
Status_d decode_secret_file_size(DecodeInfo *decInfo, long *file_size);
Status_d decode_secret_file_data(DecodeInfo *decInfo, long file_size);

/* Helper decode functions */
Status_d decode_byte_from_lsb(char *data, char *image_buffer);
Status_d decode_size_from_lsb(int *size, char *image_buffer);

/* (Optional) You can add more helpers if needed later */
Status_d decode_int_from_lsb(int *value, char *image_buffer);
Status_d decode_data_from_image(char *data, int size, FILE *fptr_stego_image);

#endif
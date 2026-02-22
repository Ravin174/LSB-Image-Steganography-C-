/*
Name: RAVI J S
Student ID: 25021_387
Date of submission : 11th Nov 2025. 
Description: The Steganography project involves both encoding and decoding processes using 
    the Least Significant Bit (LSB) technique. In encoding, secret data is hidden within an image
    without visibly altering it, creating a stego image for secure communication. During decoding,
    the hidden bits are extracted from the stego image to reconstruct the original secret message. 
    This ensures safe and invisible data transmission while maintaining image integrity.
*/
#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "decode.h"
#include "types.h"
#include "common.h"

// Function to check whether the operation is encode or decode
OperationType check_operation_type(char *argv[])
{
    if (strcmp(argv[1], "-e") == 0)
        return e_encode;
    else if (strcmp(argv[1], "-d") == 0)
        return e_decode;
    else
        return e_unsupported;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage:\n");
        printf("For encoding: %s -e <.bmp file> <secret.txt> [output.bmp]\n", argv[0]); // Updated Usage
        printf("For decoding: %s -d <stego.bmp> <output.txt>\n", argv[0]);
        return 0;
    }

    OperationType opt = check_operation_type(argv);

    EncodeInfo encInfo;
    DecodeInfo decInfo;

    switch (opt)
    {
    case e_encode:
        // FIX: Allow 4 arguments (optional output file)
        if (argc < 4 || argc > 5)
        {
            printf("Invalid number of arguments for encoding.\n");
            printf("Usage: %s -e <.bmp file> <secret.txt> [output.bmp]\n", argv[0]); // Updated Usage
            return 0;
        }

        // Determine the output filename for printing info
        char *output_filename = (argc == 5) ? argv[4] : "steg.bmp (default)";

        printf("Selected encoding operation.\n");
        printf("Input BMP file   : %s\n", argv[2]);
        printf("Secret text file : %s\n", argv[3]);
        printf("Output BMP file  : %s\n", output_filename); // Print determined name

        // Pass arguments to validation function
        if (read_and_validate_encode_args(argv, &encInfo) == e_success)
        {
            if (do_encoding(&encInfo) == e_success)
            {
                // Print the *actual* final filename stored in encInfo, which handles the default
                printf("Encoding completed successfully.\nOutput file saved as: %s\n", encInfo.stego_image_fname);
            }
            else
                printf("ERROR: Encoding failed.\n");
        }
        else
            printf("ERROR: Invalid encoding arguments.\n");
        break;

    case e_decode:
        if (argc != 4)
        {
            printf("Invalid number of arguments for decoding.\n");
            printf("Usage: %s -d <stego.bmp> <output.txt>\n", argv[0]);
            return 0;
        }

        printf("Selected decoding operation.\n");
        printf("Stego BMP file   : %s\n", argv[2]);
        printf("Output text file : %s\n", argv[3]);

        if (read_and_validate_decode_args(argv, &decInfo) == e_success)
        {
            if (do_decoding(&decInfo) == e_success)
                printf("Decoding completed successfully.\nOutput file saved as: %s\n", argv[3]);
            else
                printf("ERROR: Decoding failed.\n");
        }
        else
            printf("ERROR: Invalid decoding arguments.\n");
        break;

    default:
        printf("Unsupported operation. Use -e for encoding or -d for decoding.\n");
        break;
    }

    return 0;
}
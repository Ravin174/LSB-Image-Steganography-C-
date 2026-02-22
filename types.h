#ifndef TYPES_H
#define TYPES_H

/* User defined types */
typedef unsigned int uint;

/* Status will be used in fn. return type */
typedef enum
{
    e_failure,
    e_success
} Status;

typedef enum
{
    d_failure,
    d_success
} Status_d;

typedef enum
{
    e_encode,
    e_decode,
    e_unsupported
} OperationType;

#endif

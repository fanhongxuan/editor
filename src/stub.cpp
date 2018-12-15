#include "config.h"
#include <stdlib.h>
extern "C"{
    const char *progname = "ctags";
    
#ifndef PARAMS
# if __STDC__ || defined __GNUC__ || defined __SUNPRO_C || defined __cplusplus || __PROTOTYPES
#  define PARAMS(Args) Args
# else
#  define PARAMS(Args) ()
# endif
#endif
    
    /* We assume to have `unsigned long int' value with at least 32 bits.  */
#define HASHWORDBITS 32
    
    /* Defines the so called `hashpjw' function by P.J. Weinberger
        [see Aho/Sethi/Ullman, COMPILERS: Principles, Techniques and Tools,
        1986, 1987 Bell Telephone Laboratories, Inc.]  */
    unsigned long int __hash_string (const char *str_param)
    {
        unsigned long int hval, g;
        const char *str = str_param;
        /* Compute the hash value for the given string.  */
        hval = 0;
        while (*str != '\0')
        {
            hval <<= 4;
            hval += (unsigned char) *str++;
            g = hval & ((unsigned long int) 0xf << (HASHWORDBITS - 4));
            if (g != 0)
            {
                hval ^= g >> (HASHWORDBITS - 8);
                hval ^= g;
            }
        }
        return hval;
    }
}
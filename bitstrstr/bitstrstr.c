#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timeb.h>
#include <stdint.h>

#define BITS 14
#define MASK ((1 << BITS) - 1)
#define JUMP_SIZE (1 << BITS)


//#define fprintf if(0)

static int
get_bit(const unsigned char *vector, int index) {
    int byte = vector[index >> 3];
    return ((byte >> (index & 0x7)) & 1);
}

static void
set_bit(unsigned char *vector, int index, int on) {
    int byte = vector[index >> 3];
    if (on)
        byte |=  (1 << (index & 0x7));
    else
        byte &= ~(1 << (index & 0x7));
    vector[index >> 3] = byte;
}

static void
dump_bitstr(const char *title, const unsigned char *vector, int bitoffset, int bitlen) {
    int i;
    if (title) fprintf(stderr, "%s:", title);
    for (i = 0; i < bitlen; i++) {
        int ix = i + bitoffset;
        int bit = get_bit(vector, ix);
        //if ((ix & 0x7) == 0) {
        //    if (i) fprintf(stderr, " ");
        //    fprintf(stderr, "0x%02x:", vector[ix>>3]);
        //}
        if (!(i & 0x7)) fprintf(stderr, " ");
        fprintf(stderr, "%c", '0' + bit);
    }
    fprintf(stderr, "\n");
}

static void
dump_window(const char *title, unsigned int window) {
    unsigned char tmp[sizeof(window)];
    int bitlen = sizeof(window) << 3;
    int i;
    for (i = 0; i < bitlen; i++) {
        set_bit(tmp, i, window & 1);
        window >>= 1;
    }
    dump_bitstr(title, tmp, 0, bitlen);
}

static int
slow_check(const unsigned char *haystack, int offset,
           const unsigned char *needle, int needle_bitoff, int needle_bitlen) {
    int i;
    dump_bitstr("slow_check needle", needle, needle_bitoff, needle_bitlen);
    dump_bitstr("         haystack", haystack, offset, needle_bitlen);
    for (i = 0; i < needle_bitlen; i++, offset++)
        if (get_bit(haystack, offset) != get_bit(needle, i + needle_bitoff))
            return 0;
    return 1;
}

static uint32_t
delta[256] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	       17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	       32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46,
	       47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
	       62, 63, 64, 72, 80, 88, 96, 104, 112, 120, 128, 136, 144,
	       152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240,
	       248, 256, 264, 272, 280, 288, 296, 304, 312, 320, 328, 336,
	       344, 352, 360, 368, 376, 384, 392, 400, 416, 432, 448, 464,
	       480, 496, 512, 528, 544, 560, 576, 592, 608, 632, 656, 680,
	       704, 728, 752, 776, 800, 832, 864, 896, 928, 960, 992, 1024,
	       1064, 1104, 1144, 1184, 1224, 1272, 1320, 1368, 1416, 1472,
	       1528, 1584, 1640, 1704, 1768, 1832, 1904, 1976, 2048, 2128,
	       2208, 2296, 2384, 2472, 2568, 2664, 2768, 2872, 2984, 3096,
	       3216, 3344, 3472, 3608, 3752, 3896, 4048, 4208, 4376, 4544,
	       4720, 4904, 5096, 5296, 5504, 5720, 5944, 6176, 6416, 6672,
	       6936, 7208, 7496, 7792, 8096, 8416, 8752, 9096, 9456, 9832,
	       10224, 10632, 11056, 11496, 11952, 12424, 12920, 13432,
	       13968, 14520, 15096, 15696, 16320, 16968, 17640, 18344,
	       19072, 19832, 20624, 21448, 22304, 23192, 24112, 25072,
	       26072, 27112, 28192, 29312, 30480, 31696, 32960, 34272,
	       35640, 37064, 38544, 40080, 41680, 43344, 45072, 46872,
	       48744, 50688, 52712, 54816, 57008, 59288, 61656, 64120,
	       66680, 69344, 72112, 74992, 77984, 81096, 84336, 87704,
	       91208, 94856, 98648, 102592, 106688 };

//int
//bitstrstr(const unsigned char *haystack, int haystack_bitlen,
//          const unsigned char *needle, int needle_bitlen) {

int
bitstrstr(const unsigned char *haystack, int haystack_bitoff, int haystack_bitlen,
          const unsigned char *needle, int needle_bitoff, int needle_bitlen) {
    unsigned char delta_ix, jump[JUMP_SIZE];
    int dist, j, j8, start;
    unsigned int window;
    const unsigned char *pivot, *pivot_top;
    
    dist = needle_bitlen - (BITS - 1);
    for (delta_ix = 255; delta[delta_ix] > dist; delta_ix--);
    memset(jump, delta_ix, sizeof(jump));

    // don't check what we know that it can't be better
    start = needle_bitoff + dist - delta[delta_ix];
    dist = delta[delta_ix];
    
    j = start >> 3;
    window  = needle[j++];
    window |= needle[j++] << 8;
    window |= needle[j++] << 16;
    window >>= (start & 0x7);
    j8 = 8 - (start & 0x7);
    
    while (dist-- > 0) {
        fprintf(stderr, "dist: %d, j: %d\n", dist, j);
        dump_window("jump", window);

        if (delta[delta_ix] > dist) delta_ix--;
        jump[window & MASK] = delta_ix;
        
        if (!j8) {
            window |= (needle[j++] << 16);
            window |= (needle[j++] << 24);
            j8 = 16;
        }
        j8--;
        window >>= 1;
    }

    pivot_top = haystack + ((haystack_bitoff + haystack_bitlen - BITS) >> 3) + 1;
    pivot = haystack + ((haystack_bitoff + needle_bitlen - BITS) >> 3);

    while (pivot < pivot_top) {
        int window = (pivot[0] | (pivot[1] << 8));
        int delta_ix = jump[window & MASK];

        if (delta_ix > 8) {
            pivot += delta[delta_ix] >> 3;
        }
        else {
            int i = delta_ix;
            window |= pivot[2] << 16;
            window >>= delta_ix;

            while ((delta_ix = jump[window & MASK]) <= 8) {
                if (!delta_ix) {
                    int pivot_offset = i - (needle_bitlen - BITS); 
                    if (slow_check(pivot, pivot_offset, needle, needle_bitoff, needle_bitlen - BITS)) {
                        int offset = pivot_offset + ((pivot - haystack) << 3);
                        if (offset >= haystack_bitoff) {
                            if (offset < haystack_bitlen + haystack_bitoff)
                                return offset;
                            return -1;
                        }
                    }
                    delta_ix = 1;
                }

                i += delta_ix;
                window >>= delta_ix;

                if (i > 8) {
                    if (++pivot >= pivot_top)
                        return -1;
                    i -= 8;
                    window |= pivot[2] << (16 - i);
                }
            }
            pivot += (i + delta[delta_ix]) >> 3;
        }
    }
    return -1;
}

#define CHUNK_SIZE 100000

static const unsigned char *
read_file(const char *fn, int *size) {
    FILE *fh;
    unsigned char *haystack = NULL;
    int allocated = 0;
    int readed = 0;
    
    fh = fopen(fn, "r");
    if (!fh) {
        perror(fn);
        exit(1);
    }
    while (1) {
        int bytes;
        if (allocated <= readed + CHUNK_SIZE) {
            allocated = allocated * 2 + CHUNK_SIZE;
            haystack = realloc(haystack, allocated);
            if (!haystack) {
                perror(fn);
                exit(1);
            }
        }
        bytes = fread(haystack + readed, 1, CHUNK_SIZE, fh);
        if (bytes > 0)
            readed += bytes;
        else if (feof(fh)) {
            if (size)
                *size = readed;
            return haystack;
        }
        else if (ferror(fh)) {
            perror(fn);
            exit(1);
        }
    }
}

static unsigned char *
get_needle(const unsigned char *haystack, int offset, int needle_bitlen) {
    int bytes = ((needle_bitlen >> 3) + 2);
    int i;
    unsigned char *needle = malloc(bytes);
    needle[bytes - 2] = 0;
    needle[bytes - 1] = 0;
    for (i = 0; i < needle_bitlen; i++, offset++)
        set_bit(needle, i, get_bit(haystack, offset));
        // set_bit(needle, i, i & 1);
    return needle;
}

static struct timeb chrono;

static void
start_chrono(void) {
    ftime(&chrono);
}

static double
read_chrono(void) {
    struct timeb now;
    ftime(&now);
    return (now.time - chrono.time) + 0.0001 * (now.millitm - chrono.millitm);
}


int
main(int argc, char *argv[]) {
    const unsigned char *haystack, *needle;
    int haystack_bytelen, needle_offset, needle_prefix, needle_bitlen, r, i, reps;
    double chrono;
    
    if ((argc < 4) || (argc > 6)) {
        fprintf(stderr, "usage: %s haystack_filename needle_offset needle_bitlen needle_prefix reps\n", argv[0]);
        exit(1);
    }

    haystack = read_file(argv[1], &haystack_bytelen);
    needle_offset = atoi(argv[2]);
    needle_bitlen = atoi(argv[3]);
    needle_prefix = (argc >= 5 ? atoi(argv[4]) : 0);
    reps = (argc >= 6 ? atoi(argv[5]) : 1);

    if (needle_bitlen + needle_offset > haystack_bytelen * 8) {
        fprintf(stderr, "not enough bits for needle\n");
        exit(1);
    }

    dump_bitstr("needle in haystack", haystack, needle_offset, needle_bitlen);

    needle = get_needle(haystack, needle_offset - needle_prefix, needle_bitlen + needle_prefix);

    dump_bitstr("            needle", needle, needle_prefix, needle_bitlen);

    start_chrono();
    for (i = 0; i < reps; i++)
        r = bitstrstr(haystack, 0, haystack_bytelen << 3, needle, needle_prefix, needle_bitlen);
    chrono = read_chrono();
    
    printf("needle found at %d, expected at %d in %g/%d = %gms\n", r, needle_offset, chrono * 1000, reps, 1000 * chrono / reps);
    
    exit ( needle_offset == r ? 0 : 1);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timeb.h>
#include <stdint.h>

#define BITS 15
#define MASK ((1 << BITS) - 1)
#define JUMP_SIZE (1 << BITS)


#define fprintf if(0)

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
bm_jump[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
	      18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
	      33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	      48, 49, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74,
	      76, 79, 82, 85, 88, 91, 94, 97, 100, 104, 108, 112, 116, 120,
	      124, 128, 133, 138, 143, 148, 153, 159, 165, 171, 177, 184,
	      191, 198, 205, 213, 221, 229, 238, 247, 256, 266, 276, 287,
	      298, 309, 321, 333, 346, 359, 373, 387, 402, 418, 434, 451,
	      469, 487, 506, 526, 547, 568, 590, 613, 637, 662, 688, 715,
	      743, 772, 802, 834, 867, 901, 937, 974, 1012, 1052, 1094,
	      1137, 1182, 1229, 1278, 1329, 1382, 1437, 1494, 1553, 1615,
	      1679, 1746, 1815, 1887, 1962, 2040, 2121, 2205, 2293, 2384,
	      2479, 2578, 2681, 2788, 2899, 3014, 3134, 3259, 3389, 3524,
	      3664, 3810, 3962, 4120, 4284, 4455, 4633, 4818, 5010, 5210,
	      5418, 5634, 5859, 6093, 6336, 6589, 6852, 7126, 7411, 7707,
	      8015, 8335, 8668, 9014, 9374, 9748, 10137, 10542, 10963,
	      11401, 11857, 12331, 12824, 13336, 13869, 14423, 14999,
	      15598, 16221, 16869, 17543, 18244, 18973, 19731, 20520,
	      21340, 22193, 23080, 24003, 24963, 25961, 26999, 28078,
	      29201, 30369, 31583, 32846, 34159, 35525, 36946, 38423,
	      39959, 41557, 43219, 44947, 46744, 48613, 50557, 52579,
	      54682, 56869, 59143, 61508, 63968, 66526, 69187, 71954,
	      74832, 77825, 80938, 84175, 87542, 91043, 94684, 98471,
	      102409, 106505, 110765, 115195, 119802 };

//int
//bitstrstr(const unsigned char *haystack, int haystack_bitlen,
//          const unsigned char *needle, int needle_bitlen) {

int
bitstrstr(const unsigned char *haystack, int haystack_bitoff, int haystack_bitlen,
          const unsigned char *needle, int needle_bitoff, int needle_bitlen) {
    unsigned char jump[JUMP_SIZE];
    unsigned int exp_jump[JUMP_SIZE];
    int i, dist, j, j8, i_byte, jump_ix;
    unsigned int window;

    dist = needle_bitlen - (BITS - 1);
    for (jump_ix = 255; bm_jump[jump_ix] > dist; jump_ix--);

    memset(jump, jump_ix, sizeof(jump));

    j = needle_bitoff >> 3;
    window  = needle[j++];
    window |= needle[j++] << 8;
    window |= needle[j++] << 16;
    window >>= (needle_bitoff & 0x7);
    j8 = 8 - (needle_bitoff & 0x7);
    
    while (dist-- > 0) {
        fprintf(stderr, "dist: %d, j: %d\n", dist, j);
        dump_window("jump", window);
        jump[window & MASK] = dist;

        if (bm_jump[jump_ix] > dist) jump_ix--;
        
        if (!j8) {
            window |= (needle[j++] << 16);
            window |= (needle[j++] << 24);
            j8 = 16;
        }
        j8--;
        window >>= 1;
    }

    for (i = 0; i < JUMP_SIZE; i++) exp_jump[i] = bm_jump[jump[i]];
    
    i = needle_bitlen - BITS;
    i_byte = i >> 3;
    while (i < haystack_bitlen - BITS) {
        int window = (haystack[i_byte] | (haystack[i_byte + 1] << 8));
        
        dump_window("window at byte", window);
        dump_bitstr("      haystack", haystack + i_byte - 1, 0, 16);
        fprintf(stderr,
                "jump %d, from (%d) %d to %d (%d)\n",
                bm_jump[jump[window & MASK]],
                i,
                (i_byte << 3),
                (i_byte << 3) + bm_jump[jump[window & MASK]],
                ((((i_byte << 3) + bm_jump[jump[window & MASK]]) >> 3) << 3));
        
        i = (i_byte << 3) + bm_jump[jump[window & MASK]];
        while (i < haystack_bitlen - BITS) {
            int window_jump;
            int i_next_byte = i >> 3;

            if (i_next_byte > i_byte) {
                i_byte = i_next_byte;
                break;
            }
            
            window = (haystack[i_byte] | (haystack[i_byte + 1] << 8) | (haystack[i_byte + 2] << 16));
            dump_window("window at bit0", window);
            window >>= (i & 0x7);

            dump_window(" window at bit", window);
            dump_bitstr("      haystack", haystack, i, BITS);
            dump_bitstr("   needle tail", needle, needle_bitoff + needle_bitlen - BITS, 16);
            dump_bitstr("        needle", needle, needle_bitoff, needle_bitlen);
            fprintf(stderr, "jump %d, from %d to %d\n", bm_jump[jump[window & MASK]], i, i + bm_jump[jump[window & MASK]]);
            
            window_jump = bm_jump[jump[window & MASK]];
            if (window_jump)
                i += window_jump;
            else {
                int offset = i - (needle_bitlen - BITS);
                if (slow_check(haystack, offset, needle, needle_bitoff, needle_bitlen))
                    return offset;
                i++;
            }
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

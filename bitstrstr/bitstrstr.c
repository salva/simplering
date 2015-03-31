#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timeb.h>

#define JUMP_SIZE (256*256)

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
slow_check(const unsigned char *haystack, int offset, const unsigned char *needle, int needle_bitlen) {
    int i;
    dump_bitstr("slow_check needle", needle, 0, needle_bitlen);
    dump_bitstr("         haystack", haystack, offset, needle_bitlen);
    for (i = 0; i < needle_bitlen; i++, offset++)
        if (get_bit(haystack, offset) != get_bit(needle, i))
            return 0;
    return 1;
}

int
bitstrstr(const unsigned char *haystack, int haystack_bitlen, const unsigned char *needle, int needle_bitlen) {
    int jump[JUMP_SIZE];
    int i, i2, j, i_byte;
    unsigned int window;

    for (i = 0; i < JUMP_SIZE; i++) jump[i] = needle_bitlen - 15;

    window = ((needle[1] << 8) | needle[0]);
    for (i = needle_bitlen - 16, i2 = 1, j = 2; i >= 0;) {
        fprintf(stderr, "i: %d, j: %d\n", i, j);
        dump_window("jump", window);
        jump[window & 0xffff] = i;
        if (--i2 == 0) {
            window |= (needle[j++] << 16);
            window |= (needle[j++] << 24);
            i2 = 16;
        }
        window >>= 1;
        i--;
    }

    i = needle_bitlen - 1;
    i_byte = (i - 7) >> 3;
    while (i < haystack_bitlen) {
        int window = (haystack[i_byte - 1] | (haystack[i_byte] << 8));
        dump_window("window at byte", window);
        dump_bitstr("      haystack", haystack + i_byte - 1, 0, 16);
        fprintf(stderr,
                "jump %d, from (%d) %d to %d (%d)\n",
                jump[window & 0xffff],
                i,
                (i_byte << 3) + 7,
                (i_byte << 3) + 7 + jump[window & 0xffff],
                ((((((i_byte << 3) + 7 + jump[window & 0xffff]) - 7) >> 3) << 3) + 7));
        i = (i_byte << 3) + 7 + jump[window & 0xffff];
        while (i < haystack_bitlen) {
            int window_jump;
            int i_next_byte = (i - 7) >> 3;

            if (i_next_byte > i_byte) {
                i_byte = i_next_byte;
                break;
            }
            
            window = (haystack[i_byte - 1] | (haystack[i_byte] << 8) | (haystack[i_byte + 1] << 16));
            dump_window("window at bit0", window);
            window >>= ((i - 7) & 0x7);

            dump_window(" window at bit", window);
            dump_bitstr("      haystack", haystack, i - 15, 16);
            dump_bitstr("   needle tail", needle, needle_bitlen - 16, 16);
            dump_bitstr("        needle", needle, 0, needle_bitlen);
            fprintf(stderr, "jump %d, from %d to %d\n", jump[window & 0xffff], i, i + jump[window & 0xffff]);
            
            window_jump = jump[window & 0xffff];
            if (window_jump)
                i += window_jump;
            else {
                int offset = i - needle_bitlen + 1;
                if (slow_check(haystack, offset, needle, needle_bitlen))
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
    int haystack_bytelen, needle_offset, needle_bitlen, r, i, reps;
    double chrono;
    
    if ((argc < 4) || (argc > 5)) {
        fprintf(stderr, "usage: %s haystack_filename needle_offset needle_bitlen reps\n", argv[0]);
        exit(1);
    }

    haystack = read_file(argv[1], &haystack_bytelen);
    needle_offset = atoi(argv[2]);
    needle_bitlen = atoi(argv[3]);
    reps = (argc == 5 ? atoi(argv[4]) : 1);

    if (needle_bitlen + needle_offset > haystack_bytelen * 8) {
        fprintf(stderr, "not enough bits for needle\n");
        exit(1);
    }

    dump_bitstr("needle in haystack", haystack, needle_offset, needle_bitlen);

    needle = get_needle(haystack, needle_offset, needle_bitlen);

    dump_bitstr("            needle", needle, 0, needle_bitlen);

    start_chrono();
    for (i = 0; i < reps; i++)
        r = bitstrstr(haystack, haystack_bytelen << 3, needle, needle_bitlen);
    chrono = read_chrono();
    
    printf("needle found at %d, expected at %d in %g/%d = %gms\n", r, needle_offset, chrono * 1000, reps, 1000 * chrono / reps);
    
    exit ( needle_offset == r ? 0 : 1);
}

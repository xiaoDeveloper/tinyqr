/* Desktop-only inspection of the vendored quirc. Not linked into the APK. */
#include "../app/src/main/c/third_party/quirc/quirc_internal.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static int token(FILE *file, char *text, size_t size) {
    int c;
    size_t n = 0;
    for (;;) {
        c = fgetc(file);
        if (c == '#') {
            while (c != '\n' && c != EOF) c = fgetc(file);
        }
        if (c == EOF) return 0;
        if (!isspace((unsigned char)c)) break;
    }
    do {
        if (n + 1 >= size) return 0;
        text[n++] = (char)c;
        c = fgetc(file);
    } while (c != EOF && !isspace((unsigned char)c));
    /* Permit a CRLF header without consuming any binary pixel bytes. */
    if (c == '\r') {
        c = fgetc(file);
        if (c != '\n' && c != EOF) ungetc(c, file);
    }
    text[n] = 0;
    return 1;
}

static int dimension(FILE *file) {
    char text[32], *end;
    if (!token(file, text, sizeof(text))) return 0;
    long value = strtol(text, &end, 10);
    return *end || value <= 0 || value > 8192 ? 0 : (int)value;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: qr_inspect sample.pgm (P5, 8-bit grayscale)\n");
        return 2;
    }
    FILE *file = fopen(argv[1], "rb");
    if (!file) { perror(argv[1]); return 2; }
    char text[32];
    int valid = token(file, text, sizeof(text)) && !strcmp(text, "P5");
    int width = dimension(file), height = dimension(file);
    valid = valid && width && height && token(file, text, sizeof(text)) && !strcmp(text, "255");
    struct quirc *q = valid ? quirc_new() : NULL;
    if (!q || quirc_resize(q, width, height) < 0) {
        fprintf(stderr, "Invalid P5 header (max 8192x8192, maxval 255) or allocation failure\n");
        if (q) quirc_destroy(q);
        fclose(file);
        return 2;
    }
    size_t size = (size_t)width * height;
    size_t received = fread(quirc_begin(q, NULL, NULL), 1, size, file);
    fclose(file);
    if (received != size) {
        fprintf(stderr, "Truncated pixel data\n");
        quirc_destroy(q);
        return 2;
    }
    quirc_end(q);
    printf("frame=%dx%d regions=%d/%d capstones=%d candidates=%d\n",
           width, height, q->num_regions - QUIRC_PIXEL_REGION,
           QUIRC_MAX_REGIONS - QUIRC_PIXEL_REGION, q->num_capstones, quirc_count(q));
    for (int i = 0; i < q->num_capstones; ++i) {
        const struct quirc_capstone *cap = &q->capstones[i];
        printf("capstone=%d center=(%d,%d) ring_area=%d stone_area=%d corners=",
               i, cap->center.x, cap->center.y,
               q->regions[cap->ring].count, q->regions[cap->stone].count);
        for (int j = 0; j < 4; ++j)
            printf(" (%d,%d)", cap->corners[j].x, cap->corners[j].y);
        putchar('\n');
    }
    int decoded = 0;
    for (int i = 0; i < quirc_count(q); ++i) {
        struct quirc_code code;
        struct quirc_data data;
        quirc_extract(q, i, &code);
        printf("candidate=%d grid=%d corners=", i, code.size);
        for (int j = 0; j < 4; ++j)
            printf(" (%d,%d)", code.corners[j].x, code.corners[j].y);
        putchar('\n');
        /* Original sampled grid allows comparison with a known module matrix. */
        if (code.size > 0 && code.size <= QUIRC_MAX_GRID_SIZE)
            for (int y = 0; y < code.size; ++y) {
                for (int x = 0; x < code.size; ++x) {
                    int bit = y * code.size + x;
                    putchar((code.cell_bitmap[bit >> 3] >> (bit & 7)) & 1 ? '1' : '0');
                }
                putchar('\n');
            }
        quirc_decode_error_t error = quirc_decode(&code, &data);
        printf("candidate=%d decode=%s\n", i, quirc_strerror(error));
        if (error != QUIRC_SUCCESS) {
            quirc_flip(&code);
            error = quirc_decode(&code, &data);
            printf("candidate=%d flipped_decode=%s\n", i, quirc_strerror(error));
        }
        if (error == QUIRC_SUCCESS) {
            ++decoded;
            printf("candidate=%d payload_bytes=%d (content omitted)\n", i, data.payload_len);
        }
    }
    if (!decoded && quirc_rebuild_stylized(q)) {
        printf("stylized_fallback candidates=%d\n", quirc_count(q));
        for (int i = 0; i < quirc_count(q); ++i) {
            struct quirc_code code;
            struct quirc_data data;
            quirc_extract(q, i, &code);
            quirc_decode_error_t error = quirc_decode(&code, &data);
            printf("stylized_candidate=%d grid=%d decode=%s\n",
                   i, code.size, quirc_strerror(error));
            if (error != QUIRC_SUCCESS) {
                quirc_flip(&code);
                error = quirc_decode(&code, &data);
                printf("stylized_candidate=%d flipped_decode=%s\n", i, quirc_strerror(error));
            }
            if (error == QUIRC_SUCCESS) {
                ++decoded;
                printf("stylized_candidate=%d payload_bytes=%d (content omitted)\n",
                       i, data.payload_len);
            }
        }
    }
    quirc_destroy(q);
    return decoded ? 0 : 1;
}

#include "../app/src/main/c/qr/qr.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum { MODULE_COUNT = 29, PIXELS_PER_MODULE = 8, PADDING = 16 };

static unsigned char *load_fixture(int *width, int *height) {
    FILE *file = fopen("tests/fixtures/qr_tinyqr_modules.txt", "r");
    char line[128];
    int row = 0;
    unsigned char *modules = calloc(MODULE_COUNT * MODULE_COUNT, 1);
    assert(file && modules);
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#') continue;
        assert((int)strcspn(line, "\r\n") == MODULE_COUNT);
        for (int column = 0; column < MODULE_COUNT; ++column) {
            assert(line[column] == '0' || line[column] == '1');
            modules[row * MODULE_COUNT + column] = line[column] == '1';
        }
        ++row;
    }
    fclose(file);
    assert(row == MODULE_COUNT);
    *width = MODULE_COUNT * PIXELS_PER_MODULE;
    *height = *width;
    return modules;
}

static unsigned char *expand_fixture(int row_stride, int *width, int *height) {
    unsigned char *modules = load_fixture(width, height);
    unsigned char *pixels = malloc((size_t)row_stride * (size_t)*height);
    assert(pixels);
    memset(pixels, 0x5a, (size_t)row_stride * (size_t)*height);
    for (int y = 0; y < *height; ++y) {
        for (int x = 0; x < *width; ++x) {
            int module_y = y / PIXELS_PER_MODULE;
            int module_x = x / PIXELS_PER_MODULE;
            pixels[y * row_stride + x] = modules[module_y * MODULE_COUNT + module_x] ? 0 : 255;
        }
    }
    free(modules);
    return pixels;
}

static void assert_empty_result(const TinyQrResult *result) {
    assert(result->data == NULL);
    assert(result->length == 0);
}

static void invalid_frames_fail(TinyQrDecoder *decoder) {
    TinyQrResult result = { (const uint8_t *)"stale", 5 };
    TinyQrFrame frame = { NULL, 1, 1, 1 };
    assert(!tinyqr_decode(decoder, &frame, &result));
    assert_empty_result(&result);
    frame = (TinyQrFrame){ (const uint8_t *)"x", 0, 1, 1 };
    assert(!tinyqr_decode(decoder, &frame, &result));
    frame = (TinyQrFrame){ (const uint8_t *)"x", 1, 0, 1 };
    assert(!tinyqr_decode(decoder, &frame, &result));
    frame = (TinyQrFrame){ (const uint8_t *)"x", 2, 1, 1 };
    assert(!tinyqr_decode(decoder, &frame, &result));
    assert(!tinyqr_decode(NULL, &frame, &result));
}

static void blank_frame_fails(TinyQrDecoder *decoder) {
    uint8_t blank[64];
    TinyQrFrame frame = { blank, 8, 8, 8 };
    TinyQrResult result;
    memset(blank, 255, sizeof(blank));
    assert(!tinyqr_decode(decoder, &frame, &result));
    assert_empty_result(&result);
}

static void fixture_decodes(TinyQrDecoder *decoder, int padded) {
    int width, height;
    int stride = MODULE_COUNT * PIXELS_PER_MODULE + (padded ? PADDING : 0);
    unsigned char *pixels = expand_fixture(stride, &width, &height);
    TinyQrFrame frame = { pixels, width, height, stride };
    TinyQrResult result;
    static const uint8_t expected[] = "tinyqr-test";
    assert(tinyqr_decode(decoder, &frame, &result));
    assert(result.length == (int)sizeof(expected) - 1);
    assert(memcmp(result.data, expected, sizeof(expected) - 1) == 0);
    free(pixels);
}

/* Derived controls retain the original known payload; none represents the can photo. */
static void transformed_fixture_decodes(TinyQrDecoder *decoder, int transform) {
    int width, height;
    int stride = MODULE_COUNT * PIXELS_PER_MODULE;
    unsigned char *source = expand_fixture(stride, &width, &height);
    unsigned char *pixels = malloc((size_t)width * height);
    assert(pixels);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int value = source[y * stride + x];
            if (transform == 0) /* 90 degree rotation */
                value = source[(height - 1 - x) * stride + y];
            else if (transform == 1) { /* 3x3 box blur, white outside the image */
                int sum = 0;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx) {
                        int sx = x + dx, sy = y + dy;
                        sum += sx >= 0 && sx < width && sy >= 0 && sy < height
                            ? source[sy * stride + sx] : 255;
                    }
                value = sum / 9;
            } else if (transform == 2) /* Low contrast: luma 104 and 152 */
                value = value ? 152 : 104;
            else /* Mirror exercises the existing flip retry. */
                value = source[y * stride + width - 1 - x];
            pixels[y * width + x] = (unsigned char)value;
        }
    }
    TinyQrFrame frame = { pixels, width, height, width };
    TinyQrResult result;
    assert(tinyqr_decode(decoder, &frame, &result));
    assert(result.length == 11 && memcmp(result.data, "tinyqr-test", 11) == 0);
    printf("control %s passed\n", (const char *[]) {"rotated", "blurred", "low-contrast", "mirrored"}[transform]);
    free(pixels);
    free(source);
}

static void damaged_fixture_fails(TinyQrDecoder *decoder) {
    int width, height;
    int stride = MODULE_COUNT * PIXELS_PER_MODULE;
    unsigned char *pixels = expand_fixture(stride, &width, &height);
    /* Erase data modules while retaining all three finders and format bits. */
    for (int y = 13 * PIXELS_PER_MODULE; y < 25 * PIXELS_PER_MODULE; ++y)
        memset(pixels + y * stride + 13 * PIXELS_PER_MODULE, 255, 12 * PIXELS_PER_MODULE);
    TinyQrFrame frame = { pixels, width, height, stride };
    TinyQrResult result;
    assert(!tinyqr_decode(decoder, &frame, &result));
    assert_empty_result(&result);
    free(pixels);
}

static bool fixture_available(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return false;
    fclose(file);
    return true;
}

static unsigned char *load_pgm_fixture(const char *path, int *width, int *height) {
    FILE *file = fopen(path, "rb");
    char magic[3] = {0};
    int max_value = 0;
    assert(file);
    assert(fscanf(file, "%2s%d%d%d", magic, width, height, &max_value) == 4);
    assert(strcmp(magic, "P5") == 0 && *width > 0 && *height > 0 && max_value == 255);
    assert(fgetc(file) == '\n');
    unsigned char *pixels = malloc((size_t)*width * (size_t)*height);
    assert(pixels);
    assert(fread(pixels, 1, (size_t)*width * (size_t)*height, file) ==
           (size_t)*width * (size_t)*height);
    assert(fgetc(file) == EOF);
    fclose(file);
    return pixels;
}

static unsigned char *transform_photo(const unsigned char *source, int width, int height,
                                      int transform, int *out_width, int *out_height) {
    int rotated = transform == 0 || transform == 2;
    if (transform == 5) {
        const double cosine = cos(0.2617993877991494);
        const double sine = sin(0.2617993877991494);
        *out_width = (int)ceil(width * cosine + height * sine);
        *out_height = (int)ceil(width * sine + height * cosine);
    } else {
        *out_width = rotated ? height : width;
        *out_height = rotated ? width : height;
    }
    unsigned char *pixels = malloc((size_t)*out_width * (size_t)*out_height);
    assert(pixels);
    for (int y = 0; y < *out_height; ++y) {
        for (int x = 0; x < *out_width; ++x) {
            int sx = x, sy = y;
            if (transform == 0) { sx = y; sy = height - 1 - x; }
            else if (transform == 1) { sx = width - 1 - x; sy = height - 1 - y; }
            else if (transform == 2) { sx = width - 1 - y; sy = x; }
            else if (transform == 5) {
                const double cosine = cos(0.2617993877991494);
                const double sine = sin(0.2617993877991494);
                double dx = x - ((double)*out_width - 1) / 2;
                double dy = y - ((double)*out_height - 1) / 2;
                sx = (int)rint(cosine * dx + sine * dy + ((double)width - 1) / 2);
                sy = (int)rint(-sine * dx + cosine * dy + ((double)height - 1) / 2);
                if (sx < 0 || sx >= width || sy < 0 || sy >= height) {
                    pixels[y * *out_width + x] = 100;
                    continue;
                }
            }
            else if (transform == 3) { /* Small 3x3 blur. */
                int sum = 0;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx) {
                        int px = x + dx, py = y + dy;
                        sum += px >= 0 && px < width && py >= 0 && py < height
                            ? source[py * width + px] : 255;
                    }
                pixels[y * width + x] = (unsigned char)(sum / 9);
                continue;
            }
            if (transform == 4) { /* Preserve the photo's midpoint while reducing contrast. */
                int value = 128 + ((int)source[y * width + x] - 128) * 7 / 10;
                pixels[y * width + x] = (unsigned char)value;
                continue;
            }
            pixels[y * *out_width + x] = source[sy * width + sx];
        }
    }
    return pixels;
}

static void stylized_photo_decodes(TinyQrDecoder *decoder) {
    int width, height;
    unsigned char *source = load_pgm_fixture("tests/fixtures/coca_stylized_original.pgm", &width, &height);
    TinyQrFrame frame = { source, width, height, width };
    TinyQrResult reference;
    assert(tinyqr_decode(decoder, &frame, &reference));
    assert(reference.length > 0);
    unsigned char *expected = malloc((size_t)reference.length);
    assert(expected);
    memcpy(expected, reference.data, (size_t)reference.length);
    int expected_length = reference.length;
    for (int transform = 0; transform < 6; ++transform) {
        int transformed_width, transformed_height;
        unsigned char *pixels = transform_photo(source, width, height, transform,
                                                 &transformed_width, &transformed_height);
        frame = (TinyQrFrame){ pixels, transformed_width, transformed_height, transformed_width };
        TinyQrResult result;
        assert(tinyqr_decode(decoder, &frame, &result));
        assert(result.length == expected_length);
        assert(memcmp(result.data, expected, (size_t)expected_length) == 0);
        free(pixels);
    }
    free(expected);
    free(source);
}

static void stylized_negative_frames_fail(TinyQrDecoder *decoder) {
    enum { WIDTH = 192, HEIGHT = 192 };
    unsigned char pixels[WIDTH * HEIGHT];
    memset(pixels, 255, sizeof(pixels));
    const int centers[][2] = {{40, 40}, {152, 40}, {40, 152}, {152, 152}};
    for (int finder = 0; finder < 4; ++finder) {
        for (int y = centers[finder][1] - 16; y <= centers[finder][1] + 16; ++y) {
            for (int x = centers[finder][0] - 16; x <= centers[finder][0] + 16; ++x) {
                int dx = x - centers[finder][0], dy = y - centers[finder][1];
                int distance = dx * dx + dy * dy;
                if (distance <= 16 * 16 && distance >= 10 * 10) pixels[y * WIDTH + x] = 0;
                if (distance <= 5 * 5) pixels[y * WIDTH + x] = 0;
            }
        }
    }
    TinyQrFrame frame = { pixels, WIDTH, HEIGHT, WIDTH };
    TinyQrResult result;
    assert(!tinyqr_decode(decoder, &frame, &result));
    assert_empty_result(&result);
    for (int i = 0; i < WIDTH * HEIGHT; ++i)
        pixels[i] = (unsigned char)((i * 73 + i / WIDTH * 29 + 17) & 255);
    assert(!tinyqr_decode(decoder, &frame, &result));
    assert_empty_result(&result);
}

int main(int argc, char **argv) {
    const char *stylized_fixture = "tests/fixtures/coca_stylized_original.pgm";
    if (argc == 2 && strcmp(argv[1], "--benchmark-stylized") == 0 &&
        !fixture_available(stylized_fixture)) {
        fputs("Required stylized photo fixture is unavailable; benchmark not run.\n", stderr);
        return 2;
    }
    TinyQrDecoder *decoder = tinyqr_decoder_create();
    assert(decoder);
    if (argc == 2 && (strcmp(argv[1], "--benchmark") == 0 ||
                      strcmp(argv[1], "--benchmark-blank") == 0 ||
                      strcmp(argv[1], "--benchmark-stylized") == 0)) {
        int width, height;
        unsigned char *pixels;
        const char *label = "standard";
        if (strcmp(argv[1], "--benchmark-stylized") == 0) {
            pixels = load_pgm_fixture("tests/fixtures/coca_stylized_original.pgm", &width, &height);
            label = "stylized fallback";
        } else {
            pixels = expand_fixture(232, &width, &height);
            if (strcmp(argv[1], "--benchmark-blank") == 0) {
                memset(pixels, 255, (size_t)width * height);
                label = "blank failure";
            }
        }
        TinyQrFrame frame = { pixels, width, height, width };
        TinyQrResult result;
        bool expected = strcmp(argv[1], "--benchmark-blank") != 0;
        assert(tinyqr_decode(decoder, &frame, &result) == expected); /* Warm up / allocate. */
        clock_t start = clock();
        for (int i = 0; i < 1000; ++i)
            assert(tinyqr_decode(decoder, &frame, &result) == expected);
        printf("1000 %s scans: %.3f ms/scan (host CPU time)\n", label,
               1000.0 * (clock() - start) / CLOCKS_PER_SEC / 1000);
        free(pixels);
        tinyqr_decoder_destroy(decoder);
        return 0;
    }
    invalid_frames_fail(decoder);
    blank_frame_fails(decoder);
    fixture_decodes(decoder, 0);
    blank_frame_fails(decoder);
    fixture_decodes(decoder, 1);
    for (int transform = 0; transform < 4; ++transform)
        transformed_fixture_decodes(decoder, transform);
    damaged_fixture_fails(decoder);
    if (fixture_available(stylized_fixture)) {
        stylized_photo_decodes(decoder);
    } else {
        puts("SKIP: stylized photo regression fixture is unavailable.");
    }
    stylized_negative_frames_fail(decoder);
    fixture_decodes(decoder, 0);
    tinyqr_decoder_destroy(decoder);
    puts("qr decoder tests passed");
    return 0;
}

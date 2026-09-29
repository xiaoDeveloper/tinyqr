#include "../app/src/main/c/qr/qr.h"
#include <assert.h>

int main(void) {
    TinyQrFrame frame = {0};
    TinyQrResult result = {0};
    TinyQrDecoder *decoder = tinyqr_decoder_create();
    assert(decoder != 0);
    assert(!tinyqr_decode(decoder, &frame, &result));
    assert(result.data == 0 && result.length == 0);
    tinyqr_decoder_destroy(decoder);
    return 0;
}

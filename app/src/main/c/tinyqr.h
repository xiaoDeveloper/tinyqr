#ifndef TINYQR_H
#define TINYQR_H

#if defined(__ANDROID__)
#include <android/log.h>
#if defined(TINYQR_DEBUG) || !defined(NDEBUG)
#define TQ_LOG(...) __android_log_print(ANDROID_LOG_INFO, "TinyQR", __VA_ARGS__)
#else
#define TQ_LOG(...) ((void)0)
#endif
#elif defined(TINYQR_DEBUG)
/* Exercise the same diagnostic messages in the native desktop tests. */
#include <stdio.h>
#define TQ_LOG(...) do { fprintf(stderr, "TinyQR: "); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while (0)
#else
#define TQ_LOG(...) ((void)0)
#endif
#endif

#ifndef TIMAGE_H__
#define TIMAGE_H__

#include <stdint.h>

typedef struct {
	const uint8_t *data;
	uint16_t width;
	uint16_t height;
	uint8_t dataSize;
} tImage;

#endif // TIMAGE_H__
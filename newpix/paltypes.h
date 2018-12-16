#ifndef PALTYPES_H
#define PALTYPES_H

#include <cstdint>

typedef uint8_t channel_t;
typedef uint16_t palindex_t;

namespace palette
{
  static const uint16_t size = 0x0100;
}

struct color24_t
{
  channel_t blue;
  channel_t green;
  channel_t red;
};

struct color32_t
{
  channel_t blue;
  channel_t green;
  channel_t red;
  channel_t alpha;
};


// These pixel types take the endian order of the system into account.
typedef uint8_t pixel_t;
typedef uint16_t pixel16_t;

struct pixel24_t
{
  channel_t red;
  channel_t green;
  channel_t blue;

  constexpr bool operator==(const pixel24_t& other) const
  {
    return blue  == other.blue  &&
           green == other.green &&
           red   == other.red;
  }
};

struct pixel32_t
{
  channel_t alpha;
  channel_t red;
  channel_t green;
  channel_t blue;

  constexpr bool operator==(const pixel32_t& other) const
  {
    return blue  == other.blue  &&
           green == other.green &&
           red   == other.red   &&
           alpha == other.alpha;
  }
};

static_assert(sizeof(color24_t) == 3, "compiler fail!");
static_assert(sizeof(color32_t) == 4, "compiler fail!");
static_assert(sizeof(pixel24_t) == 3, "compiler fail!");
static_assert(sizeof(pixel32_t) == 4, "compiler fail!");

#endif // PALTYPES_H

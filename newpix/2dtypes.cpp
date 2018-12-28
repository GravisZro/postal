#include "2dtypes.h"

#include "3dtypes.h"

struct ImageHeader
{
  uint32_t type;      // Image type
  uint32_t typeDest;  // Type to convert to upon load (New version 2)
  uint32_t Size;      // Image data's size
  int16_t  WinWidth;  // Width of image
  int16_t  WinHeight; // Height of image
  int16_t  Width;     // Width of buffer (new version 2)
  int16_t  Height;    // Height of buffer (new version 2)
  int16_t  WinX;      // Position of image in the buffer
  int16_t  WinY;      // Position of image in the buffer
  int32_t  Pitch;     // Pitch of image
  int16_t  Depth;     // Color depth of image
};

struct SpriteHeader
{
  space3d_t<int16_t> dimensions;
  space3d_t<int16_t> hotspot;

  int16_t Angle;      // Angle of rotation
  int32_t Width;      // Sprite's Desired Display Width (for scale blit)
  int32_t Height;     // Sprite's Desired Display Height (for scale blit)

  space3d_t<double> position;
  space3d_t<double> acceleration;
  space3d_t<double> velocity;

  uint32_t Flags;     // Attribute and status flags
};

void ImageResource::load(void) noexcept
{
  struct format_t
  {
    uint32_t signature;  // filetype signature (should be "IM  "/"SPRT"/"BM")
    uint32_t version;   // version number (should be 5/3/NA)
  };


//  FileType == SPRITE_COOKIE
//  Version == SPRITE_CURRENT_VERSION

}

#include "2dtypes.h"

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
  int16_t X;          // Sprite X
  int16_t Y;          // Sprite Y
  int16_t Z;          // Sprite Z

  int16_t HotSpotX;   // The offset from m_sX to the hotspot.
                      //  This value would normally be subtracted from m_sX to get the destination (e.g., screen) position.
  int16_t HotSpotY;   // The offset from m_sY to the hotspot.
                      //  This value would normally be subtracted from m_sY to get the destination (e.g., screen) position.
  int16_t HotSpotZ;   // The offset from m_sZ to the hotspot.
                      //  This value would normally be subtracted from m_sZ to get the destination (e.g., screen) position.
  int16_t Angle;      // Angle of rotation
  int32_t Width;      // Sprite's Desired Display Width (for scale blit)
  int32_t Height;     // Sprite's Desired Display Height (for scale blit)

  double  Xpos;       // X Position
  double  Ypos;       // Y Position
  double  Zpos;       // Z Position

  double  Xacc;       // X Acceleration
  double  Yacc;       // Y Acceleration
  double  Zacc;       // Z Acceleration

  double  Xvel;       // X Velocity
  double  Yvel;       // Y Velocity
  double  Zvel;       // Z Velocity

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

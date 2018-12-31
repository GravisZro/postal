#ifndef FORCES_H
#define FORCES_H

#include "thing.h"
#include <sprites.h>

#include "2dtypes.h"
#include "3dtypes.h"
#include "3dmath.h"

static constexpr space3d_t<double> invalid_position = { UINT32_MAX, UINT32_MAX, UINT32_MAX };

class sprite_base_t
    : public CThing,
      virtual public CSprite
{
public:
  sprite_base_t(void) noexcept;
  virtual ~sprite_base_t(void) noexcept;

  virtual CSprite* GetSprite(void) final { return this; }

  virtual double GetX(void) const final { return position.x; }
  virtual double GetY(void) const final { return position.y; }
  virtual double GetZ(void) const final { return position.z; }

  space3d_t<double> position;     // 3d position
  space3d_t<double> rotation;     // 3d sprite rotation
  space2d_t<int16_t> position2d;  // 2d position

  uint16_t priority;              // sprite priority
  uint16_t layer;                 // sprite layer

/*
  struct flags_t
  {
    uint32_t Alpha          : 1; // Set if on alpha layer, clear otherwise
    uint32_t Opaque         : 1; // Set if on opaque layer, clear otherwise
    uint32_t Xrayee         : 1; // Set if xray target, clear otherwise
    uint32_t Hidden         : 1; // Set if hidden, clear otherwise
    uint32_t DeleteOnClear  : 1; // Set to delete sprite when layer is cleared
    uint32_t HighIntensity  : 1; // Set to use higher light intensities when
    uint32_t DeleteOnRender : 1; // After rendering object, delete it.
    uint32_t BlitOpaque     : 1; // Blit sprite opaque (currently only supported for 2D uncompressed, non-alpha objects).

    inline void clear(void) { *reinterpret_cast<uint32_t*>(this) = 0;}
  } flags;
*/
  signal<> SpriteUpdate;
private:
//  bool m_InScene;
  sprite_base_t* m_self;
};

#endif // FORCES_H

#ifndef FORCES_H
#define FORCES_H

#include "thing.h"
#include <sprites.h>

#include "2dtypes.h"
#include "3dtypes.h"
#include "3dmath.h"

static constexpr space3d_t<double> invalid_position = { UINT32_MAX, UINT32_MAX, UINT32_MAX };

struct sprite_data_t
{
  bool is_child;

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
};

struct render_cache_t
{
  int16_t            Brightness;      // Indicates the brightness with which the object will be fogged (0 .. 255).
  space3d_t<int16_t> DirectRender;    // Location to Render() directly to composite buffer.
  space2d_t<int16_t> IndirectRender;  // Location to Render() indirectly to clip buffer.
  space2d_t<int16_t> RenderOffset;    // Offset to Render() to account for bounding sphere.
};

class sprite_base_t
    : public CThing,
      public sprite_data_t,
      protected render_cache_t,
      virtual public CSprite
{
public:
  sprite_base_t(void) noexcept;
  virtual ~sprite_base_t(void) noexcept;

  CSprite* GetSprite(void) { return this; }

  double GetX(void) const { return position.x; }
  double GetY(void) const { return position.y; }
  double GetZ(void) const { return position.z; }

  bool isChild(void) const { return m_IsChild; }
  void addChild(managed_ptr<sprite_base_t>& _child);
  void removeChild(managed_ptr<sprite_base_t>& _child);

  signal<> SpriteUpdate;
private:
  sprite_base_t* m_self;
  std::set<managed_ptr<sprite_base_t>> m_children;
//  bool m_InScene;
  bool m_IsChild;
};

#endif // FORCES_H

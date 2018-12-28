#ifndef FORCES_H
#define FORCES_H

#include "thing.h"
#include <sprites.h>

#include "3dtypes.h"
#include "3dmath.h"


class sprite_base_t
    : public CThing,
      virtual public CSprite
{
public:
  sprite_base_t(void) noexcept;
  virtual ~sprite_base_t(void) noexcept;

  space3d_t<double> m_position;
  space3d_t<double> m_rotation;

  signal<> SpriteUpdate;
private:
  sprite_base_t* m_self;
};

#endif // FORCES_H

#include "sprite_base.h"

#include "realm.h"
#include <put/cxxutils/vterm.h>

sprite_base_t::sprite_base_t(void) noexcept
  : m_self(this)
{
  Object::connect(SpriteUpdate,
                  fslot_t<void>([this](void) noexcept
                  {
                    if(this == m_self) // ensure object data is valid (not destructed)
                      realm()->Scene()->UpdateSprite(dynamic_cast<CSprite*>(this));
                  }));
}


sprite_base_t::~sprite_base_t(void) noexcept
{
  realm()->Scene()->RemoveSprite(dynamic_cast<CSprite*>(this));
  m_self = nullptr;
}

#include "sprite_base.h"

#include "realm.h"
#include <put/cxxutils/vterm.h>

sprite_base_t::sprite_base_t(void) noexcept
  : m_self(this),
    m_IsChild(false)
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


void sprite_base_t::addChild(managed_ptr<sprite_base_t>& _child)
{
  ASSERT(!_child->isChild()); // ensure no double parenting
  _child->m_IsChild = true;
  m_children.insert(_child);
}

void sprite_base_t::removeChild(managed_ptr<sprite_base_t>& _child)
{
  _child->m_IsChild = false;
  m_children.erase(_child);
}

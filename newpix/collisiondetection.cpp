#include "collisiondetection.h"


Collidable::Collidable(void) noexcept
  : m_self(this)
{

}

Collidable::~Collidable(void) noexcept
{
  m_self = nullptr;
}

CollisionDetection::CollisionDetection(void) noexcept
{

}

CollisionDetection::~CollisionDetection(void) noexcept
{

}

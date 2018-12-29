#ifndef COLLISIONDETECTION_H
#define COLLISIONDETECTION_H

#include <newpix/sprite_base.h>

#include <smash.h>


class Collidable
    : public sprite_base_t
{
public:
  Collidable(void) noexcept;
  virtual ~Collidable(void) noexcept;


private:
  Collidable* m_self;
};

class CollisionDetection
{
public:
  CollisionDetection(void) noexcept;
  virtual ~CollisionDetection(void) noexcept;
};

#endif // COLLISIONDETECTION_H

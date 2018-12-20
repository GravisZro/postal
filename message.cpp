#include "message.h"

#include "flag.h"
#include "Thing3d.h"

GameMessage::GameMessage(const GameMessage& other) noexcept
{
  std::memcpy(this, &other, sizeof(GameMessage));
}

// Function to save whatever type of message this is.
int16_t GameMessage::Save(RFile* pFile)
{
  int16_t sResult = SUCCESS;

  if (pFile && pFile->IsOpen())
  {
    pFile->Write(&msg_Generic.eType);
    pFile->Write(&msg_Generic.sPriority);

    switch (msg_Generic.eType)
    {
      case typeGeneric:
      case typeObjectDelete:
      case typeSafeSpot:
      case typeSuicide:
      case typeTrigger:
      case typeWeaponSelect:
      case typeWeaponFire:
      case typeWrithing:
      case typeDeath:
      case typeCheater:
      case typeHelp:
        break;

      case typeShot:
        pFile->Write(&msg_Shot.sDamage);
        pFile->Write(&msg_Shot.sAngle);
        break;

      case typeExplosion:
        pFile->Write(&msg_Explosion.sDamage);
        pFile->Write(&msg_Explosion.sX);
        pFile->Write(&msg_Explosion.sY);
        pFile->Write(&msg_Explosion.sZ);
        pFile->Write(&msg_Explosion.sVelocity);
        break;

      case typeBurn:
        pFile->Write(&msg_Burn.sDamage);
        break;

      case typePopout:
      case typeShootCycle:
        pFile->Write(&msg_Popout.ucIDNext);
        pFile->Write(&msg_Popout.u16UniqueDudeID);
        pFile->Write(&msg_Popout.u16UniquePylonID);
        pFile->Write(&msg_Popout.sNextPylonX);
        pFile->Write(&msg_Popout.sNextPylonZ);
        break;

      case typePanic:
        pFile->Write(&msg_Panic.sX);
        pFile->Write(&msg_Panic.sY);
        pFile->Write(&msg_Panic.sZ);
        break;

      case typeDrawBlood:
        pFile->Write(&msg_DrawBlood.s2dX);
        pFile->Write(&msg_DrawBlood.s2dY);
        break;

      case typeDudeTrigger:
        pFile->Write(&msg_DudeTrigger.u16DudeUniqueID);
        pFile->Write(&msg_DudeTrigger.dX);
        pFile->Write(&msg_DudeTrigger.dZ);
        break;

      case typePutMeDown:
        uint16_t flag_id = msg_PutMeDown.flag->GetInstanceID();
        pFile->Write(&flag_id);
        break;
    }
  }
  else
  {
    sResult = FAILURE;
  }

  return sResult;
}

// Function to load whatever type of message was saved
int16_t GameMessage::Load(RFile* pFile)
{
  int16_t sResult = SUCCESS;

  if (pFile && pFile->IsOpen())
  {
    pFile->Read(&msg_Generic.eType);
    pFile->Read(&msg_Generic.sPriority);

    switch (msg_Generic.eType)
    {
      case typeGeneric:
      case typeObjectDelete:
      case typeSafeSpot:
      case typeSuicide:
      case typeTrigger:
        break;

      case typeShot:
        pFile->Read(&msg_Shot.sDamage);
        pFile->Read(&msg_Shot.sAngle);
        break;

      case typeExplosion:
        pFile->Read(&msg_Explosion.sDamage);
        pFile->Read(&msg_Explosion.sX);
        pFile->Read(&msg_Explosion.sY);
        pFile->Read(&msg_Explosion.sZ);
        pFile->Read(&msg_Explosion.sVelocity);
        break;

      case typeBurn:
        pFile->Read(&msg_Burn.sDamage);
        break;

      case typePopout:
      case typeShootCycle:
        pFile->Read(&msg_Popout.ucIDNext);
        pFile->Read(&msg_Popout.u16UniqueDudeID);
        pFile->Read(&msg_Popout.u16UniquePylonID);
        pFile->Read(&msg_Popout.sNextPylonX);
        pFile->Read(&msg_Popout.sNextPylonZ);
        break;

      case typePanic:
        pFile->Read(&msg_Panic.sX);
        pFile->Read(&msg_Panic.sY);
        pFile->Read(&msg_Panic.sZ);
        break;

      case typeDrawBlood:
        pFile->Read(&msg_DrawBlood.s2dX);
        pFile->Read(&msg_DrawBlood.s2dY);
        break;

      case typeDudeTrigger:
        pFile->Read(&msg_DudeTrigger.u16DudeUniqueID);
        pFile->Read(&msg_DudeTrigger.dX);
        pFile->Read(&msg_DudeTrigger.dZ);
        break;

      case typePutMeDown:
        uint16_t flag_id = 0;
        pFile->Read(&flag_id);
        //msg_PutMeDown.flag = well shit
        break;
    }
  }
  else
  {
    sResult = FAILURE;
  }

  return sResult;
}

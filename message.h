////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2016 RWS Inc, All Rights Reserved
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of version 2 of the GNU General Public License as published by
// the Free Software Foundation
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//
// message.h
// Project: Postal
//
//	History
//
//		02/15/97 BRH	Started this file to include a few messages that we will
//							start out with.  All of the messages have the type first
//							so that if they are cast as a Generic Message, you can 
//							look at the type first and do a switch statement to 
//							cast it to the correct type of message struct.  
//
//		02/16/97 BRH	We talked about doing a union of the structs but I'm
//							not sure how that makes it easier to use rather than
//							doing the casting like DirectPlay.  Also I was wondering
//							if these should have constructors that default their type
//							variable to their specific type rather than making the
//							person who creates a struct fill in the type.
//
//		02/17/97 BRH	Created another message structure with a union of the
//							other message types and included the type enum in
//							that structure.  Changed the Thing to use a MessageQueue
//							of this new structure rather than passing the generic
//							message structure pointer and casting it to the specific
//							type of message.
//
//		02/18/97	JMI	Moved MessageType enum to global scope.
//
//		02/19/97 BRH	Added ObjectDelete message which tells an object to
//							kill itself immediately (ie. delete this)
//
//		02/19/97 BRH	Changed the bogus extra data in the messages like
//							burn heat, explosion force, bullet caliber to sDamage 
//							so that all of these can give some kind of game relative
//							damage amount.
//
//		02/24/97	JMI	Added generic notification message with generic parameters.
//							Useful for simple messages within an object and stuff like
//							that where it seems like defining a whole new message
//							would be silly.
//
//		03/04/97	JMI	Well, I guess it wasn't that silly after all.  Added a
//							DrawBlood_Message which I should've done to begin with
//							instead of adding that silly notification message.
//
//		03/05/97 BRH	Added center and velocity of explosion to the explosion
//							message.  Also added panic message which is sent by 
//							victims to other victims to warn them.
//
//		03/05/97	JMI	Added Suicide_Message.
//
//		03/06/97	JMI	Added msg_Suicide to GameMessage union.
//
//		03/19/97 BRH	Added Trigger message so that the CDude can trigger
//							the remote controlled mine or other weapons that we
//							may decide require an external trigger.
//
//		04/20/97 BRH	Added bouy strategy suggestion messages.  Bouys can
//							be assigned a suggestion message that an enemy can
//							ask the bouy to send in order to get information from
//							the bouy to perform certain local specific logic.
//
//		04/20/97 BRH	Added load and save functions to messages so that the
//							bouys could use them to save and load the messages.
//
//		05/02/97 BRH	Added a message for the dude trigger which informs
//							a pylon that a CDude stepped on the trigger area.
//							Also changed the Popout and ShootCycle messages to 
//							contain more information.
//
//		06/09/97 BRH	Added Messages for CDude to send to the CDemon to inform
//							of his actions like selecting a new weapon and firing
//							a weapon.
//
//		06/10/97 BRH	Added Death and Writhing messages so CPerson can send
//							CDemon messsages for comments.
//
//		06/11/97 BRH	Added shooter to the weapon messages so that we can
//							identify the shooter of the weapon so that credit can
//							be given to whoever gets a kill using a weapon.
//
//		08/01/97 BRH	Added a cheater message to send to the Demon when someone
//							enters a cheat code.
//
//		08/02/97 BRH	Added a call for help message that enemies send to each other
//							when they are getting shot.
//
//		08/28/97 BRH	Added PutMeDown message that the flag sends to the dude 
//							when it finds the flagbase.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef MESSAGE_H
#define MESSAGE_H

#include <RSPiX.h>
//#include "dude.h"

#include <newpix/managedpointer.h>

////////////////////////////////////////////////////////////////////////////////
// Message Types
////////////////////////////////////////////////////////////////////////////////

typedef uint8_t MessageType;

enum
{
	typeGeneric,
	typeShot,
	typeExplosion,
	typeBurn,
   typeKill,
   typeObjectDelete,
	typeNotification,		// Generic notification with generic parameters for
								// reuse.  Delete this silly message!
	typeDrawBlood,			// Message indicating blood should be drawn on the
								// background at the specified location.
	typePanic,
	typeSuicide,			// Receiver should commit suicide.
	typeTrigger,			// Used to trigger remote control mines
	typePopout,				// Used by bouy to give direction and distance for popout logic
	typeShootCycle,		// Used by bouy to give next bouy ID for cycling between
	typeSafeSpot,			// Used by bouy to identify itself as a "behind cover" bouy
	typeDudeTrigger,		// Used by attribute trigger to tell pylons that a Dude is near
	typeWeaponSelect,		// Used by CDude to tell CDemon when a new weapon has been selected
	typeWeaponFire,		// Used by CDude to tell CDemon when a weapon has been fired.
	typeWrithing,			// Used by CPerson to tell CDemon when a person is writhing
	typeDeath,				// Used by CPerson to tell CDemon when a person has died
	typeCheater,			// Used by CDude to tell CDemon when a person entered a cheat code
								// so that it can mock them.
	typeHelp,				// Used by CPerson to tell others that it it getting shot
	typePutMeDown,			// Used by flag to tell CDude to put it down in the base.

	typeEndOfMessages
};


////////////////////////////////////////////////////////////////////////////////
// Message structures
////////////////////////////////////////////////////////////////////////////////

class CThing3d;
class CFlag;

struct Generic_Message
{
	MessageType eType; // = typeGeneric
	int16_t			sPriority;
};

struct Shot_Message
{
	MessageType eType;// = typeShot;
	int16_t			sPriority;
	int16_t			sDamage;
	int16_t			sAngle;
   managed_ptr<CThing3d> shooter; // Instance ID of the shooter (for scoring)
};

struct Explosion_Message
{
	MessageType eType; // = typeExplosion;
	int16_t			sPriority;
	int16_t			sDamage;
	int16_t			sX;			// Center of explosion
	int16_t			sY;			// Center of explosion
	int16_t			sZ;			// Center of explosion
	int16_t			sVelocity;	// Relative size of explosion 

   managed_ptr<CThing3d> shooter;// Instance ID of the shooter
};

struct Burn_Message
{
	MessageType eType; // = typeBurn;
	int16_t			sPriority;
	int16_t			sDamage;
   managed_ptr<CThing3d> shooter;	// Instance ID of the shooter
};

struct ObjectDelete_Message
{
	MessageType eType; // = typeObjectDelete
	int16_t			sPriority;
};

struct Popout_Message
{
	MessageType eType; // = typePopout
	int16_t			sPriority;
	uint8_t			ucIDNext;			// Next Pylon to run to
	uint16_t			u16UniqueDudeID;	// Dude to target
	uint16_t			u16UniquePylonID;	// Easier way to get to pylon data
	int16_t			sNextPylonX;		// Next pylon X position
	int16_t			sNextPylonZ;		// Next pylon Z position
};

struct ShootCycle_Message
{
	MessageType eType; // = typeShootCycle
	int16_t			sPriority;
	uint8_t			ucIDNext;			// ID of next bouy in cycle
	uint16_t			u16UniqueDudeID;	// Dude to target
	uint16_t			u16UniquePylonID;	// Easier way to get to pylon data
	int16_t			sNextPylonX;		// Next pylon X position
	int16_t			sNextPylonZ;		// Next pylon Z position
};

struct SafeSpot_Message
{
	MessageType eType; // = typeSafeSpot
	int16_t			sPriority;
};

struct Panic_Message
{
	MessageType eType;	//typePanic
	int16_t			sPriority;
	int16_t			sX;		// Center of panic
	int16_t			sY;		// Center of panic
	int16_t			sZ;		// Center of panic
};

struct DrawBlood_Message
{
	MessageType	eType;	// = typeDrawBlood
	int16_t			sPriority;
	int16_t			s2dX;		// 2D X position on background to draw blood.
	int16_t			s2dY;		// 2D Y position on background to draw blood.
};

struct Suicide_Message
{
	MessageType	eType;	// = typeSuicide
	int16_t			sPriority;
};

struct Trigger_Message
{
	MessageType eType;	// = typeTrigger
	int16_t			sPriority;
};

struct DudeTrigger_Message
{
	MessageType eType; // = typeDudeTrigger
	int16_t		   sPriority;
	uint16_t			u16DudeUniqueID;
	double		dX;
	double		dZ;
};

struct WeaponSelect_Message
{
	MessageType	eType; // = typeWeaponSelect
	int16_t			sPriority;
	int16_t			sWeapon; //CDude::WeaponType eWeapon;
};

struct WeaponFire_Message
{
	MessageType	eType; // = typeWeaponFire
	int16_t			sPriority;
	int16_t			sWeapon; //CDude::WeaponType	eWeapon;
};

struct Writhing_Message
{
	MessageType	eType; // = typeWrithing
	int16_t			sPriority;	
};

struct Death_Message
{
	MessageType	eType; // = typeDeath
	int16_t			sPriority;
};

struct Cheater_Message
{
	MessageType eType;// = typeCheater
	int16_t			sPriority;
};

struct Help_Message
{
	MessageType eType; // = typeHelp
	int16_t			sPriority;
};

struct PutMeDown_Message
{
	MessageType eType;// = typePutMeDown
	int16_t			sPriority;
   managed_ptr<CFlag> flag;
};

struct GameMessage
{
union
	{
		Generic_Message		msg_Generic;
		Shot_Message			msg_Shot;
		Explosion_Message		msg_Explosion;
		Burn_Message			msg_Burn;
		ObjectDelete_Message	msg_ObjectDelete;
		DrawBlood_Message		msg_DrawBlood;
		Panic_Message			msg_Panic;
		Suicide_Message		msg_Suicide;
		Trigger_Message		msg_Trigger;
		Popout_Message			msg_Popout;
		ShootCycle_Message	msg_ShootCycle;
		SafeSpot_Message		msg_SafeSpot;
		DudeTrigger_Message	msg_DudeTrigger;
		WeaponSelect_Message msg_WeaponSelect;
		WeaponFire_Message	msg_WeaponFire;
		Writhing_Message		msg_Writhing;
		Death_Message			msg_Death;
		Cheater_Message		msg_Cheater;
		Help_Message			msg_Help;
		PutMeDown_Message		msg_PutMeDown;

	};

  GameMessage(void) noexcept  { }
  ~GameMessage(void) noexcept { }
  GameMessage(const GameMessage& other) noexcept;

	// Function to save whatever type of message this is.
   int16_t Save(RFile* pFile);

	// Function to load whatever type of message was saved
   int16_t Load(RFile* pFile);
};


////////////////////////////////////////////////////////////////////////////////
// Message union
////////////////////////////////////////////////////////////////////////////////


#endif //MESSAGE_H

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////

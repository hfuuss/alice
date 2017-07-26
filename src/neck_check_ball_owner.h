// -*-c++-*-

/*!
  \file neck_check_ball_owner.h
  \brief change_view and turn_neck to look the ball owner.
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifndef HELIOS_NECK_CHECK_BALL_OWNER_H
#define HELIOS_NECK_CHECK_BALL_OWNER_H

#include <rcsc/player/soccer_action.h>

// added 2008-07-07

/*!
  \class Neck_CheckBallOwner
  \brief turn_neck (and change_view, if necessary) to look the ball owner
*/
class Neck_CheckBallOwner
    : public rcsc::NeckAction {
public:
    /*!
      \brief constructor
     */
    Neck_CheckBallOwner()
      { }

    /*!
      \brief do turn neck (and change_view, if necessary)
      \param agent agent pointer to the agent itself
      \return always true
     */
    bool execute( rcsc::PlayerAgent * agent );

    /*!
      \brief create cloned object
      \return create pointer to the cloned object
    */
    rcsc::NeckAction * clone() const;
};

#endif

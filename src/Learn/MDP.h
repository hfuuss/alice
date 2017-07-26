// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
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

#ifndef MDP_H
#define MDP_H
#include "../chain_action/cooperative_action.h"
#include <rcsc/player/world_model.h>

//存入历史动作发生时候的世界模型

//struct  Action_History_WorldModel 
//{ 
   // rcsc::WorldModel  &m_history_wm;
  
//}
//;

enum  AfterKickActionType 
{
    //成功 奖励的情形
    ShootGoalSucc,
    PassBallSucc,
    DribbleSucc,
    HoldSucc,
    //失败 惩罚的情形
    ShootFailed,
    PassFailed,
    DribbleFailed,
    HoldFailed
};

//TODO: 防守动作失败的具体定义,合成防守动作是前提 


enum  DefensiveMoveActionType
{
  
  
  
  
};
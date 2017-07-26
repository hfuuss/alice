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

#ifndef MDP_KICKER_ACTION_HISTORY_H
#define MDP_KICKER_ACTION_HISTORY_H
#include "../chain_action/cooperative_action.h"
#include "MDP.h"
#include<rcsc/player/world_model.h>



struct  Kick_action_history_info 

{
  int  m_cycle;
  int  action_sort;//这是第几个动作 
  
  CooperativeAction m_action;//历史动作 
  rcsc::WorldModel  m_history_world_model;//历史场景 
  
  AfterKickActionType m_result_type; //历史动作的结果
  double   award;//奖励值 
  
};


class  mdp_kicker_action_history 
{
public:
   mdp_kicker_action_history(Kick_action_history_info  in_put):
   M_kick_action_history_info(in_put)
   {}
   
   //don't use  
   mdp_kicker_action_history(const mdp_kicker_action_history & copy)
   {
     M_kick_action_history_info=copy.M_kick_action_history_info;
   }
   
   //don't use 
   void  operator=(const mdp_kicker_action_history &a) 
   {
     M_kick_action_history_info=a.M_kick_action_history_info;
   }
   
   
private: 
     void  Record();
     void  Sort_Info(int i)
     {
       return  save_all[i];
     }
     
   
private:
  
  Kick_action_history_info  M_kick_action_history_info;
  
  static Kick_action_history_info   save_all[6000];
  static int info_count;
  
  
  
  
  
  
  
  
  
};


#endif 
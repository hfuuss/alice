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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Brain.h"

#include "field_analyzer.h"
#include "simple_pass_checker.h"
#include "cooperative_action.h"
#include <rcsc/player/player_evaluator.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/logger.h>
#include <rcsc/math_util.h>
#include <rcsc/player/intercept_table.h>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <boost/concept_check.hpp>
#include <rcsc/geom/voronoi_diagram.h>
#include <fstream>
#include<map>
#include<utility>
#include "Evaluation.h"
// #define DEBUG_PRINTUntitled Folder


#define  ERROR_VALUE   0

using namespace rcsc;

static const int VALID_PLAYER_THRESHOLD = 8;






double
Brain::operator()( const PredictState & state,
                                  const std::vector< ActionStatePair > & path )
{
    double  result=0;


    if (path.size()==0)
    {
        return result;
    }
    const CooperativeAction  &first_action =path.front().action();

    // 0根节点判断自己能不能射门

    if (first_action.category()==CooperativeAction::Shoot)
    {

        return  10e+10;
    }

    int count=0;
    
    if (path[0].action().category()==CooperativeAction::Shoot) 
    {
      result+=10e+8;
    }
    else if (path.size()==2&&path[1].action().category()==CooperativeAction::Shoot)
    {
      result+=10e+5;
    }


    for ( std::vector< ActionStatePair >::const_iterator it =path.begin(); it!=path.end(); it++)
    {

          result+=decesion_tree(state,(*it));
          count++;
    }








    return result/count;
}


/*-------------------------------------------------------------------*/
/*!

 */
double
Brain::evaluate_state( const PredictState & state )
{
    const ServerParam & SP = ServerParam::i();

    const AbstractPlayerObject * holder = state.ballHolder();

#ifdef DEBUG_PRINT
    dlog.addText( Logger::ACTION_CHAIN,
                  "========= (evaluate_state) ==========" );
#endif

    //
    // if holder is invalid, return bad evaluation
    //
    if ( ! holder )
    {

        return -1000;
    }

    if ( holder->side()!=state.ourSide())
    {

        return   -1000;
    }





    const int holder_unum = holder->unum();




    if ( state.ball().pos().x < - ( SP.pitchHalfLength() - 0.1 )
         && state.ball().pos().absY() < SP.goalHalfWidth() )
    {

        return -1000;
    }


    //
    // out of pitch
    //
    if ( state.ball().pos().absX() > SP.pitchHalfLength()
         || state.ball().pos().absY() > SP.pitchHalfWidth() )
    {
#ifdef DEBUG_PRINT
        dlog.addText( Logger::ACTION_CHAIN,
                      "(eval) XXX out of pitch" );
#endif

        return - 1000;
    }



    //
    // set basic evaluation
    //




    double    point=Evaluation::instance().EvaluatePosition(state.ball().pos(),
                                                            holder->side()==state.ourSide())
                *100;

//    std::cout<<state.ball().pos()  <<"   EVA   "<<point<<std::endl;

    // in our goal  penalty

    if (state.ball().pos().x<-35.0&&state.ball().pos().absY()<20)
    {

        point-=1000;
    }


    if ( FieldAnalyzer::can_shoot_from
         ( holder->unum() == state.self().unum(),
           holder->pos(),
           state.getPlayerCont( new OpponentOrUnknownPlayerPredicate( state.ourSide() ) ),
           VALID_PLAYER_THRESHOLD ) )
    {
        point += 1.0e+6;
#ifdef DEBUG_PRINT
        dlog.addText( Logger::ACTION_CHAIN,
                      "(eval) bonus for goal %f (%f)", 1.0e+6, point );
#endif

        if ( holder_unum == state.self().unum() )
        {
            point += 5.0e+5;
#ifdef DEBUG_PRINT
            dlog.addText( Logger::ACTION_CHAIN,
                          "(eval) bonus for goal self %f (%f)", 5.0e+5, point );
#endif
        }
    }



    return point;
}


double
Brain::decesion_tree( const PredictState & state,const ActionStatePair  path )
{


    const CooperativeAction  &first_action =path.action();

    double basic_eva=evaluate_state(state);




    const Vector2D  targetBall=state.ball().pos();
    const Vector2D  currentBall=state.current_wm()->ball().pos();


    int spent_time=first_action.category()==CooperativeAction::Dribble?
            (targetBall-currentBall).r()/first_action.firstBallSpeed():0;

    double  opp_dist_target_ball = FieldAnalyzer::get_opponent_dist(state,targetBall,spent_time,3);
    double  opp_dist_current_ball= FieldAnalyzer::get_opponent_dist(state,currentBall,0,3);

    const int self_min=state.current_wm()->interceptTable()->selfReachCycle();

    const int opp_min= state.current_wm()->interceptTable()->opponentReachCycle();


    //0.1  判断 球是否绝对安全

    bool  very_safe = opp_dist_target_ball>4.00? true:false;


    double evaluation=0;

    if (currentBall.x<36.0)
    {

        evaluation=less36_decesion_tree(state,path);
    }
    else
    {

        evaluation=over36_decesion_tree(state,path);
    }


    return evaluation;
}



double

Brain::less36_decesion_tree(const PredictState & state,const ActionStatePair path)
{

    const CooperativeAction  &first_action =path.action();
    double basic_eva=evaluate_state(state);



    const Vector2D  targetBall=state.ball().pos();
    const Vector2D  currentBall=state.current_wm()->ball().pos();

    const double targetBall_dist_goal =targetBall.dist(ServerParam::i().theirTeamGoalPos());

    int spent_time= 0;  /*first_action.category()==CooperativeAction::Dribble?first_action.durationStep():0;*/

    double  opp_dist_target_ball = FieldAnalyzer::get_opponent_dist(state,targetBall,spent_time,3);
    double  opp_dist_current_ball= FieldAnalyzer::get_opponent_dist(state,currentBall,0,3);

    const int self_min=state.current_wm()->interceptTable()->selfReachCycle();

    const int opp_min= state.current_wm()->interceptTable()->opponentReachCycle();


    //0.1  判断 球是否绝对安全

    bool  safe = opp_dist_target_ball>4.00? true:false;

    double p=basic_eva;




    if (currentBall.x>36.0)
    {

        return  ERROR_VALUE;
    }


    bool  over_defensive_line=state.ball().pos().x>state.theirDefensePlayerLineX()-1.0? true:false;


    if (safe) //0.0
    {

        double  basic_safe_bonus=10;
        p+=basic_safe_bonus;

        if (over_defensive_line) //0.1.0
        {

            double  basic_over_defensive_line_bonus=100;
            p+=basic_over_defensive_line_bonus;

            double attackle_line_bonus=(110-targetBall_dist_goal)*10;
            p+=attackle_line_bonus;

            return p;
        }

         if(targetBall.x>currentBall.x) //推进0.1.1
         {
                double basic_push_ball_bonus=50;
                p+=basic_push_ball_bonus;

                double  push_x_bonus= (targetBall.x-currentBall.x)*10;
                p+=push_x_bonus;
                return p;

         }



         return p;

    }

    if (!safe)
    {


        double  basic_try_safe_bonus=bound(0.0, opp_dist_target_ball,4.0)*0.25;

        p+=basic_try_safe_bonus;

        double  success_prob= opp_dist_target_ball<2.5? 0.0 : 1-   ( (4-opp_dist_target_ball)/4);

        if (over_defensive_line) //0.1.0
        {

            double  basic_try_over_defensive_line_bonus=100*success_prob;
            p+=basic_try_over_defensive_line_bonus;

            double attackle_line_bonus=  first_action.category()==CooperativeAction::Pass ? (110-targetBall_dist_goal)*10:
                                                                                             first_action.firstBallSpeed()*15;


            p+=attackle_line_bonus*success_prob;

            return p;
        }

         if(targetBall.x>currentBall.x) //推进0.1.1
         {
                double try_basic_push_ball_bonus=50*success_prob;
                p+=try_basic_push_ball_bonus;

                double  try_push_x_bonus= (targetBall.x-currentBall.x)*10*success_prob;
                p+=try_push_x_bonus;
                return p;

         }



         return p;

    }




   return p;

}





double

Brain::over36_decesion_tree(const PredictState & state,const ActionStatePair path)
{

    const CooperativeAction  &first_action =path.action();

    double basic_eva=evaluate_state(state);




    const Vector2D  targetBall=state.ball().pos();
    const Vector2D  currentBall=state.current_wm()->ball().pos();


    int spent_time= 0;  /*first_action.category()==CooperativeAction::Dribble?first_action.durationStep():0;*/

    double  opp_dist_target_ball = FieldAnalyzer::get_opponent_dist(state,targetBall,spent_time,3);
    double  opp_dist_current_ball= FieldAnalyzer::get_opponent_dist(state,currentBall,0,3);

    const int self_min=state.current_wm()->interceptTable()->selfReachCycle();

    const int opp_min= state.current_wm()->interceptTable()->opponentReachCycle();


    //0.1  判断 球是否绝对安全

    bool  safe = opp_dist_target_ball>4.00? true:false;

    double p=basic_eva;




    if (currentBall.x<36.0)
    {

        return  ERROR_VALUE;
    }




    bool  over_defensive_line=state.ball().pos().x>state.theirDefensePlayerLineX()-1.0? true:false;
    const double targetBall_dist_goal =targetBall.dist(ServerParam::i().theirTeamGoalPos());


    if (safe) //0.0
    {

        double  basic_safe_bonus=10;
        p+=basic_safe_bonus;

       return p;

    }

    if (!safe)
    {


        double  basic_try_safe_bonus=bound(0.0, opp_dist_target_ball,4.0)*2.5;

        p+=basic_try_safe_bonus;

        double  success_prob= opp_dist_target_ball<2.5? 0.0 : 1- (4-opp_dist_target_ball)/4;



         return p;

    }




   return p;

}





bool
Brain::is_bed_pass(const ActionStatePair path)
{
//    const CooperativeAction  &first_action =path.action();
//    const PredictState & state=path.state();

//    double basic_eva=evaluate_state(path.state());



//    const Vector2D  receive_pos=state.ball().pos();
//    const Vector2D  first_ball_pos=state.current_wm()->ball().pos();

//    static const double CONTROL_AREA_BUF = 0.15;  // buffer for kick table

//    const ServerParam & SP = ServerParam::i();



//    const AbstractPlayerCont::const_iterator end = state.theirPlayers().end();
//    for ( AbstractPlayerCont::const_iterator o = state.theirPlayers().begin();
//          o != end;
//          ++o )
//    {

//        if (!(*o)) continue;

//        const Vector2D opponent_pos=(*o)->pos();

//        if (opponent_pos.x<35.0) continue;

//          double max_x=std::max(first_ball_pos.x,receive_pos.x);
//          double min_x=std::min(first_ball_pos.x,receive_pos.x);
//          //4.0 as  a buff

//          if (opponent_pos.x>max_x) continue;
//          if (opponent_pos.x<min_x) continue;

//          double max_y=std::max(first_ball_pos.y,receive_pos.y);
//          double min_y=std::min(first_ball_pos.y,receive_pos.y);

//          if ( opponent_pos.y>max_y)  continue;
//          if ( opponent_pos.y<min_y)  continue;


//          // pass line
//           Segment2D passLine(first_ball_pos,receive_pos);

//         if (passLine.contains(opponent_pos) ||passLine.dist( opponent_pos)<1.2)
//         {

//            return true;
//         }



//      }





    return false;
}


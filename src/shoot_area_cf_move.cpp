

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>

#include <rcsc/action/body_intercept.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_goalie_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/action/view_wide.h>


#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/debug_client.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

#include<rcsc/player/say_message_builder.h>
#include <rcsc/common/audio_memory.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include "neck_default_intercept_neck.h"
#include "neck_offensive_intercept_neck.h"
#include "bhv_chain_action.h"
#include "field_analyzer.h"
#include<rcsc/geom/voronoi_diagram.h>
#include <rcsc/action/body_go_to_point_dodge.h>
#include "shoot_area_cf_move.h"
#include "neck_offensive_intercept_neck.h"
#include "bhv_basic_tackle.h"
#include "strategy.h"
#include"bhv_basic_move.h"
#include "voron_point.h"
#include <fstream>
#include<rcsc/player/soccer_intention.h>


using namespace  rcsc;





static int  VALID_PLAYER_THRESHOLD=8;
bool shoot_area_cf_move::execute(PlayerAgent* agent)
{
    const WorldModel &wm =agent->world();
    const int self_min=wm.interceptTable()->selfReachCycle();
    const int mate_min=wm.interceptTable()->teammateReachCycle();
    const int opp_min=wm.interceptTable()->opponentReachCycle();
    Vector2D home_pos=Strategy::i().getPosition(11);

    if ( Bhv_BasicTackle().execute( agent ) )
    {
        return true;
    }

    //press  opp ball owner


    if (!wm.existKickableTeammate()&&self_min<=mate_min)
    {
        Body_Intercept().execute(agent);
        agent->setNeckAction(new Neck_OffensiveInterceptNeck());
        return true;
    }



   // if (throughPassShootHelper(agent))
   // {

      //  return true;
   // }



    if (opp_min+3<mate_min)
    {
        Bhv_BasicMove().execute(agent);
        return true;
    }


    //  if (miracle_rec_pass_shoot_move().execute(agent))
    //  {
    //      return true;
    //  }





    //----------------------------------------------
    // set target
    rcsc::Vector2D target_point = home_pos;



    target_point.x=wm.offsideLineX();


    const VoronoiDiagram &M_Pass_Voron =voron_point::instance().passVoronoiDiagram(agent);
    
    std::vector<Vector2D> point_on_vn_segment;
    
    M_Pass_Voron.getPointsOnSegments(1.5,6,&point_on_vn_segment);
    
    VoronoiDiagram::Vector2DCont Voron=M_Pass_Voron.resultPoints();
    double best_point=-1000;
    Vector2D best_pos=target_point;
    int count=0;
    for (std::vector<Vector2D>::const_iterator it =point_on_vn_segment.begin();it!=point_on_vn_segment.end();it++)
    {
      
      
      
       //std::cout<<(*it)<<std::endl;


        if(! (*it).isValid())
        {
            continue;
        }


        if ((*it).absX()>52||(*it).absY()>34)
        {
            continue;
        }


        if ( (*it).x>wm.offsideLineX())
        {

            continue;
        }







        const double dist_to_opp_goal_weight=5.0;
        count++;
        double my_point=evaluateTargetPoint(count,wm,home_pos,wm.ball().pos(),(*it));

        if (my_point>best_point)
        {
            best_point=my_point;
            best_pos=(*it);
        }



    }



    target_point=best_pos;

  //  agent->doPointto(best_pos.x,best_pos.y);



    
    
    //     //!ckeck thourgh pass
    //    // !min cycle to run stright line
    
    //     if (wm.ball().pos().x>35&&wm.self().pos().x<35)
    //     {
    //       target_point.y = wm.self().pos().y;

    //     }



    //     /// !fast  comme forward
    //     if (wm.self().pos().x<home_pos.x-1.5&&wm.ball().pos().x>wm.theirDefenseLineX())
    //     {
    //       target_point.y =wm.self().pos().y;
    //     }



    double dash_power = rcsc::ServerParam::i().maxPower();
    double dist_thr = wm.ball().distFromSelf()*0.1;
    
    if (dist_thr<1) dist_thr=0.5;


    if ( ! rcsc::Body_GoToPoint( target_point, dist_thr, dash_power,
                                 -1.0, // speed
                                 1, // perferd cycle
                                 true, // save recovery
                                 20.0 // dir thr
                                 ).execute( agent ) )

    {

        rcsc::Body_TurnToBall().execute(agent);
    }


    



    agent->setNeckAction( new Neck_TurnToBallOrScan( 0 ) );
    agent->setArmAction( new Arm_PointToPoint( target_point ) );

    return true;
}







/*-------------------------------------------------------------------*/
/*!

 */
double
shoot_area_cf_move::evaluateTargetPoint( const int count,
                                    const WorldModel & wm,
                                    const Vector2D & home_pos,
                                    const Vector2D & ball_pos,
                                    const Vector2D & move_pos )
{
    const double var_factor_opponent_dist = 1.0 / ( 2.0 * std::pow( 3.0, 2 ) );
    const double var_factor_teammate_dist = 1.0 / ( 2.0 * std::pow( 2.0, 2 ) );

#ifndef DEBUG_PRINT
    (void)count;
    (void)home_pos;
#endif

    double shoot_rate = 1.0;
     if (!FieldAnalyzer::can_shoot_from
                ( false,
                 move_pos,
                  wm.getPlayerCont( new OpponentOrUnknownPlayerPredicate( wm.ourSide() ) ),
                  VALID_PLAYER_THRESHOLD ) )
    {
        shoot_rate = 0.9;
    }

    const int opp_count = opponents_in_cross_cone( wm, ball_pos, move_pos );
    double cross_rate = std::pow( 0.9, opp_count );

    double opponent_rate = 1.0;
    if ( ! wm.opponentsFromSelf().empty() )
    {
        for ( PlayerPtrCont::const_iterator o = wm.opponentsFromSelf().begin(),
                  end = wm.opponentsFromSelf().end();
              o != end;
              ++o )
        {
            double d2 = std::max( 0.001, (*o)->pos().dist2( move_pos ) );
            double r = 1.0 - ( std::exp( -d2 * var_factor_opponent_dist )
                               * std::pow( 0.98, (*o)->posCount() ) );
            if ( r < opponent_rate )
            {
                opponent_rate = r;
            }
        }
    }

    double teammate_rate = 1.0;
    if ( ! wm.teammatesFromSelf().empty() )
    {
        for ( PlayerPtrCont::const_iterator t = wm.teammatesFromSelf().begin(),
                  end = wm.teammatesFromSelf().end();
              t != end;
              ++t )
        {
            double d2 = (*t)->pos().dist2( move_pos );
            double r = 1.0 - std::exp( -d2 * var_factor_teammate_dist );
            if ( r < teammate_rate )
            {
                teammate_rate = r;
            }
        }
    }

    double home_dist_rate = std::exp( -home_pos.dist2( move_pos ) / ( 2.0 * std::pow( 4.0, 2 ) ) );
    double ball_dist_rate = std::exp( -ball_pos.dist2( move_pos ) / ( 2.0 * std::pow( 100.0, 2 ) ) );

    double score = 100.0 * shoot_rate * cross_rate * opponent_rate * teammate_rate * home_dist_rate * ball_dist_rate;



    return score;
}


/*-------------------------------------------------------------------*/
/*!

 */
int
shoot_area_cf_move::opponents_in_cross_cone( const WorldModel & wm,
                                        const Vector2D & ball_pos,
                                        const Vector2D & move_pos )
{
    if ( wm.opponentsFromBall().empty() )
    {
        return 0;
    }

    const double ball_move_dist = ball_pos.dist( move_pos );

    if ( ball_move_dist > 20.0 )
    {
        return 1000;
    }

    const ServerParam & SP = ServerParam::i();

    const Vector2D ball_vel = ( move_pos - ball_pos ).setLengthVector( SP.ballSpeedMax() );
    const AngleDeg ball_move_angle = ball_vel.th();

#ifdef DEBUG_PRINT_CROSS_CONE
    dlog.addText( Logger::TEAM,
                  "(opponents_in_cross_cone) ball(%.1f %.1f) move_angle=%.1f",
                  ball_pos.x, ball_pos.y, ball_move_angle.degree() );
#endif

    int count = 0;
    for ( PlayerPtrCont::const_iterator o = wm.opponentsFromBall().begin(),
              end = wm.opponentsFromBall().end();
          o != end;
          ++o )
    {
        if ( (*o)->ghostCount() >= 2 ) continue;
        if ( (*o)->posCount() >= 5 ) continue;

        const Vector2D opp_pos = (*o)->inertiaFinalPoint();
        const double opp_dist = opp_pos.dist( ball_pos );

        const double control_area = ( (*o)->goalie()
                                      ? SP.catchableArea()
                                      : (*o)->playerTypePtr()->kickableArea() );
        const double hide_radian = std::asin( std::min( control_area / opp_dist, 1.0 ) );
        double angle_diff = ( ( opp_pos - ball_pos ).th() - ball_move_angle ).abs();
        angle_diff = std::max( angle_diff - AngleDeg::rad2deg( hide_radian ), 0.0 );

        if ( angle_diff < 14.0 )
        {
            count += 1;
        }
#ifdef DEBUG_PRINT_CROSS_CONE
        dlog.addText( Logger::TEAM,
                      "__ opp %d (%.1f %.1f) angle_diff=%.1f",
                      (*o)->unum(), opp_pos.x, opp_pos.y, angle_diff );
#endif
    }

    return count;
}

bool shoot_area_cf_move::throughPassShootHelper(rcsc::PlayerAgent *agent)
{


    const WorldModel &wm =agent->world();
    const int num =wm.self().unum();
    bool through_pass=false;
    Vector2D home_pos=Strategy::i().getPosition(wm.self().unum());
    Vector2D target_point=home_pos;
    const int mate_min=wm.interceptTable()->teammateReachCycle();
    const int opp_min =wm.interceptTable()->opponentReachCycle();
    const int self_min=wm.interceptTable()->selfReachCycle();
   



    bool   fast_dribble_team=mate_min<opp_min&&wm.ball().vel().x>0.99&&wm.ball().pos().x>wm.self().pos().x;



    if (!fast_dribble_team)  return false;



        double buff=wm.offsideLineX()-wm.self().pos().x;

        target_point.x=std::max(wm.offsideLineX()+buff,wm.ball().inertiaPoint(mate_min).x+buff);

        target_point.y=fabs(target_point.y-wm.self().pos().y)<6.0? wm.self().pos().y:target_point.y;

        double opp_dist_target=1000;
        const PlayerObject *opp_marker= wm.getOpponentNearestTo(target_point,5,&opp_dist_target);

        if (wm.self().pos().x>home_pos.x)
        {
            return false;
        }



           double dist_thr=wm.ball().distFromSelf()*0.1;
           dist_thr=dist_thr<0.5? 0.5:dist_thr;

           if (!Body_GoToPoint(target_point,dist_thr,ServerParam::i().maxDashPower()).execute(agent))
           {
               Body_TurnToBall().execute(agent);
           }

           if (wm.ball().distFromSelf()>18)
           {
               agent->setNeckAction(new Neck_TurnToBallOrScan());

           }

           else
           {
               agent->setNeckAction(new Neck_TurnToBall());
           }



    return true;




  }

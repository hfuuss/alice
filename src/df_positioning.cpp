
#include"df_positioning.h"
#include<rcsc/action/body_go_to_point.h>
#include<rcsc/player/player_agent.h>
#include<rcsc/action/neck_turn_to_ball.h>
#include<rcsc/player/world_model.h>
#include "strategy.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_turn_to_angle.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_player_or_scan.h>
#include <rcsc/action/neck_turn_to_point.h>
#include <rcsc/action/view_synch.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/world_model.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/server_param.h>
#include <rcsc/common/logger.h>
#include <rcsc/action/neck_turn_to_player_or_scan.h>
#include<rcsc/action/neck_turn_to_ball_and_player.h>

#include <rcsc/geom/vector_2d.h>
#include <rcsc/geom/matrix_2d.h>
#include  "neck_check_ball_owner.h"
#include <assert.h>
//#define DEBUG_MARK

using namespace  rcsc;

bool DefensiveAction::Mark(rcsc::PlayerAgent *agent)

{
#ifdef DEBUG_MARK
    assert(!M_defensive_opp);
#endif

    if (!M_defensive_opp)
    {
        return false;
    }


    const WorldModel & wm = agent->world();

    const int min_step = wm.interceptTable()->opponentReachCycle();

    const Vector2D my_pos = wm.self().inertiaPoint( min_step );




    AngleDeg target_angle = (M_aimPos-my_pos).th();



    double dist_thr = 0.3;



    if ( !Body_GoToPoint( M_aimPos, dist_thr, M_dashPower,
                         -1.0, // dash speed
                         1, true, 12
                         ).execute( agent ) )
    {



        Body_TurnToAngle( target_angle ).execute( agent );
    }

    int view_count=0;
    if (wm.self().pos().dist(M_aimPos)<1.0)  view_count=1;


    agent->setNeckAction(new Neck_TurnToBallAndPlayer(M_defensive_opp,view_count));

    return  true;

}


bool  DefensiveAction::Formation(PlayerAgent *agent)
{
    const WorldModel &wm =agent->world();

    double  dist_thr= fabs(wm.ball().pos().x-wm.self().pos().x) *0.1;
    dist_thr=dist_thr<0.5? 0.5: dist_thr;

    if (!Body_GoToPoint(M_aimPos,dist_thr,M_dashPower).execute(agent))
    {

        Body_TurnToBall().execute(agent);
    }




    agent->setNeckAction(new  Neck_CheckBallOwner);




    return  true;


}

int DefensiveTools::getOurPlayerHomeNearestBall(const WorldModel &wm)

{
    int min_dist_player_num=-1;
    double min_dist=1000;

    for (int i=1;i<12;i++){

        const Vector2D home=Strategy::i().getPosition(i);
        double  dist =wm.ball().pos().dist(home);
        if (dist<min_dist){

            min_dist=dist;
            min_dist_player_num=i;
        }
    }


   return min_dist_player_num;


}

int  DefensiveTools::getOurPlayerHomeNearestBall(PlayerAgent *agent)
{

    const WorldModel &wm = agent->world();
    int min_dist_player_num=-1;
    double min_dist=1000;

    for (int i=1;i<12;i++){

        const Vector2D home=Strategy::i().getPosition(i);
        double  dist =wm.ball().pos().dist(home);
        if (dist<min_dist){

            min_dist=dist;
            min_dist_player_num=i;
        }
    }


   return min_dist_player_num;


}

int  DefensiveTools::getOurPlayerHomeNearestOpp(const rcsc::WorldModel &wm,const  AbstractPlayerObject*opp)
{

    int min_dist_player_num=-1;
    double min_dist=1000;

   if (!opp){
       return -1;
     }

    for (int i=1;i<12;i++){

//        if (i>5) continue;

        const Vector2D home=Strategy::i().getPosition(i);
        double  dist =opp->pos().dist(home);
        if (dist<min_dist){

            min_dist=dist;
            min_dist_player_num=i;
        }
    }


   return min_dist_player_num;


}


bool  DefensiveTools::exist_blocker(const rcsc::WorldModel&wm)
{
    const PlayerObject *fastopp=wm.interceptTable()->fastestOpponent();
    if (!fastopp)  return false;
    double d=1000;
    const PlayerObject *blocker=wm.getTeammateNearestTo(fastopp,5,&d);
    if (d>3.0)  return false;
//    if (fastopp->pos().x+1.0<blocker->pos().x)  return false;

    return true;

}


struct DistCompare{

private:
    rcsc::Vector2D  m_pos;


public:


 DistCompare(rcsc::Vector2D pos):
     m_pos(pos)
   {}


 bool operator () (const AbstractPlayerObject *lhs, const AbstractPlayerObject *rhs)
 {

     return lhs->pos().dist(m_pos)<rhs->pos().dist(m_pos);

 }


};


int DefensiveTools::getSelfRankDistAim(const WorldModel &wm, const Vector2D pos)
{
    std::vector<const AbstractPlayerObject *> our_player;
    for (AbstractPlayerCont::const_iterator it =wm.ourPlayers().begin();
         it !=wm.ourPlayers().end(); it++)
    {

        if ( !(*it) ) continue;
        our_player.push_back((*it));

    }

    std::sort(our_player.begin(),our_player.end(),DistCompare(pos));

    int index=1;

    for ( std::vector<const AbstractPlayerObject *> ::const_iterator  it =our_player.begin();
          it !=our_player.end(); it++)
    {

        if ( (*it)->unum()==wm.self().unum()) return index;
        index++;
    }

    return -1;

}


const AbstractPlayerObject *  DefensiveTools::getRankMate(const WorldModel &wm, const Vector2D pos,const int rank)
{
    std::vector<const AbstractPlayerObject *> our_player;
    for (AbstractPlayerCont::const_iterator it =wm.ourPlayers().begin();
         it !=wm.ourPlayers().end(); it++)
    {

        if ( !(*it) ) continue;
        our_player.push_back((*it));

    }

    std::sort(our_player.begin(),our_player.end(),DistCompare(pos));

    int index=1;

    for ( std::vector<const AbstractPlayerObject *> ::const_iterator  it =our_player.begin();
          it !=our_player.end(); it++)
    {

        if (index==rank) return (*it);
        index++;
    }

    return NULL;

}



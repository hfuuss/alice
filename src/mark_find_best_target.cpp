/*
 * CopyRight(C)  YongDeng
 *
 * 2016-
 */

#include "mark_find_best_target.h"
#include  "strategy.h"
#include "df_positioning.h"
#include<rcsc/player/world_model.h>
#include<rcsc/geom/vector_2d.h>
#include<rcsc/common/server_param.h>
#include<rcsc/player/intercept_table.h>
#include <vector>
#include<stdio.h>


using namespace rcsc;





struct  normal_mark_eva_cmp
{
private:
    const  rcsc::Vector2D  m_self_pos;
    const  rcsc::Vector2D  m_home_pos;

public:

   normal_mark_eva_cmp( const rcsc::Vector2D self_pos, const rcsc::Vector2D home_pos):
        m_self_pos(self_pos), m_home_pos(home_pos)
    {}

    bool  operator() (const rcsc::AbstractPlayerObject *lhs, const rcsc::AbstractPlayerObject *rhs)
    {

        const  rcsc::Vector2D  our_goal=ServerParam::i().ourTeamGoalPos();

        double  lhs_eva= lhs->pos().dist(our_goal) * -0.4+ lhs->pos().dist(m_self_pos) *-0.6;
        double  rhs_eva= rhs->pos().dist(our_goal) * -0.4+ rhs->pos().dist(m_self_pos) *-0.6;


        return  lhs_eva>rhs_eva;

    }

};

struct  df_mark_eva_cmp
{
private:
    const  rcsc::Vector2D  m_self_pos;
    const  rcsc::Vector2D  m_home_pos;

public:

   df_mark_eva_cmp( const rcsc::Vector2D self_pos, const rcsc::Vector2D home_pos):
        m_self_pos(self_pos), m_home_pos(home_pos)
    {}

    bool  operator() (const rcsc::AbstractPlayerObject *lhs, const rcsc::AbstractPlayerObject *rhs)
    {

        const  rcsc::Vector2D  our_goal=ServerParam::i().ourTeamGoalPos();

        double  lhs_eva= lhs->pos().dist(m_home_pos) * -1;
        double  rhs_eva= rhs->pos().dist(m_home_pos) * -1;


        return  lhs_eva>rhs_eva;


    }

};

struct  set_play_mark_eva_cmp

{
private:
    const  rcsc::Vector2D  m_self_pos;
    const  rcsc::Vector2D  m_ball_pos;


public:

   set_play_mark_eva_cmp( const rcsc::Vector2D self_pos, const rcsc::Vector2D ball_pos):
        m_self_pos(self_pos), m_ball_pos(ball_pos)
    {}

    bool  operator() (const rcsc::AbstractPlayerObject *lhs, const rcsc::AbstractPlayerObject *rhs)
    {
        double  lhs_eva= lhs->pos().dist(m_ball_pos) *-1.0;
        double  rhs_eva= rhs->pos().dist(m_ball_pos) *-1.0;
        return  lhs_eva>rhs_eva;

    }

};





 const  rcsc::AbstractPlayerObject *   mark_find_best_target::get_ba_dg_best_mark_opp( )
{

     const WorldModel &wm = M_player_agent->world();
     const Vector2D home =Strategy::i().getPosition(wm.self().unum());
     std::vector<const AbstractPlayerObject *> opps_near_self;
     opps_near_self.reserve(4);
     int count=0;
     for ( PlayerPtrCont::const_iterator it =wm.opponentsFromSelf().begin();
           it != wm.opponentsFromSelf().end();  it++)
     {

         if (count>4)
         {
             break;
         }

         if ( !(*it))
         {
             continue;
         }
         opps_near_self.push_back( (*it));
         count++;

     }



         std::sort ( opps_near_self.begin(),opps_near_self.end(),  normal_mark_eva_cmp(wm.self().pos(),home ));



     const AbstractPlayerObject * fastest_opp = wm.interceptTable()->fastestOpponent();


     bool  find=false;


     for(std::vector<const AbstractPlayerObject*>::const_iterator it = opps_near_self.begin();
         it !=opps_near_self.end();  it++ )
     {

         if ( ! (*it))  continue;

         const Vector2D opp=(*it)->pos();
         double  self_dist_opp=wm.self().pos().dist(opp);
         double mate_dist_Myaim=1000;
         double opp_near_mate=1000;
         const PlayerObject * mate=wm.getTeammateNearestTo(opp,5,&mate_dist_Myaim);
         const PlayerObject * opp_to_mate=wm.getOpponentNearestTo(mate,5,&opp_near_mate);




         if (self_dist_opp>=mate_dist_Myaim)
         {


             const int SelfRank=DefensiveTools::getSelfRankDistAim(wm,opp);





             if ((opp_to_mate==(*it) || !opp_to_mate )&& mate!=wm.getOurGoalie() )

             {
                 continue;
             }

             else   if (SelfRank==2)
             {
                  return (*it);
//                 mark_target=(*it);
//                 find=true;
//                 break;
             }
             else  if (SelfRank==3)
             {
                 const AbstractPlayerObject * mate=DefensiveTools::getRankMate(wm,opp,2);
                 const PlayerObject * opp_to_mate=wm.getOpponentNearestTo(mate->pos(),5,&opp_near_mate);
                 if ((opp_to_mate==(*it) || !opp_to_mate )&& mate!=wm.getOurGoalie() )

                 {
                     continue;
                 }
                 else
                 {
                     return (*it);
                 }

             }
             else  if (SelfRank==4)
             {
                 const AbstractPlayerObject * mate_1=DefensiveTools::getRankMate(wm,opp,2);
                 const AbstractPlayerObject * mate_2=DefensiveTools::getRankMate(wm,opp,3);
                 const PlayerObject * opp_to_mate_1=wm.getOpponentNearestTo(mate_1->pos(),5,&opp_near_mate);
                 const PlayerObject * opp_to_mate_2=wm.getOpponentNearestTo(mate_2->pos(),5,&opp_near_mate);

                 if (((opp_to_mate_1==(*it) || !opp_to_mate_1)&& mate_1!=wm.getOurGoalie())

                      ||((opp_to_mate_2==(*it) || !opp_to_mate_2)&& mate_2!=wm.getOurGoalie()))

                 {
                     continue;
                 }
                 else
                 {
                     return (*it);
                 }


             }

             continue;

         }


         else
         {
               return (*it);
//             mark_target=(*it);
//             find=true;
//             break;

         }


     }


   // opps_near_self.clear();

    return NULL;



 }



const AbstractPlayerObject* mark_find_best_target::get_set_play_mark_opp()
{



    const WorldModel &wm = M_player_agent->world();
    Vector2D home =Strategy::i().getPosition(wm.self().unum());
    std::vector<const AbstractPlayerObject *> opps_near_self;
    opps_near_self.reserve(11);
    int count=0;
    for (   AbstractPlayerCont::const_iterator it =wm.theirPlayers().begin();
          it != wm.theirPlayers().end();  it++)
    {

        if (count>11)
        {
            break;
        }

        if ( !(*it))
        {
            continue;
        }
        opps_near_self.push_back( (*it));
        count++;

    }



  std::sort ( opps_near_self.begin(),opps_near_self.end(), set_play_mark_eva_cmp(wm.self().pos(),home ));



    const AbstractPlayerObject * fastest_opp = wm.interceptTable()->fastestOpponent();


    bool  find=false;


    for(std::vector<const AbstractPlayerObject*>::const_iterator it = opps_near_self.begin();
        it !=opps_near_self.end();  it++ )
    {

        if ( ! (*it))  continue;

//        const Vector2D opp=(*it)->pos();
//        double  self_dist_opp=wm.self().pos().dist(opp);
//        double mate_dist_Myaim=1000;
//        double opp_near_mate=1000;
//        const PlayerObject * mate=wm.getTeammateNearestTo(opp,5,&mate_dist_Myaim);
//        const PlayerObject * opp_to_mate=wm.getOpponentNearestTo(mate,5,&opp_near_mate);




        if (DefensiveTools::getOurPlayerHomeNearestOpp(wm,(*it))!=wm.self().unum())
        {

                continue;

        }


        else
        {
         return (*it);

        }


    }




   return NULL;


}


const rcsc::Vector2D  mark_find_best_target::getMarkTarget(PlayerAgent *agent, const AbstractPlayerObject *mark_target)
{

    const WorldModel & wm = agent->world();
    Vector2D mark_target_pos = mark_target->inertiaFinalPoint();
    Vector2D  home=Strategy::i().getPosition(wm.self().unum());
    const double dist_tance=wm.self().pos().dist(mark_target_pos);
    const double run_time=wm.self().playerType().cyclesToReachDistance(dist_tance);
    const int opp_min=wm.interceptTable()->opponentReachCycle();


//    if ( mark_target->velCount()<=1)
//    {
//        mark_target_pos+=mark_target->vel() *run_time;
//    }

    Vector2D move_point=mark_target_pos;
    if (wm.ball().posCount()<=1&&mark_target->posCount()<=1)
    {
        AngleDeg mark_angle=(wm.ball().inertiaPoint(1)-mark_target_pos).th();
        move_point= move_point + Vector2D::polar2vector(mark_target->playerTypePtr()->kickableArea(),mark_angle);
    }


    
    if (home.x<move_point.x)
    {
       move_point.x=(mark_target_pos.x+home.x)/2;
    }
    
    
    
    
    return move_point;
}


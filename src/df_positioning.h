
#ifndef DY_DF_POSITIONING_H
#define DY_DF_POSITIONING_H

#include<rcsc/geom/vector_2d.h>
#include<rcsc/player/player_agent.h>
#include <rcsc/player/abstract_player_object.h>

class  DefensiveAction
{



public:

 DefensiveAction(   rcsc::Vector2D aimPos,  double dashPower, const rcsc::AbstractPlayerObject *opp):
        M_aimPos(aimPos),M_dashPower(dashPower), M_defensive_opp(opp)
    {}


    bool  Mark(rcsc::PlayerAgent *agent);
    // bool  Block(rcsc::PlayerAgent *agent);
    bool  Formation(rcsc::PlayerAgent *agent);


private:



    const rcsc::AbstractPlayerObject *M_defensive_opp;
    rcsc::Vector2D M_aimPos;
    const double M_dashPower;




};


class  DefensiveTools
 {
    public:
    DefensiveTools()
    {}
    static int  getOurPlayerHomeNearestBall(rcsc::PlayerAgent *agent);
    static int  getOurPlayerHomeNearestBall(const rcsc::WorldModel &wm);
    static int  getOurPlayerHomeNearestOpp(const rcsc::WorldModel &wm, const rcsc::AbstractPlayerObject *opp);
    static int  getSelfRankDistAim(const rcsc::WorldModel&wm,const rcsc::Vector2D pos);
    static const rcsc::AbstractPlayerObject * getRankMate(const rcsc::WorldModel&wm,const rcsc::Vector2D pos,const int  rank);

    static  bool exist_blocker(const rcsc::WorldModel &wm);



};




#endif

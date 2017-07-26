

#ifndef DY_REC_PASS_SHOOT_MOVE_H
#define DY_REC_PASS_SHOOT_MOVE_H


#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>
class dy_rec_pass_shoot_move
        : public rcsc::SoccerBehavior
{

public:
    dy_rec_pass_shoot_move()
    {}
    bool execute(rcsc::PlayerAgent *agent);
private:

    bool search_route(rcsc::PlayerAgent *agent,rcsc::Vector2D &target);
    double evaluation_point(rcsc::PlayerAgent *agent, rcsc::Vector2D target);

    bool can_rec_ball(rcsc::PlayerAgent *agent, rcsc::Vector2D target);


};
#endif // MIRACLE_REC_PASS_SHOOT_MOVE_H

/*
 * CopyRight(C)  YongDeng
 *
 * 2016-
 */


#ifndef DY_MAKR_FIND_BEST_TARGET_H
#define DY_MAKR_FIND_BEST_TARGET_H

#include <rcsc/player/player_agent.h>
#include <rcsc/player/abstract_player_object.h>

class mark_find_best_target

{

private:
    bool  m_set_play_mark;

public:
    mark_find_best_target(rcsc::PlayerAgent *agent,bool  set_play_mark=false):
        M_player_agent(agent), m_set_play_mark(set_play_mark)
    {}
    

    const  rcsc::AbstractPlayerObject *  get_ba_dg_best_mark_opp();
   // const  rcsc::AbstractPlayerObject *  get_ba_df_best_mark_opp();
    const  rcsc::AbstractPlayerObject *   get_set_play_mark_opp();
    static  const rcsc::Vector2D  getMarkTarget(rcsc::PlayerAgent *agent,
                                                const  rcsc::AbstractPlayerObject *mark_target);







private:
    rcsc::PlayerAgent *M_player_agent;

};

#endif // MIRACLE_MAKR_FIND_BEST_TARGET_H

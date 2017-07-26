// -*-c++-*-

/*!
  \file generator_tackle.cpp
  \brief tackle/foul generator class Source File
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tackle_generator.h"

#include "field_analyzer.h"

#include <rcsc/player/player_agent.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/ray_2d.h>
#include <rcsc/geom/rect_2d.h>
#include <rcsc/math_util.h>
#include <rcsc/timer.h>

#include <limits>

#define ASSUME_OPPONENT_KICK

// #define DEBUG_PROFILE
// #define DEBUG_PRINT

// #define DEBUG_PREDICT_OPPONENT_REACH_STEP
// #define DEBUG_PREDICT_OPPONENT_REACH_STEP_LEVEL2


using namespace rcsc;

namespace {
const int ANGLE_DIVS = 40;
}

/*-------------------------------------------------------------------*/
/*!

 */
TackleGenerator::Result::Result()
    : index_( -1 )
{
    clear();
}

/*-------------------------------------------------------------------*/
/*!

 */
TackleGenerator::Result::Result( const int index,
                                 const rcsc::AngleDeg & angle,
                                 const rcsc::Vector2D & vel )
    : index_( index ),
      tackle_angle_( angle ),
      ball_vel_( vel ),
      ball_speed_( vel.r() ),
      ball_move_angle_( vel.th() ),
      score_( 0.0 )
{

}

/*-------------------------------------------------------------------*/
/*!

 */
void
TackleGenerator::Result::clear()
{
    tackle_angle_ = 0.0;
    ball_vel_.assign( 0.0, 0.0 );
    ball_speed_ = 0.0;
    ball_move_angle_ = 0.0;
    score_ = -std::numeric_limits< double >::max();
}

/*-------------------------------------------------------------------*/
/*!

 */
TackleGenerator::TackleGenerator()
{
    M_candidates.reserve( ANGLE_DIVS );
    clear();
}

/*-------------------------------------------------------------------*/
/*!

 */
TackleGenerator &
TackleGenerator::instance()
{
    static TackleGenerator s_instance;
    return s_instance;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
TackleGenerator::clear()
{
    M_best_result = Result();
    M_candidates.clear();
}

/*-------------------------------------------------------------------*/
/*!

 */
void
TackleGenerator::generate( const WorldModel & wm )
{
    static GameTime s_update_time( 0, 0 );

    if ( s_update_time == wm.time() )
    {
        // dlog.addText( Logger::CLEAR,
        //               __FILE__": already updated" );
        return;
    }
    s_update_time = wm.time();

    clear();

    if ( wm.self().isKickable() )
    {
        // dlog.addText( Logger::CLEAR,
        //               __FILE__": kickable" );
        return;
    }

    if ( wm.self().tackleProbability() < 0.001
         && wm.self().foulProbability() < 0.001 )
    {
        // dlog.addText( Logger::CLEAR,
        //               __FILE__": never tacklable" );
        return;
    }

    if ( wm.time().stopped() > 0 )
    {
        // dlog.addText( Logger::CLEAR,
        //               __FILE__": time stopped" );
        return;
    }

    if ( wm.gameMode().type() != GameMode::PlayOn
         && ! wm.gameMode().isPenaltyKickMode() )
    {
        // dlog.addText( Logger::CLEAR,
        //               __FILE__": illegal playmode " );
        return;
    }


#ifdef DEBUG_PROFILE
    MSecTimer timer;
#endif

    calculate( wm );

#ifdef DEBUG_PROFILE
    dlog.addText( Logger::CLEAR,
                  __FILE__": PROFILE. elapsed=%.3f [ms]",
                  timer.elapsedReal() );
#endif

}

/*-------------------------------------------------------------------*/
/*!

 */
void
TackleGenerator::calculate( const WorldModel & wm )
{
    const ServerParam & SP = ServerParam::i();

    const double min_angle = SP.minMoment();
    const double max_angle = SP.maxMoment();
    const double angle_step = std::fabs( max_angle - min_angle ) / ANGLE_DIVS;

    const bool our_shootable = ( wm.ball().pos().dist2( SP.theirTeamGoalPos() ) < std::pow( 18.0, 2 ) );
#ifdef ASSUME_OPPONENT_KICK
    const bool their_shootable = ( wm.ball().pos().dist2( SP.ourTeamGoalPos() ) < std::pow( 18.0, 2 ) );
    const Vector2D shoot_accel = ( SP.ourTeamGoalPos() - wm.ball().pos() ).setLengthVector( 2.0 );
    const bool their_cross = ( wm.ball().pos().absY() > SP.goalHalfWidth()
                               && wm.ball().pos().x < -SP.pitchHalfLength() + 10.0 );
    const Vector2D cross_accel = ( Vector2D( -45.0, 0.0 ) - wm.ball().pos() ).setLengthVector( 2.0 );
#endif

    const AngleDeg ball_rel_angle = wm.ball().angleFromSelf() - wm.self().body();
    const double tackle_rate
        = SP.tacklePowerRate()
        * ( 1.0 - 0.5 * ball_rel_angle.abs() / 180.0 );

#ifdef DEBUG_PRINT
    dlog.addText( Logger::CLEAR,
                  __FILE__": min_angle=%.1f max_angle=%.1f angle_step=%.1f",
                  min_angle, max_angle, angle_step );
    dlog.addText( Logger::CLEAR,
                  __FILE__": ball_rel_angle=%.1f tackle_rate=%.1f",
                  ball_rel_angle.degree(), tackle_rate );
#endif

    std::vector< double > angles;
    angles.reserve( ANGLE_DIVS + ( our_shootable ? 8 : 0 ) );
    for ( int a = 0; a < ANGLE_DIVS; ++a )
    {
        angles.push_back( min_angle + angle_step * a );
    }

    if ( our_shootable )
    {
        const double dist_step = SP.goalWidth() - 2.0 / 7.0;
        for ( int i = 0; i < 8; ++i )
        {
            Vector2D target( SP.pitchHalfLength(),
                             -SP.goalHalfWidth() + 1.0 + dist_step*i );
            angles.push_back( ( target - wm.ball().pos() ).th().degree() );
        }
    }

    int count = 0;
    for ( std::vector< double >::const_iterator a = angles.begin(), a_end = angles.end();
          a != a_end;
          ++a, ++count )
    {
        const AngleDeg dir = *a;

        double eff_power= ( SP.maxBackTacklePower()
                            + ( SP.maxTacklePower() - SP.maxBackTacklePower() )
                            * ( 1.0 - ( dir.abs() / 180.0 ) ) );
        eff_power *= tackle_rate;

        AngleDeg angle = wm.self().body() + dir;
        Vector2D accel = Vector2D::from_polar( eff_power, angle );

#ifdef ASSUME_OPPONENT_KICK
        if ( wm.existKickableOpponent() )
        {
            if ( their_cross )
            {
                accel += cross_accel;
                double d = accel.r();
                if ( d > SP.ballAccelMax() )
                {
                    accel *= ( SP.ballAccelMax() / d );
                }
            }
            else if ( their_shootable )
            {
                accel += shoot_accel;
                double d = accel.r();
                if ( d > SP.ballAccelMax() )
                {
                    accel *= ( SP.ballAccelMax() / d );
                }
            }
        }
#endif

        Vector2D vel = wm.ball().vel() + accel;
        if ( wm.existKickableOpponent() )
        {
            if ( wm.ball().seenVelCount() == 0
                 || ( wm.ball().seenVelCount() == 1
                      && wm.ball().seenPosCount() == 0 ) )
            {
                vel = wm.ball().seenVel() + accel;
            }
        }

        double speed = vel.r();
        if ( speed > SP.ballSpeedMax() )
        {
            vel *= ( SP.ballSpeedMax() / speed );
        }

        M_candidates.push_back( Result( count + 1, angle, vel ) );
#ifdef DEBUG_PRINT
        const Result & result = M_candidates.back();
        dlog.addText( Logger::CLEAR,
                      "%d: angle=%.1f(dir=%.1f), result: vel(%.2f %.2f ) speed=%.2f move_angle=%.1f",
                      result.index_,
                      result.tackle_angle_.degree(), dir.degree(),
                      result.ball_vel_.x, result.ball_vel_.y,
                      result.ball_speed_, result.ball_move_angle_.degree() );
#endif
    }


    M_best_result.clear();

    for ( Container::iterator it = M_candidates.begin(), end = M_candidates.end();
          it != end;
          ++it )
    {
        it->score_ = evaluate( wm, *it );

#ifdef DEBUG_PRINT
        Vector2D ball_end_point = inertia_final_point( wm.ball().pos(),
                                                       it->ball_vel_,
                                                       SP.ballDecay() );
        dlog.addLine( Logger::CLEAR,
                      wm.ball().pos(),
                      ball_end_point,
                      "#0000ff" );
        char buf[16];
        snprintf( buf, 16, "%d:%.3f", it->index_, it->score_ );
        dlog.addMessage( Logger::CLEAR,
                         ball_end_point, buf, "#ffffff" );
#endif

        if ( it->score_ > M_best_result.score_ )
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::CLEAR,
                          ">>>> updated" );
#endif
            M_best_result = *it;
        }
    }


#ifdef DEBUG_PRINT
    dlog.addLine( Logger::CLEAR,
                  wm.ball().pos(),
                  inertia_final_point( wm.ball().pos(),
                                       M_best_result.ball_vel_,
                                       SP.ballDecay() ),
                  "#ff0000" );
    dlog.addText( Logger::CLEAR,
                  "==== best_angle=%.1f score=%f speed=%.3f move_angle=%.1f",
                  M_best_result.tackle_angle_.degree(),
                  M_best_result.score_,
                  M_best_result.ball_speed_,
                  M_best_result.ball_move_angle_.degree() );
#endif
}

/*-------------------------------------------------------------------*/
/*!

 */
double
TackleGenerator::evaluate( const WorldModel & wm,
                           const Result & result )
{
    const ServerParam & SP = ServerParam::i();

    const Vector2D ball_end_point = inertia_final_point( wm.ball().pos(),
                                                         result.ball_vel_,
                                                         SP.ballDecay() );
    const Segment2D ball_line( wm.ball().pos(), ball_end_point );
    const double ball_speed = result.ball_speed_;
    const AngleDeg ball_move_angle = result.ball_move_angle_;

#ifdef DEBUG_PRINT
    dlog.addText( Logger::CLEAR,
                  "%d: (evaluate) angle=%.1f speed=%.2f move_angle=%.1f end_point=(%.2f %.2f)",
                  result.index_,
                  result.tackle_angle_.degree(),
                  ball_speed,
                  ball_move_angle.degree(),
                  ball_end_point.x, ball_end_point.y );
#endif

    //
    // moving to their goal
    //
    if ( ball_end_point.x > SP.pitchHalfLength()
         && wm.ball().pos().dist2( SP.theirTeamGoalPos() ) < std::pow( 20.0, 2 ) )
    {
        const Line2D goal_line( Vector2D( SP.pitchHalfLength(), 10.0 ),
                                Vector2D( SP.pitchHalfLength(), -10.0 ) );
        const Vector2D intersect = ball_line.intersection( goal_line );
        if ( intersect.isValid()
             && intersect.absY() < SP.goalHalfWidth() )
        {
            double shoot_score = 1000000.0;
            double speed_rate = 1.0 - std::exp( - std::pow( ball_speed, 2 )
                                                / ( 2.0 * std::pow( SP.ballSpeedMax()*0.5, 2 ) ) );
            double y_rate = std::exp( - std::pow( intersect.absY(), 2 )
                                      / ( 2.0 * std::pow( SP.goalWidth(), 2 ) ) );
            double goalie_rate = 1.0;
            const AbstractPlayerObject * goalie = wm.getOpponentGoalie();
            if ( goalie )
            {
                double angle_diff = ( goalie->angleFromBall() - result.ball_move_angle_ ).abs();
                goalie_rate = 1.0 - std::exp( - std::pow( angle_diff, 2 ) / ( 2.0 * std::pow( 10.0, 2 ) ) );
            }

            shoot_score *= speed_rate;
            shoot_score *= y_rate;
            shoot_score *= goalie_rate;
#ifdef DEBUG_PRINT
            dlog.addText( Logger::CLEAR,
                          "%d: __ shoot %f (speed_rate=%f y_rate=%f goalie_rate=%f)",
                          result.index_,
                          shoot_score, speed_rate, y_rate, goalie_rate );
#endif
            return shoot_score;
        }
    }

    //
    // moving to our goal
    //
    if ( ball_end_point.x < -SP.pitchHalfLength() )
    {
        const Line2D goal_line( Vector2D( -SP.pitchHalfLength(), 10.0 ),
                                Vector2D( -SP.pitchHalfLength(), -10.0 ) );
        const Vector2D intersect = ball_line.intersection( goal_line );
        if ( intersect.isValid()
             && intersect.absY() < SP.goalHalfWidth() + 3.0 )
        {
            double shoot_score = 0.0;
            double y_penalty = ( -10000.0
                                 * std::exp( - std::pow( intersect.absY() - SP.goalHalfWidth(), 2 )
                                             / ( 2.0 * std::pow( SP.goalHalfWidth(), 2 ) ) ) );
            double speed_bonus = ( +1000.0
                                   * std::exp( - std::pow( ball_speed, 2 )
                                               / ( 2.0 * std::pow( SP.ballSpeedMax()*0.5, 2 ) ) ) );
            shoot_score = y_penalty + speed_bonus;
#ifdef DEBUG_PRINT
            dlog.addText( Logger::CLEAR,
                          "%d: __ in our goal %f (y_pealty=%f speed_bonus=%f)",
                          result.index_,
                          shoot_score, y_penalty, speed_bonus );
#endif
            return shoot_score;
        }
    }

    //
    // normal evaluation
    //

    int opponent_reach_step = predictOpponentsReachStep( wm,
                                                         wm.ball().pos(),
                                                         result.ball_vel_,
                                                         ball_move_angle );
    Vector2D final_point = inertia_n_step_point( wm.ball().pos(),
                                                 result.ball_vel_,
                                                 opponent_reach_step,
                                                 SP.ballDecay() );
    {
        Segment2D final_segment( wm.ball().pos(), final_point );
        Rect2D pitch = Rect2D::from_center( 0.0, 0.0, SP.pitchLength(), SP.pitchWidth() );
        Vector2D intersection;
        int n = pitch.intersection( final_segment, &intersection, NULL );
        if ( n > 0 )
        {
            final_point = intersection;
        }
    }

    double one_step_opponent_dist2 = getOneStepMinimumOpponentDistance2( wm,
                                                                         wm.ball().pos(),
                                                                         result.ball_vel_ );


    AngleDeg our_goal_angle = ( SP.ourTeamGoalPos() - wm.ball().pos() ).th();
    double our_goal_angle_diff = ( our_goal_angle - ball_move_angle ).abs();
    double our_goal_angle_rate = 1.0 - std::exp( - std::pow( our_goal_angle_diff, 2 )
                                                 / ( 2.0 * std::pow( 40.0, 2 ) ) );

    double y_rate = 1.0;
    if ( wm.ball().pos().x > -SP.pitchHalfLength() + 3.0 )
    {
        y_rate = ( final_point.absY() > SP.pitchHalfWidth() - 0.1
                   ? 1.0
                   : std::exp( - std::pow( final_point.absY() - SP.pitchHalfWidth(), 2 )
                               / ( 2.0 * std::pow( SP.pitchHalfWidth() * 0.7, 2 ) ) ) );
    }

    double opp_rate = 1.0 - std::exp( - std::pow( (double)opponent_reach_step, 2 )
                                      / ( 2.0 * std::pow( 30.0, 2 ) ) );
    double dist_rate = 1.0 - std::exp( - one_step_opponent_dist2 / ( 2.0 * std::pow( 1.0, 2 ) ) );

    double score = 10000.0 * our_goal_angle_rate * y_rate * opp_rate * dist_rate;

#ifdef DEBUG_PRINT
    dlog.addText( Logger::CLEAR,
                  "%d: __ goal_angle_rate=%f",
                  result.index_, our_goal_angle_rate, y_rate, score );
    dlog.addText( Logger::CLEAR,
                  "%d: __ y_rate=%f",
                  result.index_, our_goal_angle_rate, y_rate, score );
    dlog.addText( Logger::CLEAR,
                  "%d: __ opp_rate=%f opponent_step=%d",
                  result.index_, opp_rate, opponent_reach_step );
    dlog.addText( Logger::CLEAR,
                  "%d: __ dist_rate=%f dist=%.3f",
                  result.index_, dist_rate, std::sqrt( one_step_opponent_dist2 ) );

    dlog.addText( Logger::CLEAR,
                  "%d: >>> score=%f",
                  result.index_, score );
#endif

    return score;
}

/*-------------------------------------------------------------------*/
/*!

 */
double
TackleGenerator::getOneStepMinimumOpponentDistance2( const WorldModel & wm,
                                                     const Vector2D & first_ball_pos,
                                                     const Vector2D & first_ball_vel )
{
    const Vector2D ball_next = first_ball_pos + first_ball_vel;
    const double move_dist2 = first_ball_pos.dist2( ball_next );
    const double move_dist = std::sqrt( move_dist2 );

    double min_dist2 = 1000000.0;
    for ( AbstractPlayerCont::const_iterator o = wm.theirPlayers().begin(),
              end = wm.theirPlayers().end();
          o != end;
          ++o )
    {
        Vector2D pos = (*o)->pos() + (*o)->vel();
        double d2 = ball_next.dist2( pos );

        const double inner_prod = first_ball_vel.innerProduct( pos - first_ball_pos );
        if ( move_dist > 1.0e-3
             && 0.0 <= inner_prod && inner_prod <= move_dist2 )
        {
            double project_point_dist
                = std::fabs( Triangle2D::double_signed_area( first_ball_pos, ball_next, pos )
                             / move_dist );
            d2 = std::min( d2, std::pow( project_point_dist, 2 ) );
        }

        if ( d2 < min_dist2 )
        {
            min_dist2 = d2;
        }
    }

    return min_dist2;
}

/*-------------------------------------------------------------------*/
/*!

 */
int
TackleGenerator::predictOpponentsReachStep( const WorldModel & wm,
                                            const Vector2D & first_ball_pos,
                                            const Vector2D & first_ball_vel,
                                            const AngleDeg & ball_move_angle )
{
    int first_min_step = 50;

#if 1
    const ServerParam & SP = ServerParam::i();
    const Vector2D ball_end_point = inertia_final_point( first_ball_pos,
                                                         first_ball_vel,
                                                         SP.ballDecay() );
    if ( ball_end_point.absX() > SP.pitchHalfLength() + 1.0
         || ball_end_point.absY() > SP.pitchHalfWidth() + 1.0 )
    {
        Rect2D pitch = Rect2D::from_center( 0.0, 0.0, SP.pitchLength(), SP.pitchWidth() );
        Ray2D ball_ray( first_ball_pos, ball_move_angle );
        Vector2D sol1, sol2;
        int n_sol = pitch.intersection( ball_ray, &sol1, &sol2 );
        if ( n_sol == 1 )
        {
            first_min_step = SP.ballMoveStep( first_ball_vel.r(), first_ball_pos.dist( sol1 ) );
#ifdef DEBUG_PRINT
            dlog.addText( Logger::CLEAR,
                          "(predictOpponent) ball will be out. step=%d reach_point=(%.2f %.2f)",
                          first_min_step,
                          sol1.x, sol1.y );
#endif
        }
    }
#endif

    int min_step = first_min_step;
    for ( AbstractPlayerCont::const_iterator o = wm.theirPlayers().begin(),
              end = wm.theirPlayers().end();
          o != end;
          ++o )
    {
        int step = predictOpponentReachStep( *o,
                                             first_ball_pos,
                                             first_ball_vel,
                                             ball_move_angle,
                                             min_step );
        if ( step < min_step )
        {
            min_step = step;
        }
    }

    return ( min_step == first_min_step
             ? 1000
             : min_step );
}


/*-------------------------------------------------------------------*/
/*!

 */
int
TackleGenerator::predictOpponentReachStep( const AbstractPlayerObject * opponent,
                                           const Vector2D & first_ball_pos,
                                           const Vector2D & first_ball_vel,
                                           const AngleDeg & ball_move_angle,
                                           const int max_cycle )
{
    const ServerParam & SP = ServerParam::i();

    const PlayerType * ptype = opponent->playerTypePtr();
    const double opponent_speed = opponent->vel().r();

    int min_cycle = FieldAnalyzer::estimate_min_reach_cycle( opponent->pos(),
                                                             ptype->kickableArea() + 0.2,
                                                             ptype->realSpeedMax(),
                                                             first_ball_pos,
                                                             ball_move_angle );
    if ( min_cycle < 0 )
    {
        min_cycle = 10; // set penalty step
    }

    min_cycle -= opponent->posCount();
    if ( min_cycle < 1 )
    {
        min_cycle = 1;
    }

#ifdef DEBUG_PREDICT_OPPONENT_REACH_STEP
    dlog.addText( Logger::CLEAR,
                  "opponent[%d] min_step=%d max_step=%d",
                  opponent->unum(),
                  min_cycle, max_cycle );
#endif

    for ( int cycle = min_cycle; cycle < max_cycle; ++cycle )
    {
        Vector2D ball_pos = inertia_n_step_point( first_ball_pos,
                                                  first_ball_vel,
                                                  cycle,
                                                  SP.ballDecay() );

        if ( ball_pos.absX() > SP.pitchHalfLength()
             || ball_pos.absY() > SP.pitchHalfWidth() )
        {
#ifdef DEBUG_PREDICT_OPPONENT_REACH_STEP
            dlog.addText( Logger::CLEAR,
                          "__ opponent=%d(%.1f %.1f) step=%d ball is out of pitch. ",
                          opponent->unum(),
                          opponent->pos().x, opponent->pos().y,
                          cycle  );
#endif
            return 1000;
        }

        Vector2D inertia_pos = opponent->inertiaPoint( cycle );
        double target_dist = inertia_pos.dist( ball_pos );

        if ( target_dist - ptype->kickableArea() < 0.001 )
        {
#ifdef DEBUG_PREDICT_OPPONENT_REACH_STEP
            dlog.addText( Logger::CLEAR,
                          "__ opponent=%d(%.1f %.1f) step=%d already there. dist=%.1f",
                          opponent->unum(),
                          opponent->pos().x, opponent->pos().y,
                          cycle,
                          target_dist );
#endif
            return cycle;
        }

        double dash_dist = target_dist;
        dash_dist -= ptype->kickableArea() + 0.2;
        if ( cycle > 1 )
        {
            dash_dist -= 0.4; // special bonus
        }

        if ( dash_dist > ptype->realSpeedMax() * cycle )
        {
#ifdef DEBUG_PREDICT_OPPONENT_REACH_STEP_LEVEL2
            dlog.addText( Logger::CLEAR,
                          "__ opponent=%d(%.1f %.1f) step=%d dash_dist=%.1f reachable=%.1f",
                          opponent->unum(),
                          opponent->pos().x, opponent->pos().y,
                          cycle, dash_dist, ptype->realSpeedMax()*cycle );
#endif
            continue;
        }

        //
        // dash
        //

        int n_dash = ptype->cyclesToReachDistance( dash_dist );

        if ( n_dash > cycle )
        {
#ifdef DEBUG_PREDICT_OPPONENT_REACH_STEP_LEVEL2
            dlog.addText( Logger::CLEAR,
                          "__ opponent=%d(%.1f %.1f) cycle=%d dash_dist=%.1f n_dash=%d",
                          opponent->unum(),
                          opponent->pos().x, opponent->pos().y,
                          cycle, dash_dist, n_dash );
#endif
            continue;
        }

        //
        // turn
        //
        int n_turn = ( opponent->bodyCount() > 1
                       ? 0
                       : FieldAnalyzer::predict_player_turn_cycle( ptype,
                                                                   opponent->body(),
                                                                   opponent_speed,
                                                                   target_dist,
                                                                   ( ball_pos - inertia_pos ).th(),
                                                                   ptype->kickableArea(),
                                                                   true ) );

        int n_step = ( n_turn == 0
                       ? n_turn + n_dash
                       : n_turn + n_dash + 1 ); // 1 step penalty for observation delay
        if ( opponent->isTackling() )
        {
            n_step += 5; // Magic Number
        }

        n_step -= std::min( 3, opponent->posCount() );

#ifdef DEBUG_PREDICT_OPPONENT_REACH_STEP
        dlog.addText( Logger::CLEAR,
                      "__ opponent=%d(%.1f %.1f) step=%d(t%d,d%d) bpos=(%.2f %.2f) dist=%.2f dash_dist=%.2f",
                      opponent->unum(),
                      opponent->pos().x, opponent->pos().y,
                      cycle, n_turn, n_dash,
                      ball_pos.x, ball_pos.y,
                      target_dist,
                      dash_dist );
#endif
        if ( n_step <= cycle )
        {
            return cycle;
        }

    }

    return 1000;
}

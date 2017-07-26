#ifndef DY_BLOCK_MODEL_H
#define DY_BLOCK_MODEL_H

#include <rcsc/player/player_agent.h>

class  BP_Block_Model 
{
	public:
	BP_Block_Model() 
	{}


	static  BP_Block_Model & instance() 
	{
		static  BP_Block_Model  s_instance;
		return s_instance;
	}

   bool  input( const double x, const double y)
   {
	   ball_pos_x=x;
	   ball_pos_y=y;
   }
  

   private:

   const double  ball_pos_x;
   const double  ball_pos_y;

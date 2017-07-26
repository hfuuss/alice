

Player_Obj	+=	\
                $(Dst_SrcObj)/bhv_savior.o\
                $(Dst_SrcObj)/voron_point.o\
                $(Dst_SrcObj)/bhv_deflecting_tackle.o\
                $(Dst_SrcObj)/BinaryToFile.o\
                $(Dst_SrcObj)/mark_find_best_target.o\
                $(Dst_SrcObj)/df_positioning.o\
                $(Dst_SrcObj)/rec_pass_shoot_move.o\
                $(Dst_SrcObj)/shoot_area_of_move.o\
                $(Dst_SrcObj)/shoot_area_sf_move.o\
                $(Dst_SrcObj)/shoot_area_cf_move.o\
                $(Dst_SrcObj)/Brain.o \
                $(Dst_SrcObj)/bhv_centerback_move.o\
                $(Dst_SrcObj)/bhv_sideback_move.o\
                $(Dst_SrcObj)/bhv_get_ball.o\
                $(Dst_SrcObj)/bhv_defensivehalf_move.o\
                $(Dst_SrcObj)/bhv_offensivehalf_defensive_move.o\
		$(Dst_SrcObj)/Evaluation.o\
                $(Dst_SrcObj)/neck_check_ball_owner.o\
		$(Dst_SrcObj)/Net.o\
		$(Dst_SrcObj)/bhv_basic_move.o \
		$(Dst_SrcObj)/bhv_attacker_offensive_move.o\
		$(Dst_SrcObj)/body_forestall_block.o\
		$(Dst_SrcObj)/bhv_offensive_half_offensive_move.o\
		$(Dst_SrcObj)/bhv_basic_offensive_kick.o \
		$(Dst_SrcObj)/bhv_basic_tackle.o \
		$(Dst_SrcObj)/bhv_custom_before_kick_off.o \
		$(Dst_SrcObj)/bhv_go_to_static_ball.o \
		$(Dst_SrcObj)/bhv_goalie_basic_move.o \
		$(Dst_SrcObj)/bhv_goalie_chase_ball.o \
		$(Dst_SrcObj)/bhv_goalie_free_kick.o \
		$(Dst_SrcObj)/bhv_prepare_set_play_kick.o \
		$(Dst_SrcObj)/bhv_set_play.o \
		$(Dst_SrcObj)/bhv_set_play_free_kick.o \
		$(Dst_SrcObj)/bhv_set_play_goal_kick.o \
		$(Dst_SrcObj)/bhv_set_play_indirect_free_kick.o \
		$(Dst_SrcObj)/bhv_set_play_kick_in.o \
		$(Dst_SrcObj)/bhv_set_play_kick_off.o \
		$(Dst_SrcObj)/bhv_their_goal_kick_move.o \
		$(Dst_SrcObj)/bhv_penalty_kick.o \
		$(Dst_SrcObj)/neck_default_intercept_neck.o \
		$(Dst_SrcObj)/neck_goalie_turn_neck.o \
		$(Dst_SrcObj)/neck_offensive_intercept_neck.o \
		$(Dst_SrcObj)/view_tactical.o \
		$(Dst_SrcObj)/intention_receive.o \
		$(Dst_SrcObj)/intention_wait_after_set_play_kick.o \
		$(Dst_SrcObj)/soccer_role.o \
		$(Dst_SrcObj)/role_center_back.o \
		$(Dst_SrcObj)/role_center_forward.o \
		$(Dst_SrcObj)/role_defensive_half.o \
		$(Dst_SrcObj)/role_goalie.o \
		$(Dst_SrcObj)/role_offensive_half.o \
		$(Dst_SrcObj)/role_sample.o \
		$(Dst_SrcObj)/role_side_back.o \
		$(Dst_SrcObj)/role_side_forward.o \
		$(Dst_SrcObj)/role_side_half.o \
		$(Dst_SrcObj)/role_keepaway_keeper.o \
		$(Dst_SrcObj)/role_keepaway_taker.o \
		$(Dst_SrcObj)/sample_communication.o \
		$(Dst_SrcObj)/keepaway_communication.o \
		$(Dst_SrcObj)/sample_player.o \
		$(Dst_SrcObj)/strategy.o \
		$(Dst_SrcObj)/main_player.o

Coach_Obj	+=	\
		$(Dst_SrcObj)/sample_coach.o \
		$(Dst_SrcObj)/main_coach.o

Trainer_Obj	+=	\
		$(Dst_SrcObj)/sample_trainer.o \
		$(Dst_SrcObj)/main_trainer.o


$(Dst_SrcObj)/%.o: $(SrcPath)/%.cpp config.mk
	@test -d $(Dst_SrcObj) || mkdir -p $(Dst_SrcObj)
	@echo ""
	$(GXX) $(INCLUDE) $(DEFS) $(CPPFLAGS) $(EXTR_SRC_CPPFLAGS) -c "$<" -o "$@"
	@echo ""


#===============================================================
# Agent2D-3.1.0 & librcsc-4.1.0 Makefile
# RoboCup2D @ Anhui Unversity of Technology
# Bugs or Suggestions please mail to guofeng1208@163.com
#===============================================================

Rcsc_Obj	+=	\
	$(Dst_RcscObj)/action/basic_actions.o \
	$(Dst_RcscObj)/action/bhv_before_kick_off.o \
	$(Dst_RcscObj)/action/bhv_emergency.o \
	$(Dst_RcscObj)/action/bhv_go_to_point_look_ball.o \
	$(Dst_RcscObj)/action/bhv_scan_field.o \
	$(Dst_RcscObj)/action/bhv_shoot2008.o \
	$(Dst_RcscObj)/action/body_advance_ball2009.o \
	$(Dst_RcscObj)/action/body_clear_ball2009.o \
	$(Dst_RcscObj)/action/body_dribble2008.o \
	$(Dst_RcscObj)/action/body_go_to_point2009.o \
	$(Dst_RcscObj)/action/body_go_to_point2010.o \
	$(Dst_RcscObj)/action/body_go_to_point_dodge.o \
	$(Dst_RcscObj)/action/body_hold_ball2008.o \
	$(Dst_RcscObj)/action/body_intercept2009.o \
	$(Dst_RcscObj)/action/body_kick_collide_with_ball.o \
	$(Dst_RcscObj)/action/body_kick_one_step.o \
	$(Dst_RcscObj)/action/body_kick_to_relative.o \
	$(Dst_RcscObj)/action/body_pass.o \
	$(Dst_RcscObj)/action/body_smart_kick.o \
	$(Dst_RcscObj)/action/body_stop_ball.o \
	$(Dst_RcscObj)/action/body_stop_dash.o \
	$(Dst_RcscObj)/action/intention_dribble2008.o \
	$(Dst_RcscObj)/action/intention_time_limit_action.o \
	$(Dst_RcscObj)/action/neck_scan_field.o \
	$(Dst_RcscObj)/action/neck_scan_players.o \
	$(Dst_RcscObj)/action/neck_turn_to_ball_and_player.o \
	$(Dst_RcscObj)/action/neck_turn_to_ball_or_scan.o \
	$(Dst_RcscObj)/action/neck_turn_to_goalie_or_scan.o \
	$(Dst_RcscObj)/action/neck_turn_to_player_or_scan.o \
	$(Dst_RcscObj)/action/neck_turn_to_low_conf_teammate.o \
	$(Dst_RcscObj)/action/view_synch.o \
	$(Dst_RcscObj)/action/kick_table.o \
	$(Dst_RcscObj)/action/shoot_table2008.o


$(Dst_RcscObj)/action/%.o: $(RcscPath)/action/%.cpp config.mk
	@test -d $(Dst_RcscObj)/action || mkdir -p $(Dst_RcscObj)/action
	@echo ""
	$(GXX) $(INCLUDE) $(DEFS) $(CPPFLAGS) $(EXTR_RCSC_CPPFLAGS) -c "$<" -o "$@"
	@echo ""


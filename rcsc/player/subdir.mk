#===============================================================
# Agent2D-3.1.0 & librcsc-4.1.0 Makefile
# RoboCup2D @ Anhui Unversity of Technology
# Bugs or Suggestions please mail to guofeng1208@163.com
#===============================================================

Rcsc_Obj	+=	\
	$(Dst_RcscObj)/player/abstract_player_object.o \
	$(Dst_RcscObj)/player/action_effector.o \
	$(Dst_RcscObj)/player/audio_sensor.o \
	$(Dst_RcscObj)/player/ball_object.o \
	$(Dst_RcscObj)/player/body_sensor.o \
	$(Dst_RcscObj)/player/debug_client.o \
	$(Dst_RcscObj)/player/freeform_parser.o \
	$(Dst_RcscObj)/player/fullstate_sensor.o \
	$(Dst_RcscObj)/player/intercept_table.o \
	$(Dst_RcscObj)/player/localization_default.o \
	$(Dst_RcscObj)/player/localization_pfilter.o \
	$(Dst_RcscObj)/player/object_table.o \
	$(Dst_RcscObj)/player/penalty_kick_state.o \
	$(Dst_RcscObj)/player/player_command.o \
	$(Dst_RcscObj)/player/player_agent.o \
	$(Dst_RcscObj)/player/player_config.o \
	$(Dst_RcscObj)/player/player_intercept.o \
	$(Dst_RcscObj)/player/player_object.o \
	$(Dst_RcscObj)/player/say_message_builder.o \
	$(Dst_RcscObj)/player/see_state.o \
	$(Dst_RcscObj)/player/self_intercept_v13.o \
	$(Dst_RcscObj)/player/self_object.o \
	$(Dst_RcscObj)/player/soccer_action.o \
	$(Dst_RcscObj)/player/view_grid_map.o \
	$(Dst_RcscObj)/player/view_mode.o \
	$(Dst_RcscObj)/player/visual_sensor.o \
	$(Dst_RcscObj)/player/world_model.o


$(Dst_RcscObj)/player/%.o: $(RcscPath)/player/%.cpp config.mk
	@test -d $(Dst_RcscObj)/player || mkdir -p $(Dst_RcscObj)/player
	@echo ""
	$(GXX) $(INCLUDE) $(DEFS) $(CPPFLAGS) $(EXTR_RCSC_CPPFLAGS) -c "$<" -o "$@"
	@echo ""


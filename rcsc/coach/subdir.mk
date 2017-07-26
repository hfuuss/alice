#===============================================================
# Agent2D-3.1.0 & librcsc-4.1.0 Makefile
# RoboCup2D @ Anhui Unversity of Technology
# Bugs or Suggestions please mail to guofeng1208@163.com
#===============================================================

Rcsc_Obj	+=	\
	$(Dst_RcscObj)/coach/coach_agent.o \
	$(Dst_RcscObj)/coach/coach_audio_sensor.o \
	$(Dst_RcscObj)/coach/coach_command.o \
	$(Dst_RcscObj)/coach/coach_config.o \
	$(Dst_RcscObj)/coach/coach_debug_client.o \
	$(Dst_RcscObj)/coach/global_object.o \
	$(Dst_RcscObj)/coach/global_visual_sensor.o \
	$(Dst_RcscObj)/coach/global_world_model.o \
	$(Dst_RcscObj)/coach/player_type_analyzer.o


$(Dst_RcscObj)/coach/%.o: $(RcscPath)/coach/%.cpp config.mk
	@test -d $(Dst_RcscObj)/coach || mkdir -p $(Dst_RcscObj)/coach
	@echo ""
	$(GXX) $(INCLUDE) $(DEFS) $(CPPFLAGS) $(EXTR_RCSC_CPPFLAGS) -c "$<" -o "$@"
	@echo ""


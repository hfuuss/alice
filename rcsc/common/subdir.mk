#===============================================================
# Agent2D-3.1.0 & librcsc-4.1.0 Makefile
# RoboCup2D @ Anhui Unversity of Technology
# Bugs or Suggestions please mail to guofeng1208@163.com
#===============================================================

Rcsc_Obj	+=	\
	$(Dst_RcscObj)/common/audio_codec.o \
	$(Dst_RcscObj)/common/audio_memory.o \
	$(Dst_RcscObj)/common/basic_client.o \
	$(Dst_RcscObj)/common/logger.o \
	$(Dst_RcscObj)/common/player_param.o \
	$(Dst_RcscObj)/common/player_type.o \
	$(Dst_RcscObj)/common/say_message_parser.o \
	$(Dst_RcscObj)/common/server_param.o \
	$(Dst_RcscObj)/common/soccer_agent.o \
	$(Dst_RcscObj)/common/stamina_model.o \
	$(Dst_RcscObj)/common/team_graphic.o


$(Dst_RcscObj)/common/%.o: $(RcscPath)/common/%.cpp config.mk
	@test -d $(Dst_RcscObj)/common || mkdir -p $(Dst_RcscObj)/common
	@echo ""
	$(GXX) $(INCLUDE) $(DEFS) $(CPPFLAGS) $(EXTR_RCSC_CPPFLAGS) -c "$<" -o "$@"
	@echo ""

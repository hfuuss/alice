#===============================================================
# Agent2D-3.1.0 & librcsc-4.1.0 Makefile
# RoboCup2D @ Anhui Unversity of Technology
# Bugs or Suggestions please mail to guofeng1208@163.com
#===============================================================

Rcsc_Obj	+=	\
	$(Dst_RcscObj)/trainer/trainer_agent.o \
	$(Dst_RcscObj)/trainer/trainer_command.o \
	$(Dst_RcscObj)/trainer/trainer_config.o


$(Dst_RcscObj)/trainer/%.o: $(RcscPath)/trainer/%.cpp config.mk
	@test -d $(Dst_RcscObj)/trainer || mkdir -p $(Dst_RcscObj)/trainer
	@echo ""
	$(GXX) $(INCLUDE) $(DEFS) $(CPPFLAGS) $(EXTR_RCSC_CPPFLAGS) -c "$<" -o "$@"
	@echo ""


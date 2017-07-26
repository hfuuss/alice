#===============================================================
# Agent2D-3.1.0 & librcsc-4.1.0 Makefile
# RoboCup2D @ Anhui Unversity of Technology
# Bugs or Suggestions please mail to guofeng1208@163.com
#===============================================================

Rcsc_Obj	+=	\
	$(Dst_RcscObj)/util/game_mode.o \
	$(Dst_RcscObj)/util/soccer_math.o \
	$(Dst_RcscObj)/util/version.o


$(Dst_RcscObj)/util/%.o: $(RcscPath)/util/%.cpp config.mk
	@test -d $(Dst_RcscObj)/util || mkdir -p $(Dst_RcscObj)/util
	@echo ""
	$(GXX) $(INCLUDE) $(DEFS) $(CPPFLAGS) $(EXTR_RCSC_CPPFLAGS) -c "$<" -o "$@"
	@echo ""


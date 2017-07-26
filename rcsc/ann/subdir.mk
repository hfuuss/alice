#===============================================================
# Agent2D-3.1.0 & librcsc-4.1.0 Makefile
# RoboCup2D @ Anhui Unversity of Technology
# Bugs or Suggestions please mail to guofeng1208@163.com
#===============================================================

Rcsc_Obj	+=	\
	$(Dst_RcscObj)/ann/ngnet.o \
	$(Dst_RcscObj)/ann/rbf.o


$(Dst_RcscObj)/ann/%.o: $(RcscPath)/ann/%.cpp config.mk
	@test -d $(Dst_RcscObj)/ann || mkdir -p $(Dst_RcscObj)/ann
	@echo ""
	$(GXX) $(INCLUDE) $(DEFS) $(CPPFLAGS) $(EXTR_RCSC_CPPFLAGS) -c "$<" -o "$@"
	@echo ""


#===============================================================
# Agent2D-3.1.0 & librcsc-4.1.0 Makefile
# RoboCup2D @ Anhui Unversity of Technology
# Bugs or Suggestions please mail to guofeng1208@163.com
#===============================================================

Rcsc_Obj	+=	\
	$(Dst_RcscObj)/gz/gzcompressor.o \
	$(Dst_RcscObj)/gz/gzfstream.o \
	$(Dst_RcscObj)/gz/gzfilterstream.o


$(Dst_RcscObj)/gz/%.o: $(RcscPath)/gz/%.cpp config.mk
	@test -d $(Dst_RcscObj)/gz || mkdir -p $(Dst_RcscObj)/gz
	@echo ""
	$(GXX) $(INCLUDE) $(DEFS) $(CPPFLAGS) $(EXTR_RCSC_CPPFLAGS) -c "$<" -o "$@"
	@echo ""


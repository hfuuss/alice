#===============================================================
# Agent2D-3.1.0 & librcsc-4.1.0 Makefile
# RoboCup2D @ Anhui Unversity of Technology
# Bugs or Suggestions please mail to guofeng1208@163.com
#===============================================================

Rcsc_Obj	+=	\
	$(Dst_RcscObj)/rcg/holder.o \
	$(Dst_RcscObj)/rcg/parser.o \
	$(Dst_RcscObj)/rcg/parser_v1.o \
	$(Dst_RcscObj)/rcg/parser_v2.o \
	$(Dst_RcscObj)/rcg/parser_v3.o \
	$(Dst_RcscObj)/rcg/parser_v4.o \
	$(Dst_RcscObj)/rcg/parser_v5.o \
	$(Dst_RcscObj)/rcg/serializer.o \
	$(Dst_RcscObj)/rcg/serializer_v1.o \
	$(Dst_RcscObj)/rcg/serializer_v2.o \
	$(Dst_RcscObj)/rcg/serializer_v3.o \
	$(Dst_RcscObj)/rcg/serializer_v4.o \
	$(Dst_RcscObj)/rcg/serializer_v5.o \
	$(Dst_RcscObj)/rcg/util.o


$(Dst_RcscObj)/rcg/%.o: $(RcscPath)/rcg/%.cpp config.mk
	@test -d $(Dst_RcscObj)/rcg || mkdir -p $(Dst_RcscObj)/rcg
	@echo ""
	$(GXX) $(INCLUDE) $(DEFS) $(CPPFLAGS) $(EXTR_RCSC_CPPFLAGS) -c "$<" -o "$@"
	@echo ""


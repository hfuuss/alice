#===============================================================
# Agent2D-3.1.0 & librcsc-4.1.0 Makefile
# RoboCup2D @ Anhui Unversity of Technology
# Bugs or Suggestions please mail to guofeng1208@163.com
#===============================================================

Rcsc_Obj	+=	\
	$(Dst_RcscObj)/param/cmd_line_parser.o \
	$(Dst_RcscObj)/param/conf_file_parser.o \
	$(Dst_RcscObj)/param/param_map.o \
	$(Dst_RcscObj)/param/rcss_param_parser.o


$(Dst_RcscObj)/param/%.o: $(RcscPath)/param/%.cpp config.mk
	@test -d $(Dst_RcscObj)/param || mkdir -p $(Dst_RcscObj)/param
	@echo ""
	$(GXX) $(INCLUDE) $(DEFS) $(CPPFLAGS) $(EXTR_RCSC_CPPFLAGS) -c "$<" -o "$@"
	@echo ""


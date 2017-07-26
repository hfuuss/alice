#===============================================================
# Agent2D-3.1.0 & librcsc-4.1.0 Makefile
# RoboCup2D @ Anhui Unversity of Technology
# Bugs or Suggestions please mail to guofeng1208@163.com
#===============================================================

Rcsc_Obj	+=	\
	$(Dst_RcscObj)/geom/triangle/triangle.o


$(Dst_RcscObj)/geom/triangle/%.o: $(RcscPath)/geom/triangle/%.c config.mk
	@test -d $(Dst_RcscObj)/geom/triangle || \
	mkdir -p $(Dst_RcscObj)/geom/triangle
	@echo ""
	$(GCC) $(CFLAGS) $(EXTR_CFLAGS) \
	-DTRILIBRARY -DREDUCED -DCDT_ONLY -DVOID=int -DREAL=double -c "$<" -o "$@"
	@echo ""


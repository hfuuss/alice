#===============================================================
# Agent2D-3.1.0 & librcsc-4.1.0 Makefile
# RoboCup2D @ Anhui Unversity of Technology
# Bugs or Suggestions please mail to guofeng1208@163.com
#===============================================================

Rcsc_Obj	+=	\
	$(Dst_RcscObj)/geom/angle_deg.o \
	$(Dst_RcscObj)/geom/circle_2d.o \
	$(Dst_RcscObj)/geom/composite_region_2d.o \
	$(Dst_RcscObj)/geom/convex_hull.o \
	$(Dst_RcscObj)/geom/delaunay_triangulation.o \
	$(Dst_RcscObj)/geom/line_2d.o \
	$(Dst_RcscObj)/geom/matrix_2d.o \
	$(Dst_RcscObj)/geom/polygon_2d.o \
	$(Dst_RcscObj)/geom/ray_2d.o \
	$(Dst_RcscObj)/geom/rect_2d.o \
	$(Dst_RcscObj)/geom/sector_2d.o \
	$(Dst_RcscObj)/geom/segment_2d.o \
	$(Dst_RcscObj)/geom/triangle_2d.o \
	$(Dst_RcscObj)/geom/triangulation.o \
	$(Dst_RcscObj)/geom/vector_2d.o \
	$(Dst_RcscObj)/geom/voronoi_diagram_original.o \
	$(Dst_RcscObj)/geom/voronoi_diagram_triangle.o


$(Dst_RcscObj)/geom/%.o: $(RcscPath)/geom/%.cpp config.mk
	@test -d $(Dst_RcscObj)/geom || mkdir -p $(Dst_RcscObj)/geom
	@echo ""
	$(GXX) $(INCLUDE) $(DEFS) $(CPPFLAGS) $(EXTR_RCSC_CPPFLAGS) -c "$<" -o "$@"
	@echo ""


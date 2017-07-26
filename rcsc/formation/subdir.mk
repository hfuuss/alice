#===============================================================
# Agent2D-3.1.0 & librcsc-4.1.0 Makefile
# RoboCup2D @ Anhui Unversity of Technology
# Bugs or Suggestions please mail to guofeng1208@163.com
#===============================================================

Rcsc_Obj	+=	\
	$(Dst_RcscObj)/formation/formation.o \
	$(Dst_RcscObj)/formation/sample_data.o \
	$(Dst_RcscObj)/formation/formation_bpn.o \
	$(Dst_RcscObj)/formation/formation_cdt.o \
	$(Dst_RcscObj)/formation/formation_dt.o \
	$(Dst_RcscObj)/formation/formation_knn.o \
	$(Dst_RcscObj)/formation/formation_ngnet.o \
	$(Dst_RcscObj)/formation/formation_rbf.o \
	$(Dst_RcscObj)/formation/formation_sbsp.o \
	$(Dst_RcscObj)/formation/formation_static.o \
	$(Dst_RcscObj)/formation/formation_uva.o


$(Dst_RcscObj)/formation/%.o: $(RcscPath)/formation/%.cpp config.mk
	@test -d $(Dst_RcscObj)/formation || mkdir -p $(Dst_RcscObj)/formation
	@echo ""
	$(GXX) $(INCLUDE) $(DEFS) $(CPPFLAGS) $(EXTR_RCSC_CPPFLAGS) -c "$<" -o "$@"
	@echo ""


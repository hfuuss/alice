#===============================================================
# Agent2D-3.1.0 & librcsc-4.1.0 Makefile
# RoboCup2D @ Anhui Unversity of Technology
# Bugs or Suggestions please mail to guofeng1208@163.com
#===============================================================

Rcsc_Obj	+=	\
	$(Dst_RcscObj)/net/basic_socket.o \
	$(Dst_RcscObj)/net/udp_socket.o \
	$(Dst_RcscObj)/net/tcp_socket.o


$(Dst_RcscObj)/net/%.o: $(RcscPath)/net/%.cpp config.mk
	@test -d $(Dst_RcscObj)/net || mkdir -p $(Dst_RcscObj)/net
	@echo ""
	$(GXX) $(INCLUDE) $(DEFS) $(CPPFLAGS) $(EXTR_RCSC_CPPFLAGS) -c "$<" -o "$@"
	@echo ""


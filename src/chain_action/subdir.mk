

Player_Obj	+=	\
		$(Dst_SrcObj)/chain_action/tackle_generator.o\
		$(Dst_SrcObj)/chain_action/actgen_cross.o \
		$(Dst_SrcObj)/chain_action/actgen_self_pass.o \
		$(Dst_SrcObj)/chain_action/actgen_shoot.o \
		$(Dst_SrcObj)/chain_action/actgen_short_dribble.o \
		$(Dst_SrcObj)/chain_action/actgen_strict_check_pass.o \
		$(Dst_SrcObj)/chain_action/action_chain_graph.o \
		$(Dst_SrcObj)/chain_action/action_chain_holder.o \
		$(Dst_SrcObj)/chain_action/bhv_chain_action.o \
		$(Dst_SrcObj)/chain_action/bhv_normal_dribble.o \
		$(Dst_SrcObj)/chain_action/bhv_pass_kick_find_receiver.o \
		$(Dst_SrcObj)/chain_action/bhv_strict_check_shoot.o \
		$(Dst_SrcObj)/chain_action/clear_ball.o \
		$(Dst_SrcObj)/chain_action/clear_generator.o \
		$(Dst_SrcObj)/chain_action/cooperative_action.o \
		$(Dst_SrcObj)/chain_action/cross_generator.o \
		$(Dst_SrcObj)/chain_action/dribble.o \
		$(Dst_SrcObj)/chain_action/field_analyzer.o \
		$(Dst_SrcObj)/chain_action/hold_ball.o \
		$(Dst_SrcObj)/chain_action/neck_turn_to_receiver.o \
		$(Dst_SrcObj)/chain_action/pass.o \
		$(Dst_SrcObj)/chain_action/predict_state.o \
		$(Dst_SrcObj)/chain_action/self_pass_generator.o \
		$(Dst_SrcObj)/chain_action/shoot.o \
		$(Dst_SrcObj)/chain_action/shoot_generator.o \
		$(Dst_SrcObj)/chain_action/short_dribble_generator.o \
		$(Dst_SrcObj)/chain_action/simple_pass_checker.o \
		$(Dst_SrcObj)/chain_action/strict_check_pass_generator.o \



$(Dst_SrcObj)/chain_action/%.o: $(SrcPath)/chain_action/%.cpp config.mk
	@test -d $(Dst_SrcObj)/chain_action || mkdir -p $(Dst_SrcObj)/chain_action
	@echo ""
	$(GXX) $(INCLUDE) $(DEFS) $(CPPFLAGS) $(EXTR_SRC_CPPFLAGS) -c "$<" -o "$@"
	@echo ""


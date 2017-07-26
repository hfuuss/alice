#=======================================================
# Agent2D-3.1.0 & librcsc-4.1.0 Makefile
# RoboCup2D @ Anhui Unversity of Technology
# Bugs or Suggestions please mail to guofeng1208@163.com
#=======================================================


config_deps +=  $(patsubst %, $(Dst_Path)/%,    \
                              $(wildcard $(DataPath)/formations-dt/*) )

config_deps +=  $(patsubst %, $(Dst_Path)/%,    \
                              $(wildcard $(DataPath)/weights/*) )

config_deps +=  $(patsubst %, $(Dst_Path)/%,    \
                              $(wildcard $(DataPath)/*.conf) )

                              
config_deps +=  $(patsubst %, $(Dst_Path)/%, \
				$(wildcard $(DataPath)/s) )
                              
                              
config_deps +=  $(patsubst %, $(Dst_Path)/%, \
				$(wildcard $(DataPath)/start) )                            
config_deps +=  $(patsubst %, $(Dst_Path)/%,    \
                              $(wildcard $(DataPath)/*.net) )

$(Dst_Path)/$(DataPath)/%.net: $(DataPath)/%.net
	@test -d $(Dst_Path)/$(DataPath) || mkdir -p $(Dst_Path)/$(DataPath)
	@cp -f $< $@
	@echo ">>> Update  \"$@\""


$(Dst_Path)/$(DataPath)/formations-dt/%: $(DataPath)/formations-dt/%
	@test -d $(Dst_Path)/$(DataPath)/formations-dt || \
	mkdir -p $(Dst_Path)/$(DataPath)/formations-dt
	@cp -f  -r  $< $@
	@echo ">>> Update  \"$@\""


$(Dst_Path)/$(DataPath)/s: $(DataPath)/s
	@cp -f  -r  $<  $(Dst_Path)
	@echo ">>> Update  \"$@\""

	
$(Dst_Path)/$(DataPath)/start: $(DataPath)/start
	@cp -f  -r  $<  $(Dst_Path)
	@echo ">>> Update  \"$@\""
	

$(Dst_Path)/$(DataPath)/weights/%: $(DataPath)/weights/%
	@test -d $(Dst_Path)/$(DataPath)/weights || \
	mkdir -p $(Dst_Path)/$(DataPath)/weights
	@cp -f  -r  $< $@
	@echo ">>> Update  \"$@\""



	
$(Dst_Path)/$(DataPath)/%.conf: $(DataPath)/%.conf
	@test -d $(Dst_Path)/$(DataPath) || mkdir -p $(Dst_Path)/$(DataPath)
	@cp -f $< $@
	@echo ">>> Update  \"$@\""\




cleandata:
	@rm -rf $(Dst_Path)/$(DataPath)
	@rm -f $(Dst_Path)/*.sh


#End of Makefile

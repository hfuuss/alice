

Player_Name     =   Alice_Player
Coach_Name      =   Alice_Coach
Trainer_Name    =   Alice_Trainer

#####################
SrcPath     =   src
RcscPath    =   rcsc
DataPath    =   data

Dst_Top     =   ..
Dst_ObjPath =   $(Dst_Top)/.bulidpack
Dst_SrcObj  =   $(Dst_ObjPath)/src
Dst_RcscObj =   $(Dst_ObjPath)/rcsc

Dst_Path    =   $(Dst_Top)/Alice-EXE

player_target   =   $(Dst_Path)/$(Player_Name)
coach_target    =   $(Dst_Path)/$(Coach_Name)
trainer_target  =   $(Dst_Path)/$(Trainer_Name)
config_target   =   update_data


#####################


GCC             =   gcc -std=c++11
GXX             =   g++ -std=c++11

CFLAGS          =   -O -Wall -W -pipe \
                    -MMD -MP -MF "$(@:%.o=%.d)" -MT "$(@:%.o=%.d)" -MT "$@"
EXTR_CFLAGS     =   
CPPFLAGS        =   -O -Wall -W -pipe \
                    -MMD -MP -MF "$(@:%.o=%.d)" -MT "$(@:%.o=%.d)" -MT "$@"

 
DEFS            =   -DHAVE_CONFIG_H -DHAVE_CONFIG
LD_PATH         =   -L.
INCLUDE         =   -I. \
                    -I./src/chain_action

EXTR_CFLAGS			+=  -ggdb
EXTR_SRC_CPPFLAGS		+=  -ggdb
EXTR_RCSC_CPPFLAGS	+=  -ggdb

EXTR_CFLAGS			+=  -shared -fPIC
EXTR_RCSC_CPPFLAGS	+=  -shared -fPIC
EXTR_LDFLAGS			+=  -shared
Rcsc_lib				+=  Rcsc_lib

####################
SUBMAKE =   \
            $(RcscPath)/player/subdir.mk    \
            $(RcscPath)/coach/subdir.mk \
            $(RcscPath)/trainer/subdir.mk   \
            $(RcscPath)/action/subdir.mk    \
            $(RcscPath)/common/subdir.mk    \
            $(RcscPath)/formation/subdir.mk \
            $(RcscPath)/monitor/subdir.mk   \
            $(RcscPath)/geom/subdir.mk  \
            $(RcscPath)/geom/triangle/subdir.mk \
            $(RcscPath)/param/subdir.mk \
            $(RcscPath)/net/subdir.mk   \
            $(RcscPath)/rcg/subdir.mk   \
            $(RcscPath)/time/subdir.mk  \
            $(RcscPath)/ann/subdir.mk   \
            $(RcscPath)/gz/subdir.mk    \
            $(RcscPath)/util/subdir.mk
SUBMAKE +=  \
            $(SrcPath)/chain_action/subdir.mk   \
            $(SrcPath)/subdir.mk
SUBMAKE +=  \
            $(DataPath)/subdir.mk

